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
    addParam<IfNotEmpty>(query, QStringLiteral("filter"), filter);
    addParam<IfNotEmpty>(query, QStringLiteral("set_presence"), presence);
    if (timeout >= 0)
        query.addQueryItem(QStringLiteral("timeout"), QString::number(timeout));
    addParam<IfNotEmpty>(query, QStringLiteral("since"), since);
    setRequestQuery(query);

    setMaxRetries(std::numeric_limits<int>::max());
}

SyncJob::SyncJob(const QString& since, const Filter& filter, int timeout,
                 const QString& presence)
    : SyncJob(since,
              QString::fromUtf8(QJsonDocument(toJson(filter)).toJson(QJsonDocument::Compact)),
              timeout, presence)
{}

BaseJob::Status SyncJob::prepareResult()
{
    d.parseJson(jsonData());
    if (Q_LIKELY(d.unresolvedRooms().isEmpty()))
        return Success;

    Q_ASSERT(d.unresolvedRooms().isEmpty());
    qCCritical(MAIN).noquote() << "Rooms missing after processing sync "
                                  "response, possibly a bug in SyncData: "
                               << d.unresolvedRooms().join(QLatin1Char(','));
    return IncorrectResponse;
}
