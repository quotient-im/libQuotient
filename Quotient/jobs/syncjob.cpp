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
    using namespace std::chrono_literals;
    // Add time for the request roundtrip on top of the server-side timeout specified above
    JobBackoffStrategy backoffStrategy { { defaultTimeout + 10s }, { 2s, 5s, 15s }, std::nullopt };
    if (timeout >= 0) {
        query.addQueryItem(u"timeout"_s, QString::number(timeout));
        backoffStrategy.jobTimeouts = { std::chrono::seconds(timeout / 1000 + 10) };
    } else
        backoffStrategy.jobTimeouts = { std::chrono::weeks { 3 } }; // effectively disable the timeout
    setBackoffStrategy(std::move(backoffStrategy));
    addParam<IfNotEmpty>(query, u"since"_s, since);
    setRequestQuery(query);
}

SyncJob::SyncJob(const QString& since, const Filter& filter, int timeout,
                 const QString& presence)
    : SyncJob(since,
              QString::fromUtf8(QJsonDocument(toJson(filter)).toJson(QJsonDocument::Compact)),
              timeout, presence)
{}

void SyncJob::setRawResponseProcessor(std::function<bool(const QByteArray&)> processor)
{
    rawResponseProcessor = std::move(processor);
}

BaseJob::Status SyncJob::prepareResult()
{
    // This has to come before parsing: parseJson() adds to the data parsed so far, which
    // would duplicate the rooms from the first attempt when the request is retried
    if (rawResponseProcessor && !rawResponseProcessor(rawData())) {
        // A failure that comes from the response itself would repeat on every retry and
        // keep the sync from ever progressing, so give up on its encryption data eventually
        if (++processingFailures < MaxProcessingAttempts)
            return { IncorrectResponse, tr("Could not process the end-to-end encryption data") };
        qCCritical(MAIN) << "Could not process the encryption data of the sync response after"
                         << processingFailures
                         << "attempts, continuing without it; room keys in it are lost";
    }

    d.parseJson(jsonData());
    if (Q_LIKELY(d.unresolvedRooms().isEmpty()))
        return Success;

    Q_ASSERT(d.unresolvedRooms().isEmpty());
    qCCritical(MAIN).noquote() << "Rooms missing after processing sync "
                                  "response, possibly a bug in SyncData: "
                               << d.unresolvedRooms().join(u',');
    return IncorrectResponse;
}
