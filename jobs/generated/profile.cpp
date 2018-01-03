/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "profile.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

SetDisplayNameJob::SetDisplayNameJob(const QString& userId, const QString& displayname)
    : BaseJob(HttpVerb::Put, "SetDisplayNameJob",
        basePath % "/profile/" % userId % "/displayname",
        Query { }
    )
{
    QJsonObject _data;
    if (!displayname.isEmpty())
        _data.insert("displayname", toJson(displayname));
    setRequestData(_data);
}

class GetDisplayNameJob::Private
{
    public:
        QString displayname;
};

GetDisplayNameJob::GetDisplayNameJob(const QString& userId)
    : BaseJob(HttpVerb::Get, "GetDisplayNameJob",
        basePath % "/profile/" % userId % "/displayname",
        Query { }, Data { }, false
    ), d(new Private)
{ }

GetDisplayNameJob::~GetDisplayNameJob() = default;

const QString& GetDisplayNameJob::displayname() const
{
    return d->displayname;
}

BaseJob::Status GetDisplayNameJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->displayname = fromJson<QString>(json.value("displayname"));
    return Success;
}

SetAvatarUrlJob::SetAvatarUrlJob(const QString& userId, const QString& avatarUrl)
    : BaseJob(HttpVerb::Put, "SetAvatarUrlJob",
        basePath % "/profile/" % userId % "/avatar_url",
        Query { }
    )
{
    QJsonObject _data;
    if (!avatarUrl.isEmpty())
        _data.insert("avatar_url", toJson(avatarUrl));
    setRequestData(_data);
}

class GetAvatarUrlJob::Private
{
    public:
        QString avatarUrl;
};

GetAvatarUrlJob::GetAvatarUrlJob(const QString& userId)
    : BaseJob(HttpVerb::Get, "GetAvatarUrlJob",
        basePath % "/profile/" % userId % "/avatar_url",
        Query { }, Data { }, false
    ), d(new Private)
{ }

GetAvatarUrlJob::~GetAvatarUrlJob() = default;

const QString& GetAvatarUrlJob::avatarUrl() const
{
    return d->avatarUrl;
}

BaseJob::Status GetAvatarUrlJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->avatarUrl = fromJson<QString>(json.value("avatar_url"));
    return Success;
}

class GetUserProfileJob::Private
{
    public:
        QString avatarUrl;
        QString displayname;
};

GetUserProfileJob::GetUserProfileJob(const QString& userId)
    : BaseJob(HttpVerb::Get, "GetUserProfileJob",
        basePath % "/profile/" % userId,
        Query { }, Data { }, false
    ), d(new Private)
{ }

GetUserProfileJob::~GetUserProfileJob() = default;

const QString& GetUserProfileJob::avatarUrl() const
{
    return d->avatarUrl;
}

const QString& GetUserProfileJob::displayname() const
{
    return d->displayname;
}

BaseJob::Status GetUserProfileJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->avatarUrl = fromJson<QString>(json.value("avatar_url"));
    d->displayname = fromJson<QString>(json.value("displayname"));
    return Success;
}

