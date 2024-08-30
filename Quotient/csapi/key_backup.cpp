// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "key_backup.h"

using namespace Quotient;

PostRoomKeysVersionJob::PostRoomKeysVersionJob(const QString& algorithm, const QJsonObject& authData)
    : BaseJob(HttpVerb::Post, u"PostRoomKeysVersionJob"_s,
              makePath("/_matrix/client/v3", "/room_keys/version"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "algorithm"_L1, algorithm);
    addParam<>(_dataJson, "auth_data"_L1, authData);
    setRequestData({ _dataJson });
    addExpectedKey("version");
}

QUrl GetRoomKeysVersionCurrentJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/room_keys/version"));
}

GetRoomKeysVersionCurrentJob::GetRoomKeysVersionCurrentJob()
    : BaseJob(HttpVerb::Get, u"GetRoomKeysVersionCurrentJob"_s,
              makePath("/_matrix/client/v3", "/room_keys/version"))
{
    addExpectedKey("algorithm");
    addExpectedKey("auth_data");
    addExpectedKey("count");
    addExpectedKey("etag");
    addExpectedKey("version");
}

QUrl GetRoomKeysVersionJob::makeRequestUrl(const HomeserverData& hsData, const QString& version)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/room_keys/version/", version));
}

GetRoomKeysVersionJob::GetRoomKeysVersionJob(const QString& version)
    : BaseJob(HttpVerb::Get, u"GetRoomKeysVersionJob"_s,
              makePath("/_matrix/client/v3", "/room_keys/version/", version))
{
    addExpectedKey("algorithm");
    addExpectedKey("auth_data");
    addExpectedKey("count");
    addExpectedKey("etag");
    addExpectedKey("version");
}

PutRoomKeysVersionJob::PutRoomKeysVersionJob(const QString& version, const QString& algorithm,
                                             const QJsonObject& authData)
    : BaseJob(HttpVerb::Put, u"PutRoomKeysVersionJob"_s,
              makePath("/_matrix/client/v3", "/room_keys/version/", version))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "algorithm"_L1, algorithm);
    addParam<>(_dataJson, "auth_data"_L1, authData);
    setRequestData({ _dataJson });
}

QUrl DeleteRoomKeysVersionJob::makeRequestUrl(const HomeserverData& hsData, const QString& version)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/room_keys/version/", version));
}

DeleteRoomKeysVersionJob::DeleteRoomKeysVersionJob(const QString& version)
    : BaseJob(HttpVerb::Delete, u"DeleteRoomKeysVersionJob"_s,
              makePath("/_matrix/client/v3", "/room_keys/version/", version))
{}

auto queryToPutRoomKeyBySessionId(const QString& version)
{
    QUrlQuery _q;
    addParam<>(_q, u"version"_s, version);
    return _q;
}

PutRoomKeyBySessionIdJob::PutRoomKeyBySessionIdJob(const QString& roomId, const QString& sessionId,
                                                   const QString& version, const KeyBackupData& data)
    : BaseJob(HttpVerb::Put, u"PutRoomKeyBySessionIdJob"_s,
              makePath("/_matrix/client/v3", "/room_keys/keys/", roomId, "/", sessionId),
              queryToPutRoomKeyBySessionId(version))
{
    setRequestData({ toJson(data) });
    addExpectedKey("etag");
    addExpectedKey("count");
}

auto queryToGetRoomKeyBySessionId(const QString& version)
{
    QUrlQuery _q;
    addParam<>(_q, u"version"_s, version);
    return _q;
}

QUrl GetRoomKeyBySessionIdJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId,
                                              const QString& sessionId, const QString& version)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/room_keys/keys/", roomId, "/",
                                            sessionId),
                                   queryToGetRoomKeyBySessionId(version));
}

GetRoomKeyBySessionIdJob::GetRoomKeyBySessionIdJob(const QString& roomId, const QString& sessionId,
                                                   const QString& version)
    : BaseJob(HttpVerb::Get, u"GetRoomKeyBySessionIdJob"_s,
              makePath("/_matrix/client/v3", "/room_keys/keys/", roomId, "/", sessionId),
              queryToGetRoomKeyBySessionId(version))
{}

auto queryToDeleteRoomKeyBySessionId(const QString& version)
{
    QUrlQuery _q;
    addParam<>(_q, u"version"_s, version);
    return _q;
}

QUrl DeleteRoomKeyBySessionIdJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId,
                                                 const QString& sessionId, const QString& version)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/room_keys/keys/", roomId, "/",
                                            sessionId),
                                   queryToDeleteRoomKeyBySessionId(version));
}

