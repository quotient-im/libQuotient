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

#include "passwordlogin.h"

using namespace QMatrixClient;

class PasswordLogin::Private
{
    public:
        QString user;
        QString password;
};

PasswordLogin::PasswordLogin(ConnectionData* connection, QString user, QString password)
    : SimpleJob(connection, JobHttpType::PostJob, "PasswordLogin", false)
    , d(new Private{user, password})
    , token("access_token", *this)
    , server("home_server", *this)
    , id("user_id", *this)
{
}

PasswordLogin::~PasswordLogin()
{
    delete d;
}

QString PasswordLogin::apiPath()
{
    return "_matrix/client/r0/login";
}

QJsonObject PasswordLogin::data()
{
    QJsonObject json;
    json.insert("type", "m.login.password");
    json.insert("user", d->user);
    json.insert("password", d->password);
    return json;
}
