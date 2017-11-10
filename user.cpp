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

#include <QtCore/QTimer>
#include <QtCore/QRegularExpression>

using namespace QMatrixClient;

class User::Private
{
    public:
        Private(QString userId, Connection* connection)
            : userId(std::move(userId)), connection(connection)
            , avatar(connection, QIcon::fromTheme(QStringLiteral("user-available")))
        { }

        QString userId;
        QString name;
        QString bridged;
        Connection* connection;
        Avatar avatar;
};

User::User(QString userId, Connection* connection)
    : QObject(connection), d(new Private(std::move(userId), connection))
{ }

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

Avatar& User::avatarObject()
{
    return d->avatar;
}

QPixmap User::avatar(int width, int height)
{
    return d->avatar.get(width, height, [=] { emit avatarChanged(this); });
}

QUrl User::avatarUrl() const
{
    return d->avatar.url();
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
        auto match = reSuffix.match(newName);
        if (match.hasMatch())
        {
            d->bridged = match.captured(1);
            newName.truncate(match.capturedStart(0));
        }
        updateName(newName);
        if (d->avatar.updateUrl(e->avatarUrl()))
            emit avatarChanged(this);
    }
}
