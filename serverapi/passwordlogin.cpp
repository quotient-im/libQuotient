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
using namespace QMatrixClient::ServerApi;

PasswordLogin::PasswordLogin(QString user, QString password)
    : CallConfig("PasswordLogin", HttpVerb::Post, "_matrix/client/r0/login"
        , Query()
        , Data(
            { { "type", "m.login.password" }
            , { "user", user }
            , { "password", password }
            })
        , false)
{ }

Result<AccessData> PasswordLogin::parseReply(const QJsonObject& json) const
{
    if( !json.contains("access_token") || !json.contains("home_server") || !json.contains("user_id") )
        return { UserDefinedError, "Unexpected data" };

    AccessData ad;
    ad.token = json.value("access_token").toString();
    ad.server = json.value("home_server").toString();
    ad.id = json.value("user_id").toString();
    return std::move(ad);
}
