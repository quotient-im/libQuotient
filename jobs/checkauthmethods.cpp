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

#include "checkauthmethods.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

using namespace QMatrixClient;

class CheckAuthMethods::Private
{
    public:
        QString session;
};

CheckAuthMethods::CheckAuthMethods(const ConnectionData* connection)
    : BaseJob(connection, HttpVerb::Get, "CheckAuthMethods",
              "_matrix/client/r0/login", Query(), Data(), false)
    , d(new Private)
{
}

CheckAuthMethods::~CheckAuthMethods()
{
    delete d;
}

QString CheckAuthMethods::session()
{
    return d->session;
}

BaseJob::Status CheckAuthMethods::parseJson(const QJsonDocument& data)
{
    // TODO
    return { BaseJob::StatusCode::UserDefinedError, "Not implemented" };
}