DeleteRoomKeyBySessionIdJob::DeleteRoomKeyBySessionIdJob(const QString& roomId,
                                                         const QString& sessionId,
                                                         const QString& version)
    : BaseJob(HttpVerb::Delete, u"DeleteRoomKeyBySessionIdJob"_s,
              makePath("/_matrix/client/v3", "/room_keys/keys/", roomId, "/", sessionId),
              queryToDeleteRoomKeyBySessionId(version))
{
    addExpectedKey("etag");
    addExpectedKey("count");
}

auto queryToPutRoomKeysByRoomId(const QString& version)
{
    QUrlQuery _q;
    addParam<>(_q, u"version"_s, version);
    return _q;
}

PutRoomKeysByRoomIdJob::PutRoomKeysByRoomIdJob(const QString& roomId, const QString& version,
                                               const RoomKeyBackup& backupData)
    : BaseJob(HttpVerb::Put, u"PutRoomKeysByRoomIdJob"_s,
              makePath("/_matrix/client/v3", "/room_keys/keys/", roomId),
              queryToPutRoomKeysByRoomId(version))
{
    setRequestData({ toJson(backupData) });
    addExpectedKey("etag");
    addExpectedKey("count");
}

auto queryToGetRoomKeysByRoomId(const QString& version)
{
    QUrlQuery _q;
    addParam<>(_q, u"version"_s, version);
    return _q;
}

QUrl GetRoomKeysByRoomIdJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId,
                                            const QString& version)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/room_keys/keys/", roomId),
                                   queryToGetRoomKeysByRoomId(version));
}

GetRoomKeysByRoomIdJob::GetRoomKeysByRoomIdJob(const QString& roomId, const QString& version)
    : BaseJob(HttpVerb::Get, u"GetRoomKeysByRoomIdJob"_s,
              makePath("/_matrix/client/v3", "/room_keys/keys/", roomId),
              queryToGetRoomKeysByRoomId(version))
{}

auto queryToDeleteRoomKeysByRoomId(const QString& version)
{
    QUrlQuery _q;
    addParam<>(_q, u"version"_s, version);
    return _q;
}

QUrl DeleteRoomKeysByRoomIdJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId,
                                               const QString& version)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/room_keys/keys/", roomId),
                                   queryToDeleteRoomKeysByRoomId(version));
}

DeleteRoomKeysByRoomIdJob::DeleteRoomKeysByRoomIdJob(const QString& roomId, const QString& version)
    : BaseJob(HttpVerb::Delete, u"DeleteRoomKeysByRoomIdJob"_s,
              makePath("/_matrix/client/v3", "/room_keys/keys/", roomId),
              queryToDeleteRoomKeysByRoomId(version))
{
    addExpectedKey("etag");
    addExpectedKey("count");
}

auto queryToPutRoomKeys(const QString& version)
{
    QUrlQuery _q;
    addParam<>(_q, u"version"_s, version);
    return _q;
}

PutRoomKeysJob::PutRoomKeysJob(const QString& version, const QHash<RoomId, RoomKeyBackup>& rooms)
    : BaseJob(HttpVerb::Put, u"PutRoomKeysJob"_s, makePath("/_matrix/client/v3", "/room_keys/keys"),
              queryToPutRoomKeys(version))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "rooms"_L1, rooms);
    setRequestData({ _dataJson });
    addExpectedKey("etag");
    addExpectedKey("count");
}

auto queryToGetRoomKeys(const QString& version)
{
    QUrlQuery _q;
    addParam<>(_q, u"version"_s, version);
    return _q;
}

QUrl GetRoomKeysJob::makeRequestUrl(const HomeserverData& hsData, const QString& version)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/room_keys/keys"),
                                   queryToGetRoomKeys(version));
}

GetRoomKeysJob::GetRoomKeysJob(const QString& version)
    : BaseJob(HttpVerb::Get, u"GetRoomKeysJob"_s, makePath("/_matrix/client/v3", "/room_keys/keys"),
              queryToGetRoomKeys(version))
{
    addExpectedKey("rooms");
}

auto queryToDeleteRoomKeys(const QString& version)
{
    QUrlQuery _q;
    addParam<>(_q, u"version"_s, version);
    return _q;
}

QUrl DeleteRoomKeysJob::makeRequestUrl(const HomeserverData& hsData, const QString& version)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/room_keys/keys"),
                                   queryToDeleteRoomKeys(version));
}

DeleteRoomKeysJob::DeleteRoomKeysJob(const QString& version)
    : BaseJob(HttpVerb::Delete, u"DeleteRoomKeysJob"_s,
              makePath("/_matrix/client/v3", "/room_keys/keys"), queryToDeleteRoomKeys(version))
{
    addExpectedKey("etag");
    addExpectedKey("count");
}
