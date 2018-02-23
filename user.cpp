/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "user.h"

#include "connection.h"
#include "avatar.h"
#include "events/event.h"
#include "events/roommemberevent.h"
#include "jobs/generated/profile.h"
#include "jobs/generated/content-repo.h"

#include <QtCore/QTimer>
#include <QtCore/QRegularExpression>
#include <QtCore/QPointer>
#include <QtCore/QStringBuilder>

#include <functional>

using namespace QMatrixClient;

class User::Private
{
    public:
        Private(QString userId, Connection* connection)
            : userId(std::move(userId)), connection(connection)
        { }

        QString userId;
        QString name;
        QString bridged;
        Connection* connection;
        Avatar avatar { QIcon::fromTheme(QStringLiteral("user-available")) };

        void setAvatar(QString contentUri, User* q);
};

User::User(QString userId, Connection* connection)
    : QObject(connection), d(new Private(std::move(userId), connection))
{
    setObjectName(userId);
}

User::~User()
{
    delete d;
}

QString User::id() const
{
    return d->userId;
}

bool User::isGuest() const
{
    Q_ASSERT(!d->userId.isEmpty() && d->userId.startsWith('@'));
    auto it = std::find_if_not(d->userId.begin() + 1, d->userId.end(),
                               [] (QChar c) { return c.isDigit(); });
    Q_ASSERT(it != d->userId.end());
    return *it == ':';
}

QString User::name() const
{
    return d->name;
}

void User::updateName(const QString& newName)
{
    const auto oldName = name();
    if (oldName != newName)
    {
        d->name = newName;
        setObjectName(displayname());
        emit nameChanged(newName, oldName);
    }
}

void User::updateAvatarUrl(const QUrl& newUrl)
{
    if (d->avatar.updateUrl(newUrl))
        emit avatarChanged(this);
}

void User::rename(const QString& newName)
{
    auto job = d->connection->callApi<SetDisplayNameJob>(id(), newName);
    connect(job, &BaseJob::success, this, [=] { updateName(newName); });
}

bool User::setAvatar(const QString& fileName)
{
    return avatarObject().upload(d->connection, fileName,
                [this] (QString contentUri) { d->setAvatar(contentUri, this); });
}

bool User::setAvatar(QIODevice* source)
{
    return avatarObject().upload(d->connection, source,
                [this] (QString contentUri) { d->setAvatar(contentUri, this); });
}

void User::Private::setAvatar(QString contentUri, User* q)
{
    auto* j = connection->callApi<SetAvatarUrlJob>(userId, contentUri);
    connect(j, &BaseJob::success, q, [q] { emit q->avatarChanged(q); });
}

QString User::displayname() const
{
    return d->name.isEmpty() ? d->userId : d->name;
}

QString User::fullName() const
{
    return d->name.isEmpty() ? d->userId :
                               d->name % " (" % d->userId % ')';
}

QString User::bridged() const
{
    return d->bridged;
}

const Avatar& User::avatarObject() const
{
    return d->avatar;
}

QImage User::avatar(int dimension)
{
    return avatar(dimension, dimension);
}

QImage User::avatar(int width, int height)
{
    return avatar(width, height, []{});
}

QImage User::avatar(int width, int height, Avatar::get_callback_t callback)
{
    return avatarObject().get(d->connection, width, height,
                      [this,callback] { emit avatarChanged(this); callback(); });
}

QString User::avatarMediaId() const
{
    return avatarObject().mediaId();
}

QUrl User::avatarUrl() const
{
    return avatarObject().url();
}

void User::processEvent(Event* event)
{
    if( event->type() == EventType::RoomMember )
    {
        auto e = static_cast<RoomMemberEvent*>(event);
        if (e->membership() == MembershipType::Leave)
            return;

        auto newName = e->displayName();
        QRegularExpression reSuffix(" \\((IRC|Gitter|Telegram)\\)$");
        auto match = reSuffix.match(newName);
        if (match.hasMatch())
        {
            d->bridged = match.captured(1);
            newName.truncate(match.capturedStart(0));
        }
        updateName(newName);
        updateAvatarUrl(e->avatarUrl());
    }
}
