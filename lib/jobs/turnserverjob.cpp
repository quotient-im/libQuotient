/******************************************************************************
 * Copyright (C) 2017 Marius Gripsgard <marius@ubports.com>
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

#include "turnserverjob.h"
#include "util.h"

using namespace QMatrixClient;

class TurnServerJob::Private
{
    public:
        QJsonObject _turnObject;
};

TurnServerJob::TurnServerJob()
    : BaseJob(HttpVerb::Get, "TurnServerJob",
              QStringLiteral("/_matrix/client/r0/voip/turnServer"))
    , d(new Private)
{
}

TurnServerJob::~TurnServerJob()
{
    delete d;
}

QJsonObject TurnServerJob::toJson() const
{
    return d->_turnObject;
}

BaseJob::Status TurnServerJob::parseJson(const QJsonDocument& data)
{
    QJsonObject json = data.object();

    if( json.contains("uris") )
    {
        d->_turnObject = json;
        return Success;
    }

    return { UserDefinedError, "turnServer object does not include uris" };
}
