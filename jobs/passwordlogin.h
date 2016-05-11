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

#ifndef QMATRIXCLIENT_PASSWORDLOGIN_H
#define QMATRIXCLIENT_PASSWORDLOGIN_H

#include "simplejob.h"

namespace QMatrixClient
{
    class ConnectionData;

    class PasswordLogin : public SimpleJob
    {
        public:
            PasswordLogin(ConnectionData* connection, QString user, QString password);
            virtual ~PasswordLogin();

            ResultItem<QString> token;
            ResultItem<QString> id;
            ResultItem<QString> server;

        protected:
            virtual QString apiPath() override;
            virtual QJsonObject data() override;

        private:
            class Private;
            Private* d;
    };
}

#endif // QMATRIXCLIENT_PASSWORDLOGIN_H
