// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "key_backup.h"

using namespace Quotient;

PostRoomKeysVersionJob::PostRoomKeysVersionJob(const QString& algorithm, const QJsonObject& authData)
    : BaseJob(HttpVerb::Post, QStringLiteral("PostRoomKeysVersionJob"),
              makePath("/_matrix/client/v3", "/room_keys/version"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("algorithm"), algorithm);
    addParam<>(_dataJson, QStringLiteral("auth_data"), authData);
    setRequestData({ _dataJson });
    addExpectedKey("version");
}

QUrl GetRoomKeysVersionCurrentJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/room_keys/version"));
}

GetRoomKeysVersionCurrentJob::GetRoomKeysVersionCurrentJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRoomKeysVersionCurrentJob"),
              makePath("/_matrix/client/v3", "/room_keys/version"))
{
    addExpectedKey("algorithm");
    addExpectedKey("auth_data");
    addExpectedKey("count");
    addExpectedKey("etag");
    addExpectedKey("version");
}

QUrl GetRoomKeysVersionJob::makeRequestUrl(QUrl baseUrl, const QString& version)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/room_keys/version/", version));
}

GetRoomKeysVersionJob::GetRoomKeysVersionJob(const QString& version)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRoomKeysVersionJob"),
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
    : BaseJob(HttpVerb::Put, QStringLiteral("PutRoomKeysVersionJob"),
              makePath("/_matrix/client/v3", "/room_keys/version/", version))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("algorithm"), algorithm);
    addParam<>(_dataJson, QStringLiteral("auth_data"), authData);
    setRequestData({ _dataJson });
}

QUrl DeleteRoomKeysVersionJob::makeRequestUrl(QUrl baseUrl, const QString& version)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/room_keys/version/", version));
}

DeleteRoomKeysVersionJob::DeleteRoomKeysVersionJob(const QString& version)
    : BaseJob(HttpVerb::Delete, QStringLiteral("DeleteRoomKeysVersionJob"),
              makePath("/_matrix/client/v3", "/room_keys/version/", version))
{}

auto queryToPutRoomKeyBySessionId(const QString& version)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("version"), version);
    return _q;
}

PutRoomKeyBySessionIdJob::PutRoomKeyBySessionIdJob(const QString& roomId, const QString& sessionId,
                                                   const QString& version, const KeyBackupData& data)
    : BaseJob(HttpVerb::Put, QStringLiteral("PutRoomKeyBySessionIdJob"),
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
    addParam<>(_q, QStringLiteral("version"), version);
    return _q;
}

QUrl GetRoomKeyBySessionIdJob::makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                              const QString& sessionId, const QString& version)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/room_keys/keys/", roomId, "/",
                                            sessionId),
                                   queryToGetRoomKeyBySessionId(version));
}

GetRoomKeyBySessionIdJob::GetRoomKeyBySessionIdJob(const QString& roomId, const QString& sessionId,
                                                   const QString& version)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRoomKeyBySessionIdJob"),
              makePath("/_matrix/client/v3", "/room_keys/keys/", roomId, "/", sessionId),
              queryToGetRoomKeyBySessionId(version))
{}

auto queryToDeleteRoomKeyBySessionId(const QString& version)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("version"), version);
    return _q;
}

QUrl DeleteRoomKeyBySessionIdJob::makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                                 const QString& sessionId, const QString& version)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/room_keys/keys/", roomId, "/",
                                            sessionId),
                                   queryToDeleteRoomKeyBySessionId(version));
}

DeleteRoomKeyBySessionIdJob::DeleteRoomKeyBySessionIdJob(const QString& roomId,
                                                         const QString& sessionId,
                                                         const QString& version)
    : BaseJob(HttpVerb::Delete, QStringLiteral("DeleteRoomKeyBySessionIdJob"),
              makePath("/_matrix/client/v3", "/room_keys/keys/", roomId, "/", sessionId),
              queryToDeleteRoomKeyBySessionId(version))
{
    addExpectedKey("etag");
    addExpectedKey("count");
}

auto queryToPutRoomKeysByRoomId(const QString& version)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("version"), version);
    return _q;
}

PutRoomKeysByRoomIdJob::PutRoomKeysByRoomIdJob(const QString& roomId, const QString& version,
                                               const RoomKeyBackup& backupData)
    : BaseJob(HttpVerb::Put, QStringLiteral("PutRoomKeysByRoomIdJob"),
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
    addParam<>(_q, QStringLiteral("version"), version);
    return _q;
}

QUrl GetRoomKeysByRoomIdJob::makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                            const QString& version)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/room_keys/keys/", roomId),
                                   queryToGetRoomKeysByRoomId(version));
}

GetRoomKeysByRoomIdJob::GetRoomKeysByRoomIdJob(const QString& roomId, const QString& version)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRoomKeysByRoomIdJob"),
              makePath("/_matrix/client/v3", "/room_keys/keys/", roomId),
              queryToGetRoomKeysByRoomId(version))
{}

auto queryToDeleteRoomKeysByRoomId(const QString& version)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("version"), version);
    return _q;
}

QUrl DeleteRoomKeysByRoomIdJob::makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                               const QString& version)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/room_keys/keys/", roomId),
                                   queryToDeleteRoomKeysByRoomId(version));
}

DeleteRoomKeysByRoomIdJob::DeleteRoomKeysByRoomIdJob(const QString& roomId, const QString& version)
    : BaseJob(HttpVerb::Delete, QStringLiteral("DeleteRoomKeysByRoomIdJob"),
              makePath("/_matrix/client/v3", "/room_keys/keys/", roomId),
              queryToDeleteRoomKeysByRoomId(version))
{
    addExpectedKey("etag");
    addExpectedKey("count");
}

auto queryToPutRoomKeys(const QString& version)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("version"), version);
    return _q;
}

PutRoomKeysJob::PutRoomKeysJob(const QString& version, const QHash<RoomId, RoomKeyBackup>& rooms)
    : BaseJob(HttpVerb::Put, QStringLiteral("PutRoomKeysJob"),
              makePath("/_matrix/client/v3", "/room_keys/keys"), queryToPutRoomKeys(version))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("rooms"), rooms);
    setRequestData({ _dataJson });
    addExpectedKey("etag");
    addExpectedKey("count");
}

auto queryToGetRoomKeys(const QString& version)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("version"), version);
    return _q;
}

QUrl GetRoomKeysJob::makeRequestUrl(QUrl baseUrl, const QString& version)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/room_keys/keys"),
                                   queryToGetRoomKeys(version));
}

GetRoomKeysJob::GetRoomKeysJob(const QString& version)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRoomKeysJob"),
              makePath("/_matrix/client/v3", "/room_keys/keys"), queryToGetRoomKeys(version))
{
    addExpectedKey("rooms");
}

auto queryToDeleteRoomKeys(const QString& version)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("version"), version);
    return _q;
}

QUrl DeleteRoomKeysJob::makeRequestUrl(QUrl baseUrl, const QString& version)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/room_keys/keys"),
                                   queryToDeleteRoomKeys(version));
}

DeleteRoomKeysJob::DeleteRoomKeysJob(const QString& version)
    : BaseJob(HttpVerb::Delete, QStringLiteral("DeleteRoomKeysJob"),
              makePath("/_matrix/client/v3", "/room_keys/keys"), queryToDeleteRoomKeys(version))
{
    addExpectedKey("etag");
    addExpectedKey("count");
}
