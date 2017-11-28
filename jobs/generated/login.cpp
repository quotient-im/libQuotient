/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#include "login.h"

#include "converters.h"
#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class LoginJob::Private
{
    public:
        QString user_id;
        QString access_token;
        QString home_server;
        QString device_id;
        
};

LoginJob::LoginJob(QString type, QString user, QString medium, QString address, QString password, QString token, QString device_id, QString initial_device_display_name)
    : BaseJob(HttpVerb::Post, "LoginJob",
        basePath % "/login",
        Query { }, Data { }, false
    ), d(new Private)
{
    Data _data;
    _data.insert("type", toJson(type));
    if (!user.isEmpty())
        _data.insert("user", toJson(user));
    if (!medium.isEmpty())
        _data.insert("medium", toJson(medium));
    if (!address.isEmpty())
        _data.insert("address", toJson(address));
    if (!password.isEmpty())
        _data.insert("password", toJson(password));
    if (!token.isEmpty())
        _data.insert("token", toJson(token));
    if (!device_id.isEmpty())
        _data.insert("device_id", toJson(device_id));
    if (!initial_device_display_name.isEmpty())
        _data.insert("initial_device_display_name", toJson(initial_device_display_name));
    setRequestData(_data);
}

LoginJob::~LoginJob()
{
    delete d;
}

const QString& LoginJob::user_id() const
{
    return d->user_id;
}

const QString& LoginJob::access_token() const
{
    return d->access_token;
}

const QString& LoginJob::home_server() const
{
    return d->home_server;
}

const QString& LoginJob::device_id() const
{
    return d->device_id;
}

BaseJob::Status LoginJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    
    d->user_id = fromJson<QString>(json.value("user_id"));

    d->access_token = fromJson<QString>(json.value("access_token"));

    d->home_server = fromJson<QString>(json.value("home_server"));

    d->device_id = fromJson<QString>(json.value("device_id"));

    return Success;
}

