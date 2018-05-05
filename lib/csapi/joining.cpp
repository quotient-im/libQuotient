/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "joining.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

namespace QMatrixClient
{
    // Converters

    QJsonObject toJson(const JoinRoomByIdJob::ThirdPartySigned& pod)
    {
        QJsonObject o;
        o.insert("sender", toJson(pod.sender));
        o.insert("mxid", toJson(pod.mxid));
        o.insert("token", toJson(pod.token));
        o.insert("signatures", toJson(pod.signatures));

        return o;
    }
} // namespace QMatrixClient

class JoinRoomByIdJob::Private
{
    public:
        QString roomId;
};

JoinRoomByIdJob::JoinRoomByIdJob(const QString& roomId, const ThirdPartySigned& thirdPartySigned)
    : BaseJob(HttpVerb::Post, "JoinRoomByIdJob",
        basePath % "/rooms/" % roomId % "/join")
    , d(new Private)
{
    QJsonObject _data;
    _data.insert("third_party_signed", toJson(thirdPartySigned));
    setRequestData(_data);
}

JoinRoomByIdJob::~JoinRoomByIdJob() = default;

const QString& JoinRoomByIdJob::roomId() const
{
    return d->roomId;
}

BaseJob::Status JoinRoomByIdJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("room_id"))
        return { JsonParseError,
            "The key 'room_id' not found in the response" };
    d->roomId = fromJson<QString>(json.value("room_id"));
    return Success;
}

namespace QMatrixClient
{
    // Converters

    QJsonObject toJson(const JoinRoomJob::Signed& pod)
    {
        QJsonObject o;
        o.insert("sender", toJson(pod.sender));
        o.insert("mxid", toJson(pod.mxid));
        o.insert("token", toJson(pod.token));
        o.insert("signatures", toJson(pod.signatures));

        return o;
    }

    QJsonObject toJson(const JoinRoomJob::ThirdPartySigned& pod)
    {
        QJsonObject o;
        o.insert("signed", toJson(pod.signedData));

        return o;
    }
} // namespace QMatrixClient

class JoinRoomJob::Private
{
    public:
        QString roomId;
};

JoinRoomJob::JoinRoomJob(const QString& roomIdOrAlias, const ThirdPartySigned& thirdPartySigned)
    : BaseJob(HttpVerb::Post, "JoinRoomJob",
        basePath % "/join/" % roomIdOrAlias)
    , d(new Private)
{
    QJsonObject _data;
    _data.insert("third_party_signed", toJson(thirdPartySigned));
    setRequestData(_data);
}

JoinRoomJob::~JoinRoomJob() = default;

const QString& JoinRoomJob::roomId() const
{
    return d->roomId;
}

BaseJob::Status JoinRoomJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("room_id"))
        return { JsonParseError,
            "The key 'room_id' not found in the response" };
    d->roomId = fromJson<QString>(json.value("room_id"));
    return Success;
}

