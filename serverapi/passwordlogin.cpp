/******************************************************************************
 * Copyright (C) 2016 Kitsune Ral <kitsune-ral@users.sf.net>
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

PasswordLogin::PasswordLogin(QString user, QString password)
    : ServerCallSetup("PasswordLogin",
        RequestParams (HttpType::Post, "_matrix/client/r0/login"
        , Query()
        , Data(
            { { "type", "m.login.password" }
            , { "user", user }
            , { "password", password }
            })
        , false
        ))
{ }


void PasswordLogin::fillResult(const QJsonObject& json)
{
    if( !json.contains("access_token") || !json.contains("home_server") || !json.contains("user_id") )
    {
        setStatus(CallStatus::UserDefinedError, "Unexpected data");
    }
    token = json.value("access_token").toString();
    server = json.value("home_server").toString();
    id = json.value("user_id").toString();
}
