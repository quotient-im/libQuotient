/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "account-data.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

static const auto SetAccountDataJobName = QStringLiteral("SetAccountDataJob");

SetAccountDataJob::SetAccountDataJob(const QString& userId, const QString& type,
                                     const QJsonObject& content)
    : BaseJob(HttpVerb::Put, SetAccountDataJobName,
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

static const auto GetAccountDataJobName = QStringLiteral("GetAccountDataJob");

GetAccountDataJob::GetAccountDataJob(const QString& userId, const QString& type)
    : BaseJob(HttpVerb::Get, GetAccountDataJobName,
              basePath % "/user/" % userId % "/account_data/" % type)
{}

static const auto SetAccountDataPerRoomJobName =
    QStringLiteral("SetAccountDataPerRoomJob");

SetAccountDataPerRoomJob::SetAccountDataPerRoomJob(const QString& userId,
                                                   const QString& roomId,
                                                   const QString& type,
                                                   const QJsonObject& content)
    : BaseJob(HttpVerb::Put, SetAccountDataPerRoomJobName,
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

static const auto GetAccountDataPerRoomJobName =
    QStringLiteral("GetAccountDataPerRoomJob");

GetAccountDataPerRoomJob::GetAccountDataPerRoomJob(const QString& userId,
                                                   const QString& roomId,
                                                   const QString& type)
    : BaseJob(HttpVerb::Get, GetAccountDataPerRoomJobName,
              basePath % "/user/" % userId % "/rooms/" % roomId
                  % "/account_data/" % type)
{}
