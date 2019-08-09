/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "joining.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

// Converters
namespace Quotient
{

template <>
struct JsonObjectConverter<JoinRoomByIdJob::ThirdPartySigned>
{
    static void dumpTo(QJsonObject& jo,
                       const JoinRoomByIdJob::ThirdPartySigned& pod)
    {
        addParam<>(jo, QStringLiteral("sender"), pod.sender);
        addParam<>(jo, QStringLiteral("mxid"), pod.mxid);
        addParam<>(jo, QStringLiteral("token"), pod.token);
        addParam<>(jo, QStringLiteral("signatures"), pod.signatures);
    }
};

} // namespace Quotient

class JoinRoomByIdJob::Private
{
public:
    QString roomId;
};

static const auto JoinRoomByIdJobName = QStringLiteral("JoinRoomByIdJob");

JoinRoomByIdJob::JoinRoomByIdJob(
    const QString& roomId, const Omittable<ThirdPartySigned>& thirdPartySigned)
    : BaseJob(HttpVerb::Post, JoinRoomByIdJobName,
              basePath % "/rooms/" % roomId % "/join")
    , d(new Private)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("third_party_signed"),
                         thirdPartySigned);
    setRequestData(_data);
}

JoinRoomByIdJob::~JoinRoomByIdJob() = default;

const QString& JoinRoomByIdJob::roomId() const { return d->roomId; }

BaseJob::Status JoinRoomByIdJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("room_id"_ls))
        return { IncorrectResponse,
                 "The key 'room_id' not found in the response" };
    fromJson(json.value("room_id"_ls), d->roomId);

    return Success;
}

// Converters
namespace Quotient
{

template <>
struct JsonObjectConverter<JoinRoomJob::Signed>
{
    static void dumpTo(QJsonObject& jo, const JoinRoomJob::Signed& pod)
    {
        addParam<>(jo, QStringLiteral("sender"), pod.sender);
        addParam<>(jo, QStringLiteral("mxid"), pod.mxid);
        addParam<>(jo, QStringLiteral("token"), pod.token);
        addParam<>(jo, QStringLiteral("signatures"), pod.signatures);
    }
};

template <>
struct JsonObjectConverter<JoinRoomJob::ThirdPartySigned>
{
    static void dumpTo(QJsonObject& jo, const JoinRoomJob::ThirdPartySigned& pod)
    {
        addParam<>(jo, QStringLiteral("signed"), pod.signedData);
    }
};

} // namespace Quotient

class JoinRoomJob::Private
{
public:
    QString roomId;
};

BaseJob::Query queryToJoinRoom(const QStringList& serverName)
{
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("server_name"), serverName);
    return _q;
}

static const auto JoinRoomJobName = QStringLiteral("JoinRoomJob");

JoinRoomJob::JoinRoomJob(const QString& roomIdOrAlias,
                         const QStringList& serverName,
                         const Omittable<ThirdPartySigned>& thirdPartySigned)
    : BaseJob(HttpVerb::Post, JoinRoomJobName,
              basePath % "/join/" % roomIdOrAlias, queryToJoinRoom(serverName))
    , d(new Private)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("third_party_signed"),
                         thirdPartySigned);
    setRequestData(_data);
}

JoinRoomJob::~JoinRoomJob() = default;

const QString& JoinRoomJob::roomId() const { return d->roomId; }

BaseJob::Status JoinRoomJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("room_id"_ls))
        return { IncorrectResponse,
                 "The key 'room_id' not found in the response" };
    fromJson(json.value("room_id"_ls), d->roomId);

    return Success;
}
