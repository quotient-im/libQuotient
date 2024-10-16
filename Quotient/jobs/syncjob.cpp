// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "syncjob.h"

#include "../logging_categories_p.h"

using namespace Quotient;

static size_t jobId = 0;

SyncJob::SyncJob(const QString& since, const QString& filter, int timeout, const QString& presence)
    : BaseJob(HttpVerb::Get, "SyncJob-"_L1 + QString::number(++jobId), "_matrix/client/r0/sync")
{
    setLoggingCategory(SYNCJOB);
    QUrlQuery query;
    addParam<IfNotEmpty>(query, u"filter"_s, filter);
    addParam<IfNotEmpty>(query, u"set_presence"_s, presence);
    if (timeout >= 0)
        query.addQueryItem(u"timeout"_s, QString::number(timeout));
    addParam<IfNotEmpty>(query, u"since"_s, since);
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
                               << d.unresolvedRooms().join(u',');
    return IncorrectResponse;
}
