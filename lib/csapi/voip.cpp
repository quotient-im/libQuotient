/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "voip.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

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
    : BaseJob(HttpVerb::Get, GetTurnServerJobName, basePath % "/voip/turnServer")
    , d(new Private)
{}

GetTurnServerJob::~GetTurnServerJob() = default;

const QJsonObject& GetTurnServerJob::data() const { return d->data; }

BaseJob::Status GetTurnServerJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);
    return Success;
}
