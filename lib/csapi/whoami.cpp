/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "whoami.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class GetTokenOwnerJob::Private
{
    public:
        QString userId;
};

QUrl GetTokenOwnerJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/account/whoami");
}

static const auto GetTokenOwnerJobName = QStringLiteral("GetTokenOwnerJob");

GetTokenOwnerJob::GetTokenOwnerJob()
    : BaseJob(HttpVerb::Get, GetTokenOwnerJobName,
        basePath % "/account/whoami")
    , d(new Private)
{
}

GetTokenOwnerJob::~GetTokenOwnerJob() = default;

const QString& GetTokenOwnerJob::userId() const
{
    return d->userId;
}

BaseJob::Status GetTokenOwnerJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("user_id"_ls))
        return { IncorrectResponse,
            "The key 'user_id' not found in the response" };
    fromJson(json.value("user_id"_ls), d->userId);
    return Success;
}

