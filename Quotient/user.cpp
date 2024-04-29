// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "user.h"

#include "avatar.h"
#include "connection.h"
#include "logging_categories_p.h"
#include "room.h"

#include "csapi/profile.h"

#include "events/roommemberevent.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringBuilder>
#include <QtCore/QTimer>

using namespace Quotient;

class Q_DECL_HIDDEN User::Private {
public:
    explicit Private(QString userId) : id(std::move(userId)) { }

    QString id;

    QString defaultName;
    QUrl defaultAvatarUrl;
};

User::User(QString userId, Connection* connection)
    : QObject(connection), d(makeImpl<Private>(std::move(userId)))
{
    setObjectName(id());
}

Connection* User::connection() const
{
    Q_ASSERT(parent());
    return static_cast<Connection*>(parent());
}

void User::load()
{
    connection()->callApi<GetUserProfileJob>(id()).then([this](const auto* profileJob) {
        d->defaultName = profileJob->displayname();
        d->defaultAvatarUrl = profileJob->avatarUrl();
        emit defaultNameChanged();
        emit defaultAvatarChanged();
    });
}

QString User::id() const { return d->id; }

QString User::name() const { return d->defaultName; }

QString User::displayname() const
{
    return d->defaultName.isEmpty() ? d->id : d->defaultName;
}

QString User::fullName() const
{
    return displayname().isEmpty() ? id() : (displayname() % " ("_L1 % id() % u')');
}

bool User::isGuest() const
{
    Q_ASSERT(!d->id.isEmpty() && d->id.startsWith(u'@'));
    auto it = std::find_if_not(d->id.cbegin() + 1, d->id.cend(),
                               [](QChar c) { return c.isDigit(); });
    Q_ASSERT(it != d->id.cend());
    return *it == u':';
}

void User::rename(const QString& newName)
{
    const auto actualNewName = sanitized(newName);
    if (actualNewName == d->defaultName)
        return; // Nothing to do

    connection()->callApi<SetDisplayNameJob>(id(), actualNewName).then([this, actualNewName] {
        // Check again, it could have changed meanwhile
        if (actualNewName != d->defaultName) {
            d->defaultName = actualNewName;
            emit defaultNameChanged();
        } else
            qCWarning(MAIN) << "User" << id() << "already has profile name set to" << actualNewName;
    });
}

void User::rename(const QString& newName, Room* r)
{
    if (!r) {
        qCWarning(MAIN) << "Passing a null room to two-argument User::rename()"
                           "is incorrect; client developer, please fix it";
        rename(newName);
        return;
    }
    // #481: take the current state and update it with the new name
    if (const auto& maybeEvt = r->currentState().get<RoomMemberEvent>(id())) {
        auto content = maybeEvt->content();
        if (content.membership == Membership::Join) {
            content.displayName = sanitized(newName);
            r->setState<RoomMemberEvent>(id(), std::move(content));
            // The state will be updated locally after it arrives with sync
            return;
        }
    }
    qCCritical(MEMBERS)
        << "Attempt to rename a non-member in a room context - ignored";
}

void User::doSetAvatar(const QUrl& contentUri)
{
    connection()->callApi<SetAvatarUrlJob>(id(), contentUri).then(this, [this, contentUri] {
        if (contentUri != d->defaultAvatarUrl) {
            connection()->userAvatar(d->defaultAvatarUrl).updateUrl(contentUri);
            emit defaultAvatarChanged();
        } else
            qCWarning(MAIN) << "User" << id() << "already has avatar URL set to"
                            << contentUri.toDisplayString();
    });
}

bool User::setAvatar(const QString& fileName)
{
    auto ft = connection()
                  ->userAvatar(d->defaultAvatarUrl)
                  .upload(fileName)
                  .then(std::bind_front(&User::doSetAvatar, this));
    // The return value only says whether the upload has started; the subsequent url setting job
    // will only start after the upload completes
    return !ft.isCanceled();
}

bool User::setAvatar(QIODevice* source)
{
    auto ft = connection()
                  ->userAvatar(d->defaultAvatarUrl)
                  .upload(source)
                  .then(std::bind_front(&User::doSetAvatar, this));
    // The return value only says whether the upload has started; the subsequent url setting job
    // will only start after the upload completes
    return !ft.isCanceled();
}

void User::removeAvatar() const
{
    connection()->callApi<SetAvatarUrlJob>(id(), QUrl());
}

void User::requestDirectChat() { connection()->requestDirectChat(d->id); }

void User::ignore() const { connection()->addToIgnoredUsers(d->id); }

void User::unmarkIgnore() const { connection()->removeFromIgnoredUsers(d->id); }

bool User::isIgnored() const { return connection()->isIgnored(d->id); }

QString User::avatarMediaId() const
{
    return d->defaultAvatarUrl.authority() + d->defaultAvatarUrl.path();
}

QUrl User::avatarUrl() const
{
    return d->defaultAvatarUrl;
}
