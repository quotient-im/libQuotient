/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "joining.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

namespace QMatrixClient
{
    // Converters

    QJsonObject toJson(const JoinRoomByIdJob::ThirdPartySigned& pod)
    {
        QJsonObject _json;
        addParam<>(_json, QStringLiteral("sender"), pod.sender);
        addParam<>(_json, QStringLiteral("mxid"), pod.mxid);
        addParam<>(_json, QStringLiteral("token"), pod.token);
        addParam<>(_json, QStringLiteral("signatures"), pod.signatures);
        return _json;
    }
} // namespace QMatrixClient

class JoinRoomByIdJob::Private
{
    public:
        QString roomId;
};

static const auto JoinRoomByIdJobName = QStringLiteral("JoinRoomByIdJob");

JoinRoomByIdJob::JoinRoomByIdJob(const QString& roomId, const Omittable<ThirdPartySigned>& thirdPartySigned)
    : BaseJob(HttpVerb::Post, JoinRoomByIdJobName,
        basePath % "/rooms/" % roomId % "/join")
    , d(new Private)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("third_party_signed"), thirdPartySigned);
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
    if (!json.contains("room_id"_ls))
        return { JsonParseError,
            "The key 'room_id' not found in the response" };
    d->roomId = fromJson<QString>(json.value("room_id"_ls));
    return Success;
}

namespace QMatrixClient
{
    // Converters

    QJsonObject toJson(const JoinRoomJob::Signed& pod)
    {
        QJsonObject _json;
        addParam<>(_json, QStringLiteral("sender"), pod.sender);
        addParam<>(_json, QStringLiteral("mxid"), pod.mxid);
        addParam<>(_json, QStringLiteral("token"), pod.token);
        addParam<>(_json, QStringLiteral("signatures"), pod.signatures);
        return _json;
    }

    QJsonObject toJson(const JoinRoomJob::ThirdPartySigned& pod)
    {
        QJsonObject _json;
        addParam<>(_json, QStringLiteral("signed"), pod.signedData);
        return _json;
    }
} // namespace QMatrixClient

class JoinRoomJob::Private
{
    public:
        QString roomId;
};

static const auto JoinRoomJobName = QStringLiteral("JoinRoomJob");

JoinRoomJob::JoinRoomJob(const QString& roomIdOrAlias, const Omittable<ThirdPartySigned>& thirdPartySigned)
    : BaseJob(HttpVerb::Post, JoinRoomJobName,
        basePath % "/join/" % roomIdOrAlias)
    , d(new Private)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("third_party_signed"), thirdPartySigned);
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
    if (!json.contains("room_id"_ls))
        return { JsonParseError,
            "The key 'room_id' not found in the response" };
    d->roomId = fromJson<QString>(json.value("room_id"_ls));
    return Success;
}

