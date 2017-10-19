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
#include "events/event.h"
#include "events/roommemberevent.h"
#include "jobs/mediathumbnailjob.h"
#include "jobs/generated/profile.h"

#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <QtGui/QIcon>
#include <QtCore/QRegularExpression>

using namespace QMatrixClient;

class User::Private
{
    public:
        Private(QString userId, Connection* connection)
            : q(nullptr), userId(std::move(userId)), connection(connection)
            , defaultIcon(QIcon::fromTheme(QStringLiteral("user-available")))
            , avatarValid(false) , avatarOngoingRequest(false)
        { }

        User* q;
        QString userId;
        QString name;
        QUrl avatarUrl;
        Connection* connection;

        QPixmap avatar;
        QIcon defaultIcon;
        QSize requestedSize;
        bool avatarValid;
        bool avatarOngoingRequest;
        /// Map of requested size to the actual pixmap used for it
        /// (it's a shame that QSize has no predefined qHash()).
        QHash<QPair<int,int>, QPixmap> scaledAvatars;
        QString bridged;

        void requestAvatar();
};

User::User(QString userId, Connection* connection)
    : QObject(connection), d(new Private(userId, connection))
{
    d->q = this; // Initialization finished
}

User::~User()
{
    delete d;
}

QString User::id() const
{
    return d->userId;
}

QString User::name() const
{
    return d->name;
}

void User::updateName(const QString& newName)
{
    const auto oldName = name();
    if (d->name != newName)
    {
        d->name = newName;
        emit nameChanged(this, oldName);
    }
}

void User::rename(const QString& newName)
{
    auto job = d->connection->callApi<SetDisplayNameJob>(id(), newName);
    connect(job, &BaseJob::success, this, [=] { updateName(newName); });
}

QString User::displayname() const
{
    if( !d->name.isEmpty() )
        return d->name;
    return d->userId;
}

QString User::bridged() const {
    return d->bridged;
}

QPixmap User::avatar(int width, int height)
{
    QSize size(width, height);

    // FIXME: Alternating between longer-width and longer-height requests
    // is a sure way to trick the below code into constantly getting another
    // image from the server because the existing one is alleged unsatisfactory.
    // This is plain abuse by the client, though; so not critical for now.
    if( (!d->avatarValid && d->avatarUrl.isValid() && !d->avatarOngoingRequest)
        || width > d->requestedSize.width()
        || height > d->requestedSize.height() )
    {
        qCDebug(MAIN) << "Getting avatar for" << id()
                      << "from" << d->avatarUrl.toString();
        d->requestedSize = size;
        d->avatarOngoingRequest = true;
        QTimer::singleShot(0, this, SLOT(requestAvatar()));
    }

    if( d->avatar.isNull() )
    {
        if (d->defaultIcon.isNull())
            return d->avatar;

        d->avatar = d->defaultIcon.pixmap(size);
    }

    auto& pixmap = d->scaledAvatars[{width, height}]; // Create the entry if needed
    if (pixmap.isNull())
    {
        pixmap = d->avatar.scaled(width, height,
                    Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return pixmap;
}

const QUrl& User::avatarUrl() const
{
    return d->avatarUrl;
}

void User::processEvent(Event* event)
{
    if( event->type() == EventType::RoomMember )
    {
        auto e = static_cast<RoomMemberEvent*>(event);
        if (e->membership() == MembershipType::Leave)
            return;

        auto newName = e->displayName();
        QRegularExpression reSuffix(" \\((IRC|Gitter)\\)$");
        auto match = reSuffix.match(d->name);
        if (match.hasMatch())
        {
            d->bridged = match.captured(1);
            newName.truncate(match.capturedStart(0));
        }
        updateName(newName);
        if( d->avatarUrl != e->avatarUrl() )
        {
            d->avatarUrl = e->avatarUrl();
            d->avatarValid = false;
        }
    }
}

void User::requestAvatar()
{
    d->requestAvatar();
}

void User::Private::requestAvatar()
{
    auto* job = connection->callApi<MediaThumbnailJob>(avatarUrl, requestedSize);
    connect( job, &MediaThumbnailJob::success, [=]() {
        avatarOngoingRequest = false;
        avatarValid = true;
        avatar = job->scaledThumbnail(requestedSize);
        scaledAvatars.clear();
        emit q->avatarChanged(q);
    });
}

