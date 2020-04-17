/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "filter.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class DefineFilterJob::Private {
public:
    QString filterId;
};

DefineFilterJob::DefineFilterJob(const QString& userId, const Filter& filter)
    : BaseJob(HttpVerb::Post, QStringLiteral("DefineFilterJob"),
              basePath % "/user/" % userId % "/filter")
    , d(new Private)
{
    setRequestData(Data(toJson(filter)));
}

DefineFilterJob::~DefineFilterJob() = default;

const QString& DefineFilterJob::filterId() const { return d->filterId; }

BaseJob::Status DefineFilterJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("filter_id"_ls))
        return { IncorrectResponse,
                 "The key 'filter_id' not found in the response" };
    fromJson(json.value("filter_id"_ls), d->filterId);

    return Success;
}

class GetFilterJob::Private {
public:
    Filter data;
};

QUrl GetFilterJob::makeRequestUrl(QUrl baseUrl, const QString& userId,
                                  const QString& filterId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), basePath % "/user/"
                                                           % userId % "/filter/"
                                                           % filterId);
}

GetFilterJob::GetFilterJob(const QString& userId, const QString& filterId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetFilterJob"),
              basePath % "/user/" % userId % "/filter/" % filterId)
    , d(new Private)
{}

GetFilterJob::~GetFilterJob() = default;

const Filter& GetFilterJob::data() const { return d->data; }

BaseJob::Status GetFilterJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);

    return Success;
}
