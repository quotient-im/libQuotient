/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "versions.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client");

class GetVersionsJob::Private
{
    public:
        QVector<QString> versions;
};

GetVersionsJob::GetVersionsJob()
    : BaseJob(HttpVerb::Get, "GetVersionsJob",
        basePath % "/versions", false)
    , d(new Private)
{
}

GetVersionsJob::~GetVersionsJob() = default;

const QVector<QString>& GetVersionsJob::versions() const
{
    return d->versions;
}

BaseJob::Status GetVersionsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->versions = fromJson<QVector<QString>>(json.value("versions"));
    return Success;
}

