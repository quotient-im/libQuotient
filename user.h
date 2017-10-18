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

namespace QMatrixClient
{
    class Event;
    class Connection;
    class User: public QObject
    {
            Q_OBJECT
        public:
            User(QString userId, Connection* connection);
            virtual ~User();

            /**
             * Returns the id of the user
             */
            Q_INVOKABLE QString id() const;

            /**
             * Returns the name chosen by the user
             */
            Q_INVOKABLE QString name() const;

            /**
             * Returns the name that should be used to display the user.
             */
            Q_INVOKABLE QString displayname() const;

            /**
             * Returns the name of bridge the user is connected from or empty.
            */
            Q_INVOKABLE QString bridged() const;

            QPixmap avatar(int requestedWidth, int requestedHeight);

            const QUrl& avatarUrl() const;

            void processEvent(Event* event);

        public slots:
            void requestAvatar();
            void rename(const QString& newName);

        signals:
            void nameChanged(User*, QString);
            void avatarChanged(User* user);

        private slots:
            void updateName(const QString& newName);

        private:
            class Private;
            Private* d;
    };
}
