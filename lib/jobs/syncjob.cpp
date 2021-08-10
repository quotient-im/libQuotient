// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "syncjob.h"

using namespace Quotient;

static size_t jobId = 0;

SyncJob::SyncJob(const QString& since, const QString& filter, int timeout,
                 const QString& presence)
    : BaseJob(HttpVerb::Get, QStringLiteral("SyncJob-%1").arg(++jobId),
              "_matrix/client/r0/sync")
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

BaseJob::Status SyncJob::prepareResult()
{
    d.parseJson(jsonData());
    if (d.unresolvedRooms().isEmpty())
        return Success;

    qCCritical(MAIN).noquote() << "Incomplete sync response, missing rooms:"
                               << d.unresolvedRooms().join(',');
    return IncorrectResponse;
}
