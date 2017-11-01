/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#include "profile.h"

#include "converters.h"
#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

SetDisplayNameJob::SetDisplayNameJob(QString userId, QString displayname)
    : BaseJob(HttpVerb::Put, "SetDisplayNameJob",
        basePath % "/profile/" % userId % "/displayname",
        Query { },
        Data {
            { "displayname", toJson(displayname) }
        }
    )
{ }

class GetDisplayNameJob::Private
{
    public:
        QString displayname;
        
};

GetDisplayNameJob::GetDisplayNameJob(QString userId)
    : BaseJob(HttpVerb::Get, "GetDisplayNameJob",
        basePath % "/profile/" % userId % "/displayname",
        Query { },
        Data { }
    ), d(new Private)
{ }

GetDisplayNameJob::~GetDisplayNameJob()
{
    delete d;
}

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

SetAvatarUrlJob::SetAvatarUrlJob(QString userId, QString avatar_url)
    : BaseJob(HttpVerb::Put, "SetAvatarUrlJob",
        basePath % "/profile/" % userId % "/avatar_url",
        Query { },
        Data {
            { "avatar_url", toJson(avatar_url) }
        }
    )
{ }

class GetAvatarUrlJob::Private
{
    public:
        QString avatar_url;
        
};

GetAvatarUrlJob::GetAvatarUrlJob(QString userId)
    : BaseJob(HttpVerb::Get, "GetAvatarUrlJob",
        basePath % "/profile/" % userId % "/avatar_url",
        Query { },
        Data { }
    ), d(new Private)
{ }

GetAvatarUrlJob::~GetAvatarUrlJob()
{
    delete d;
}

const QString& GetAvatarUrlJob::avatar_url() const
{
    return d->avatar_url;
}

BaseJob::Status GetAvatarUrlJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    
    d->avatar_url = fromJson<QString>(json.value("avatar_url"));

    return Success;
}

class GetUserProfileJob::Private
{
    public:
        QString avatar_url;
        QString displayname;
        
};

GetUserProfileJob::GetUserProfileJob(QString userId)
    : BaseJob(HttpVerb::Get, "GetUserProfileJob",
        basePath % "/profile/" % userId,
        Query { },
        Data { }
    ), d(new Private)
{ }

GetUserProfileJob::~GetUserProfileJob()
{
    delete d;
}

const QString& GetUserProfileJob::avatar_url() const
{
    return d->avatar_url;
}

const QString& GetUserProfileJob::displayname() const
{
    return d->displayname;
}

BaseJob::Status GetUserProfileJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    
    d->avatar_url = fromJson<QString>(json.value("avatar_url"));

    d->displayname = fromJson<QString>(json.value("displayname"));

    return Success;
}

