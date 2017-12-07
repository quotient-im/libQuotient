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

GetTokenOwnerJob::GetTokenOwnerJob()
    : BaseJob(HttpVerb::Get, "GetTokenOwnerJob",
        basePath % "/account/whoami",
        Query { }
    ), d(new Private)
{ }

GetTokenOwnerJob::~GetTokenOwnerJob()
{
    delete d;
}

const QString& GetTokenOwnerJob::userId() const
{
    return d->userId;
}

BaseJob::Status GetTokenOwnerJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("user_id"))
        return { JsonParseError,
            "The key 'user_id' not found in the response" };
    d->userId = fromJson<QString>(json.value("user_id"));
    return Success;
}

