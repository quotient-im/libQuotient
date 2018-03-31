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
        QString returned_id;
        QString returned_server;
        QString returned_token;
};

PasswordLogin::PasswordLogin(QString user, QString password)
    : BaseJob(HttpVerb::Post, "PasswordLogin",
              "_matrix/client/r0/login", Query(), Data(), false)
    , d(new Private)
{
    QJsonObject _data;
    _data.insert("type", QStringLiteral("m.login.password"));
    _data.insert("user", user);
    _data.insert("password", password);
    setRequestData(_data);
}

PasswordLogin::~PasswordLogin()
{
    delete d;
}

QString PasswordLogin::token() const
{
    return d->returned_token;
}

QString PasswordLogin::id() const
{
    return d->returned_id;
}

QString PasswordLogin::server() const
{
    return d->returned_server;
}

BaseJob::Status PasswordLogin::parseJson(const QJsonDocument& data)
{
    QJsonObject json = data.object();
    if( !json.contains("access_token") || !json.contains("home_server") || !json.contains("user_id") )
    {
        return { UserDefinedError, "No expected data" };
    }
    d->returned_token = json.value("access_token").toString();
    d->returned_server = json.value("home_server").toString();
    d->returned_id = json.value("user_id").toString();
    return Success;
}
