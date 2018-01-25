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

#pragma once

#include <QtCore/QString>
#include <QtCore/QObject>
#include "avatar.h"

namespace QMatrixClient
{
    class Event;
    class Connection;
    class User: public QObject
    {
            Q_OBJECT
            Q_PROPERTY(QString id READ id CONSTANT)
            Q_PROPERTY(QString name READ name NOTIFY nameChanged)
            Q_PROPERTY(QString displayName READ displayname NOTIFY nameChanged STORED false)
            Q_PROPERTY(QString bridgeName READ bridged NOTIFY nameChanged STORED false)
            Q_PROPERTY(QUrl avatarUrl READ avatarUrl NOTIFY avatarChanged)
        public:
            User(QString userId, Connection* connection);
            ~User() override;

            /**
             * Returns the id of the user
             */
            QString id() const;

            /**
             * Returns the name chosen by the user
             */
            QString name() const;

            /**
             * Returns the name that should be used to display the user.
             */
            QString displayname() const;

            /**
             * Returns the name of bridge the user is connected from or empty.
            */
            QString bridged() const;

            const Avatar& avatarObject();
            Q_INVOKABLE QImage avatar(int dimension);
            Q_INVOKABLE QImage avatar(int requestedWidth, int requestedHeight);

            QUrl avatarUrl() const;

            void processEvent(Event* event);

        public slots:
            void rename(const QString& newName);
            bool setAvatar(const QString& fileName);
            bool setAvatar(QIODevice* source);

        signals:
            void nameChanged(QString newName, QString oldName);
            void avatarChanged(User* user);

        private slots:
            void updateName(const QString& newName);
            void updateAvatarUrl(const QUrl& newUrl);

        private:
            class Private;
            Private* d;
    };
}
