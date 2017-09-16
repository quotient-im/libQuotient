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
#include "util.h"

#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <QtGui/QIcon>
#include <QtCore/QRegularExpression>
#include <algorithm>

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
        QVector<QPixmap> scaledAvatars;
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
    return croppedAvatar(width, height); // FIXME: Return an uncropped avatar;
}

QPixmap User::croppedAvatar(int width, int height)
{
    QSize size(width, height);

    if( !d->avatarValid
        || width > d->requestedSize.width()
        || height > d->requestedSize.height() )
    {
        if( !d->avatarOngoingRequest && d->avatarUrl.isValid() )
        {
            qCDebug(MAIN) << "Getting avatar for" << id();
            d->requestedSize = size;
            d->avatarOngoingRequest = true;
            QTimer::singleShot(0, this, SLOT(requestAvatar()));
        }
    }

    if( d->avatar.isNull() )
    {
        if (d->defaultIcon.isNull())
            return d->avatar;

        d->avatar = d->defaultIcon.pixmap(size);
    }

    for (const QPixmap& p: d->scaledAvatars)
    {
        if (p.size() == size)
            return p;
    }
    QPixmap newlyScaled = d->avatar.scaled(size,
        Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QPixmap scaledAndCroped = newlyScaled.copy(
        std::max((newlyScaled.width() - width)/2, 0),
        std::max((newlyScaled.height() - height)/2, 0),
        width, height);
    d->scaledAvatars.push_back(scaledAndCroped);
    return scaledAndCroped;
}

void User::processEvent(Event* event)
{
    if( event->type() == EventType::RoomMember )
    {
        auto e = static_cast<RoomMemberEvent*>(event);
        if (e->membership() == MembershipType::Leave)
            return;

        if( d->name != e->displayName() )
        {
            const auto oldName = d->name;
            d->name = e->displayName();
            QRegularExpression reSuffix(" \\((IRC|Gitter)\\)$");
            auto match = reSuffix.match(d->name);
            if (match.hasMatch()) {
                d->bridged = match.captured(1);
                d->name = d->name.left(match.capturedStart(0));
            }
            emit nameChanged(this, oldName);
        }
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
