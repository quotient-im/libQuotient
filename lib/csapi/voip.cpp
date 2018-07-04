/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "voip.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class GetTurnServerJob::Private
{
    public:
        QJsonObject data;
};

QUrl GetTurnServerJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/voip/turnServer");
}

static const auto GetTurnServerJobName = QStringLiteral("GetTurnServerJob");

GetTurnServerJob::GetTurnServerJob()
    : BaseJob(HttpVerb::Get, GetTurnServerJobName,
        basePath % "/voip/turnServer")
    , d(new Private)
{
}

GetTurnServerJob::~GetTurnServerJob() = default;

const QJsonObject& GetTurnServerJob::data() const
{
    return d->data;
}

BaseJob::Status GetTurnServerJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("data"_ls))
        return { JsonParseError,
            "The key 'data' not found in the response" };
    d->data = fromJson<QJsonObject>(json.value("data"_ls));
    return Success;
}

