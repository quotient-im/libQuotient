/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "account-data.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

SetAccountDataJob::SetAccountDataJob(const QString& userId, const QString& type,
                                     const QJsonObject& content)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetAccountDataJob"),
              basePath % "/user/" % userId % "/account_data/" % type)
{
    setRequestData(Data(toJson(content)));
}

QUrl GetAccountDataJob::makeRequestUrl(QUrl baseUrl, const QString& userId,
                                       const QString& type)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/user/" % userId
                                       % "/account_data/" % type);
}

GetAccountDataJob::GetAccountDataJob(const QString& userId, const QString& type)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetAccountDataJob"),
              basePath % "/user/" % userId % "/account_data/" % type)
{}

SetAccountDataPerRoomJob::SetAccountDataPerRoomJob(const QString& userId,
                                                   const QString& roomId,
                                                   const QString& type,
                                                   const QJsonObject& content)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetAccountDataPerRoomJob"),
              basePath % "/user/" % userId % "/rooms/" % roomId
                  % "/account_data/" % type)
{
    setRequestData(Data(toJson(content)));
}

QUrl GetAccountDataPerRoomJob::makeRequestUrl(QUrl baseUrl,
                                              const QString& userId,
                                              const QString& roomId,
                                              const QString& type)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/user/" % userId % "/rooms/"
                                       % roomId % "/account_data/" % type);
}

GetAccountDataPerRoomJob::GetAccountDataPerRoomJob(const QString& userId,
                                                   const QString& roomId,
                                                   const QString& type)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetAccountDataPerRoomJob"),
              basePath % "/user/" % userId % "/rooms/" % roomId
                  % "/account_data/" % type)
{}
