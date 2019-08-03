/******************************************************************************
 * Copyright (C) 2016 Felix Rohrbach <kde@fxrh.de>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "syncjob.h"

using namespace QMatrixClient;

static size_t jobId = 0;

SyncJob::SyncJob(const QString& since, const QString& filter, int timeout,
                 const QString& presence)
    : BaseJob(HttpVerb::Get, QStringLiteral("SyncJob-%1").arg(++jobId),
              QStringLiteral("_matrix/client/r0/sync"))
{
    setLoggingCategory(SYNCJOB);
    QUrlQuery query;
    if (!filter.isEmpty())
        query.addQueryItem(QStringLiteral("filter"), filter);
    if (!presence.isEmpty())
        query.addQueryItem(QStringLiteral("set_presence"), presence);
    if (timeout >= 0)
        query.addQueryItem(QStringLiteral("timeout"), QString::number(timeout));
    if (!since.isEmpty())
        query.addQueryItem(QStringLiteral("since"), since);
    setRequestQuery(query);

    setMaxRetries(std::numeric_limits<int>::max());
}

SyncJob::SyncJob(const QString& since, const Filter& filter, int timeout,
                 const QString& presence)
    : SyncJob(since,
              QJsonDocument(toJson(filter)).toJson(QJsonDocument::Compact),
              timeout, presence)
{}

BaseJob::Status SyncJob::parseJson(const QJsonDocument& data)
{
    d.parseJson(data.object());
    if (d.unresolvedRooms().isEmpty())
        return BaseJob::Success;

    qCCritical(MAIN).noquote() << "Incomplete sync response, missing rooms:"
                               << d.unresolvedRooms().join(',');
    return BaseJob::IncorrectResponseError;
}
