/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "account-data.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

SetAccountDataJob::SetAccountDataJob(const QString& userId, const QString& type, const QJsonObject& content)
    : BaseJob(HttpVerb::Put, "SetAccountDataJob",
        basePath % "/user/" % userId % "/account_data/" % type)
{
    setRequestData(Data(content));
}

SetAccountDataPerRoomJob::SetAccountDataPerRoomJob(const QString& userId, const QString& roomId, const QString& type, const QJsonObject& content)
    : BaseJob(HttpVerb::Put, "SetAccountDataPerRoomJob",
        basePath % "/user/" % userId % "/rooms/" % roomId % "/account_data/" % type)
{
    setRequestData(Data(content));
}

