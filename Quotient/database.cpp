// SPDX-FileCopyrightText: 2021 Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "database.h"

#include "connection.h"
#include "logging_categories_p.h"

#include "e2ee/qolmaccount.h"
#include "e2ee/qolminboundsession.h"
#include "e2ee/qolmoutboundsession.h"
#include "e2ee/qolmsession.h"
#include "e2ee/cryptoutils.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

using namespace Quotient;

Database::Database(const QString& userId, const QString& deviceId,
                   PicklingKey&& picklingKey)
    : m_userId(userId)
    , m_deviceId(deviceId)
    , m_picklingKey(std::move(picklingKey))
{
    auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                        "Quotient_"_L1 + m_userId);
    auto dbDir = m_userId;
    dbDir.replace(u':', u'_');
    const QString databasePath{ QStandardPaths::writableLocation(
                                    QStandardPaths::AppDataLocation)
                                % u'/' % dbDir };
    QDir(databasePath).mkpath("."_L1);
    db.setDatabaseName(databasePath + "/quotient_%1.db3"_L1.arg(m_deviceId));
    db.open(); // Further accessed via database()

    switch(version()) {
    case 0: migrateTo1(); [[fallthrough]];
    case 1: migrateTo2(); [[fallthrough]];
    case 2: migrateTo3(); [[fallthrough]];
    case 3: migrateTo4(); [[fallthrough]];
    case 4: migrateTo5(); [[fallthrough]];
    case 5: migrateTo6(); [[fallthrough]];
    case 6: migrateTo7(); [[fallthrough]];
    case 7: migrateTo8(); [[fallthrough]];
    case 8: migrateTo9(); [[fallthrough]];
    case 9: migrateTo10();
    }
}

int Database::version()
{
    auto query = execute(QStringLiteral("PRAGMA user_version;"));
    if (query.next()) {
        bool ok = false;
        int value = query.value(0).toInt(&ok);
        qCDebug(DATABASE) << "Database version" << value;
        if (ok)
            return value;
    } else {
        qCritical(DATABASE) << "Failed to check database version";
    }
    return -1;
}

QSqlQuery Database::execute(const QString& queryString)
{
    QSqlQuery query(queryString, database());
    if (query.lastError().type() != QSqlError::NoError) {
        qCritical(DATABASE) << "Failed to execute query";
        qCritical(DATABASE) << query.lastQuery();
        qCritical(DATABASE) << query.lastError();
    }
    return query;
}

void Database::execute(QSqlQuery& query)
{
    if (!query.exec()) {
        qCritical(DATABASE) << "Failed to execute query";
        qCritical(DATABASE) << query.lastQuery();
        qCritical(DATABASE) << query.lastError();
    }
}

void Database::transaction()
{
    database().transaction();
}

void Database::commit()
{
    database().commit();
}

void Database::migrateTo1()
{
    qCDebug(DATABASE) << "Migrating database to version 1";
    transaction();
    execute(QStringLiteral("CREATE TABLE accounts (pickle TEXT);"));
    execute(QStringLiteral("CREATE TABLE olm_sessions (senderKey TEXT, sessionId TEXT, pickle TEXT);"));
    execute(QStringLiteral("CREATE TABLE inbound_megolm_sessions (roomId TEXT, senderKey TEXT, sessionId TEXT, pickle TEXT);"));
    execute(QStringLiteral("CREATE TABLE outbound_megolm_sessions (roomId TEXT, senderKey TEXT, sessionId TEXT, pickle TEXT);"));
    execute(QStringLiteral("CREATE TABLE group_session_record_index (roomId TEXT, sessionId TEXT, i INTEGER, eventId TEXT, ts INTEGER);"));
    execute(QStringLiteral("CREATE TABLE tracked_users (matrixId TEXT);"));
    execute(QStringLiteral("CREATE TABLE outdated_users (matrixId TEXT);"));
    execute(QStringLiteral("CREATE TABLE tracked_devices (matrixId TEXT, deviceId TEXT, curveKeyId TEXT, curveKey TEXT, edKeyId TEXT, edKey TEXT);"));

    execute(QStringLiteral("PRAGMA user_version = 1;"));
    commit();
}

void Database::migrateTo2()
{
    qCDebug(DATABASE) << "Migrating database to version 2";
    transaction();

    execute(QStringLiteral("ALTER TABLE inbound_megolm_sessions ADD ed25519Key TEXT"));
    execute(QStringLiteral("ALTER TABLE olm_sessions ADD lastReceived TEXT"));

    // Add indexes for improving queries speed on larger database
    execute(QStringLiteral("CREATE INDEX sessions_session_idx ON olm_sessions(sessionId)"));
    execute(QStringLiteral("CREATE INDEX outbound_room_idx ON outbound_megolm_sessions(roomId)"));
    execute(QStringLiteral("CREATE INDEX inbound_room_idx ON inbound_megolm_sessions(roomId)"));
    execute(QStringLiteral("CREATE INDEX group_session_idx ON group_session_record_index(roomId, sessionId, i)"));
    execute(QStringLiteral("PRAGMA user_version = 2;"));
    commit();
}

void Database::migrateTo3()
{
    qCDebug(DATABASE) << "Migrating database to version 3";
    transaction();

    execute(QStringLiteral("CREATE TABLE inbound_megolm_sessions_temp AS SELECT roomId, sessionId, pickle FROM inbound_megolm_sessions;"));
    execute(QStringLiteral("DROP TABLE inbound_megolm_sessions;"));
    execute(QStringLiteral("ALTER TABLE inbound_megolm_sessions_temp RENAME TO inbound_megolm_sessions;"));
    execute(QStringLiteral("ALTER TABLE inbound_megolm_sessions ADD olmSessionId TEXT;"));
    execute(QStringLiteral("ALTER TABLE inbound_megolm_sessions ADD senderId TEXT;"));
    execute(QStringLiteral("PRAGMA user_version = 3;"));
    commit();
}

void Database::migrateTo4()
{
    qCDebug(DATABASE) << "Migrating database to version 4";
    transaction();

    execute(QStringLiteral("CREATE TABLE sent_megolm_sessions (roomId TEXT, userId TEXT, deviceId TEXT, identityKey TEXT, sessionId TEXT, i INTEGER);"));
    execute(QStringLiteral("ALTER TABLE outbound_megolm_sessions ADD creationTime TEXT;"));
    execute(QStringLiteral("ALTER TABLE outbound_megolm_sessions ADD messageCount INTEGER;"));
    execute(QStringLiteral("PRAGMA user_version = 4;"));
    commit();
}

void Database::migrateTo5()
{
    qCDebug(DATABASE) << "Migrating database to version 5";
    transaction();

    execute(QStringLiteral("ALTER TABLE tracked_devices ADD verified BOOL;"));
    execute(QStringLiteral("PRAGMA user_version = 5"));
    commit();
}

void Database::migrateTo6()
{
    qCDebug(DATABASE) << "Migrating database to version 6";
    transaction();

    execute(QStringLiteral("CREATE TABLE encrypted (name TEXT, cipher TEXT, iv TEXT);"));
    execute(QStringLiteral("PRAGMA user_version = 6"));
    commit();
}

void Database::migrateTo7()
{
    qCDebug(DATABASE) << "Migrating database to version 7";
    transaction();
    execute(QStringLiteral("CREATE TABLE master_keys (userId TEXT, key TEXT, verified INTEGER);"));
    execute(QStringLiteral("CREATE TABLE self_signing_keys (userId TEXT, key TEXT);"));
    execute(QStringLiteral("CREATE TABLE user_signing_keys (userId TEXT, key TEXT);"));
    execute(QStringLiteral("INSERT INTO outdated_users SELECT * FROM tracked_users;"));
    execute(QStringLiteral("ALTER TABLE tracked_devices ADD selfVerified INTEGER;"));
    execute(QStringLiteral("PRAGMA user_version = 7;"));

    commit();
}

void Database::migrateTo8()
{
    qCDebug(DATABASE) << "Migrating database to version 8";
    transaction();

    execute(QStringLiteral("ALTER TABLE inbound_megolm_sessions ADD senderKey TEXT;"));
    auto query = prepareQuery("SELECT sessionId, olmSessionId FROM inbound_megolm_sessions;"_L1);
    execute(query);
    while (query.next()) {
        if (query.value("olmSessionId"_L1).toString().startsWith("BACKUP"_L1)) {
            continue;
        }
        auto senderKeyQuery = prepareQuery("SELECT senderKey FROM olm_sessions WHERE sessionId=:olmSessionId;"_L1);
        senderKeyQuery.bindValue(":olmSessionId"_L1, query.value("olmSessionId"_L1).toByteArray());
        execute(senderKeyQuery);
        if (!senderKeyQuery.next()) {
            continue;
        }
        auto updateQuery = prepareQuery("UPDATE inbound_megolm_sessions SET senderKey=:senderKey WHERE sessionId=:sessionId;"_L1);
        updateQuery.bindValue(":sessionId"_L1, query.value("sessionId"_L1).toByteArray());
        updateQuery.bindValue(":senderKey"_L1, senderKeyQuery.value("senderKey"_L1).toByteArray());

        execute(updateQuery);
    }
    execute(QStringLiteral("PRAGMA user_version = 8;"));
    commit();
}

void Database::migrateTo9()
{
    qCDebug(DATABASE) << "Migrating database to version 9";
    transaction();

    auto query = prepareQuery("SELECT curveKey FROM tracked_devices WHERE matrixId=:matrixId AND deviceId=:deviceId;"_L1);
    query.bindValue(":matrixId"_L1, m_userId);
    query.bindValue(":deviceId"_L1, m_deviceId);
    execute(query);
    if (!query.next()) {
        return;
    }
    auto curveKey = query.value("curveKey"_L1).toByteArray();
    query = prepareQuery("UPDATE inbound_megolm_sessions SET senderKey=:senderKey WHERE olmSessionId=:self;"_L1);
    query.bindValue(":senderKey"_L1, curveKey);
    query.bindValue(":self"_L1, QByteArrayLiteral("SELF"));
    execute(QStringLiteral("PRAGMA user_version = 9;"));
    execute(query);
    commit();
}

void Database::migrateTo10()
{
    qCDebug(DATABASE) << "Migrating database to version 10";

    transaction();

    execute(QStringLiteral("ALTER TABLE inbound_megolm_sessions ADD senderClaimedEd25519Key TEXT;"));

    auto query = prepareQuery("SELECT DISTINCT senderKey FROM inbound_megolm_sessions;"_L1);
    execute(query);

    QStringList keys;
    while (query.next()) {
        keys += query.value("senderKey"_L1).toString();
    }
    for (const auto& key : keys) {
        auto edKeyQuery = prepareQuery("SELECT edKey FROM tracked_devices WHERE curveKey=:curveKey;"_L1);
        edKeyQuery.bindValue(":curveKey"_L1, key);
        execute(edKeyQuery);
        if (!edKeyQuery.next()) {
            continue;
        }
        const auto &edKey = edKeyQuery.value("edKey"_L1).toByteArray();

        auto updateQuery = prepareQuery("UPDATE inbound_megolm_sessions SET senderClaimedEd25519Key=:senderClaimedEd25519Key WHERE senderKey=:senderKey;"_L1);
        updateQuery.bindValue(":senderKey"_L1, key.toLatin1());
        updateQuery.bindValue(":senderClaimedEd25519Key"_L1, edKey);
        execute(updateQuery);
    }

    execute(QStringLiteral("pragma user_version = 10"));
    commit();

}

void Database::storeOlmAccount(const QOlmAccount& olmAccount)
{
    auto deleteQuery = prepareQuery(QStringLiteral("DELETE FROM accounts;"));
    auto query = prepareQuery(QStringLiteral("INSERT INTO accounts(pickle) VALUES(:pickle);"));
    query.bindValue(":pickle"_L1, olmAccount.pickle(m_picklingKey));
    transaction();
    execute(deleteQuery);
    execute(query);
    commit();
}

std::optional<OlmErrorCode> Database::setupOlmAccount(QOlmAccount& olmAccount)
{
    auto query = prepareQuery(QStringLiteral("SELECT pickle FROM accounts;"));
    execute(query);
    if (query.next())
        return olmAccount.unpickle(
            query.value(QStringLiteral("pickle")).toByteArray(), m_picklingKey);

    olmAccount.setupNewAccount();
    return {};
}

void Database::clear()
{
    // SQLite driver only supports one query at a time, so feed them one by one
    transaction();
    for (auto&& q : { QStringLiteral("DELETE FROM accounts;"), // @clang-format: one per line, plz
                      QStringLiteral("DELETE FROM olm_sessions;"),
                      QStringLiteral("DELETE FROM inbound_megolm_sessions;"),
                      QStringLiteral("DELETE FROM group_session_record_index;"),
                      QStringLiteral("DELETE FROM master_keys;"),
                      QStringLiteral("DELETE FROM self_signing_keys;"),
                      QStringLiteral("DELETE FROM user_signing_keys;") })
        execute(q);
    commit();

}

void Database::saveOlmSession(const QByteArray& senderKey,
                              const QOlmSession& session,
                              const QDateTime& timestamp)
{
    auto query = prepareQuery(QStringLiteral("INSERT INTO olm_sessions(senderKey, sessionId, pickle, lastReceived) VALUES(:senderKey, :sessionId, :pickle, :lastReceived);"));
    query.bindValue(":senderKey"_L1, senderKey);
    query.bindValue(":sessionId"_L1, session.sessionId());
    query.bindValue(":pickle"_L1, session.pickle(m_picklingKey));
    query.bindValue(":lastReceived"_L1, timestamp);
    transaction();
    execute(query);
    commit();
}

std::unordered_map<QByteArray, std::vector<QOlmSession> > Database::loadOlmSessions()
{
    auto query = prepareQuery(QStringLiteral(
        "SELECT * FROM olm_sessions ORDER BY lastReceived DESC;"));
    transaction();
    execute(query);
    commit();
    std::unordered_map<QByteArray, std::vector<QOlmSession>> sessions;
    while (query.next()) {
        if (auto&& expectedSession =
                QOlmSession::unpickle(query.value("pickle"_L1).toByteArray(),
                                      m_picklingKey)) {
            sessions[query.value("senderKey"_L1).toByteArray()].emplace_back(
                std::move(*expectedSession));
        } else
            qCWarning(E2EE)
                << "Failed to unpickle olm session:" << expectedSession.error();
    }
    return sessions;
}

std::unordered_map<QByteArray, QOlmInboundGroupSession> Database::loadMegolmSessions(
    const QString& roomId)
{
    auto query = prepareQuery(QStringLiteral("SELECT * FROM inbound_megolm_sessions WHERE roomId=:roomId;"));
    query.bindValue(":roomId"_L1, roomId);
    transaction();
    execute(query);
    commit();
    decltype(Database::loadMegolmSessions({})) sessions;
    while (query.next()) {
        if (auto&& expectedSession = QOlmInboundGroupSession::unpickle(
                query.value("pickle"_L1).toByteArray(), m_picklingKey)) {
            const auto sessionId = query.value("sessionId"_L1).toByteArray();
            if (const auto it = sessions.find(sessionId); it != sessions.end()) {
                qCritical(DATABASE) << "More than one inbound group session "
                                       "with the same session id"
                                    << sessionId << "in the database";
                Q_ASSERT(false);
                // For Release builds, take the last session found
                qCritical(DATABASE)
                    << "The database is intact but only one session will be "
                       "used so some messages will be undecryptable";
                sessions.erase(it);
            }
            expectedSession->setOlmSessionId(
                query.value("olmSessionId"_L1).toByteArray());
            expectedSession->setSenderId(query.value("senderId"_L1).toString());
            sessions.try_emplace(query.value("sessionId"_L1).toByteArray(),
                                 std::move(*expectedSession));
        } else
            qCWarning(E2EE) << "Failed to unpickle megolm session:"
                            << expectedSession.error();
    }
    return sessions;
}

void Database::saveMegolmSession(const QString& roomId,
                                 const QOlmInboundGroupSession& session, const QByteArray &senderKey, const QByteArray& senderClaimedEdKey)
{
    auto deleteQuery = prepareQuery(QStringLiteral("DELETE FROM inbound_megolm_sessions WHERE roomId=:roomId AND sessionId=:sessionId;"));
    deleteQuery.bindValue(":roomId"_L1, roomId);
    deleteQuery.bindValue(":sessionId"_L1, session.sessionId());
    auto query = prepareQuery(
        QStringLiteral("INSERT INTO inbound_megolm_sessions(roomId, sessionId, pickle, senderId, olmSessionId, senderKey, senderClaimedEd25519Key) VALUES(:roomId, :sessionId, :pickle, :senderId, :olmSessionId, :senderKey, :senderClaimedEd25519Key);"));
    query.bindValue(":roomId"_L1, roomId);
    query.bindValue(":sessionId"_L1, session.sessionId());
    query.bindValue(":pickle"_L1, session.pickle(m_picklingKey));
    query.bindValue(":senderId"_L1, session.senderId());
    query.bindValue(":olmSessionId"_L1, session.olmSessionId());
    query.bindValue(":senderKey"_L1, senderKey);
    query.bindValue(":senderClaimedEd25519Key"_L1, senderClaimedEdKey);
    transaction();
    execute(deleteQuery);
    execute(query);
    commit();
}

void Database::addGroupSessionIndexRecord(const QString& roomId, const QString& sessionId, uint32_t index, const QString& eventId, qint64 ts)
{
    auto query = prepareQuery("INSERT INTO group_session_record_index(roomId, sessionId, i, eventId, ts) VALUES(:roomId, :sessionId, :index, :eventId, :ts);"_L1);
    query.bindValue(":roomId"_L1, roomId);
    query.bindValue(":sessionId"_L1, sessionId);
    query.bindValue(":index"_L1, index);
    query.bindValue(":eventId"_L1, eventId);
    query.bindValue(":ts"_L1, ts);
    transaction();
    execute(query);
    commit();
}

std::pair<QString, qint64> Database::groupSessionIndexRecord(const QString& roomId, const QString& sessionId, qint64 index)
{
    auto query = prepareQuery(QStringLiteral("SELECT * FROM group_session_record_index WHERE roomId=:roomId AND sessionId=:sessionId AND i=:index;"));
    query.bindValue(":roomId"_L1, roomId);
    query.bindValue(":sessionId"_L1, sessionId);
    query.bindValue(":index"_L1, index);
    transaction();
    execute(query);
    commit();
    if (!query.next()) {
        return {};
    }
    return {query.value("eventId"_L1).toString(), query.value("ts"_L1).toLongLong()};
}

QSqlDatabase Database::database() const
{
    return QSqlDatabase::database("Quotient_"_L1 + m_userId);
}

QSqlQuery Database::prepareQuery(const QString& queryString) const
{
    QSqlQuery query(database());
    query.prepare(queryString);
    return query;
}

void Database::clearRoomData(const QString& roomId)
{
    transaction();
    for (const auto& queryText :
         { QStringLiteral(
               "DELETE FROM inbound_megolm_sessions WHERE roomId=:roomId;"),
           QStringLiteral(
               "DELETE FROM outbound_megolm_sessions WHERE roomId=:roomId;"),
           QStringLiteral("DELETE FROM group_session_record_index WHERE "
                          "roomId=:roomId;") }) {
        auto q = prepareQuery(queryText);
        q.bindValue(QStringLiteral(":roomId"), roomId);
        execute(q);
    }
    commit();
}

void Database::setOlmSessionLastReceived(const QByteArray& sessionId, const QDateTime& timestamp)
{
    auto query = prepareQuery(QStringLiteral("UPDATE olm_sessions SET lastReceived=:lastReceived WHERE sessionId=:sessionId;"));
    query.bindValue(":lastReceived"_L1, timestamp);
    query.bindValue(":sessionId"_L1, sessionId);
    transaction();
    execute(query);
    commit();
}

void Database::saveCurrentOutboundMegolmSession(const QString& roomId,
    const QOlmOutboundGroupSession& session)
{
    const auto pickle = session.pickle(m_picklingKey);
    auto deleteQuery = prepareQuery(
        QStringLiteral("DELETE FROM outbound_megolm_sessions WHERE roomId=:roomId AND sessionId=:sessionId;"));
    deleteQuery.bindValue(":roomId"_L1, roomId);
    deleteQuery.bindValue(":sessionId"_L1, session.sessionId());

    auto insertQuery = prepareQuery(
        QStringLiteral("INSERT INTO outbound_megolm_sessions(roomId, sessionId, pickle, creationTime, messageCount) VALUES(:roomId, :sessionId, :pickle, :creationTime, :messageCount);"));
    insertQuery.bindValue(":roomId"_L1, roomId);
    insertQuery.bindValue(":sessionId"_L1, session.sessionId());
    insertQuery.bindValue(":pickle"_L1, pickle);
    insertQuery.bindValue(":creationTime"_L1, session.creationTime());
    insertQuery.bindValue(":messageCount"_L1, session.messageCount());

    transaction();
    execute(deleteQuery);
    execute(insertQuery);
    commit();
}

std::optional<QOlmOutboundGroupSession> Database::loadCurrentOutboundMegolmSession(
    const QString& roomId)
{
    auto query = prepareQuery(
        QStringLiteral("SELECT * FROM outbound_megolm_sessions WHERE roomId=:roomId ORDER BY creationTime DESC;"));
    query.bindValue(":roomId"_L1, roomId);
    execute(query);
    if (query.next()) {
        if (auto&& session = QOlmOutboundGroupSession::unpickle(
                query.value("pickle"_L1).toByteArray(), m_picklingKey)) {
            session->setCreationTime(
                query.value("creationTime"_L1).toDateTime());
            session->setMessageCount(query.value("messageCount"_L1).toInt());
            return std::move(*session);
        }
    }
    return {};
}

void Database::setDevicesReceivedKey(
    const QString& roomId,
    const QVector<std::tuple<QString, QString, QString>>& devices,
    const QByteArray& sessionId, uint32_t index)
{
    transaction();
    for (const auto& [user, device, curveKey] : devices) {
        auto query = prepareQuery(QStringLiteral("INSERT INTO sent_megolm_sessions(roomId, userId, deviceId, identityKey, sessionId, i) VALUES(:roomId, :userId, :deviceId, :identityKey, :sessionId, :i);"));
        query.bindValue(":roomId"_L1, roomId);
        query.bindValue(":userId"_L1, user);
        query.bindValue(":deviceId"_L1, device);
        query.bindValue(":identityKey"_L1, curveKey);
        query.bindValue(":sessionId"_L1, sessionId);
        query.bindValue(":i"_L1, index);
        execute(query);
    }
    commit();
}

QMultiHash<QString, QString> Database::devicesWithoutKey(
    const QString& roomId, QMultiHash<QString, QString> devices,
    const QByteArray& sessionId)
{
    auto query = prepareQuery(QStringLiteral("SELECT userId, deviceId FROM sent_megolm_sessions WHERE roomId=:roomId AND sessionId=:sessionId"));
    query.bindValue(":roomId"_L1, roomId);
    query.bindValue(":sessionId"_L1, sessionId);
    transaction();
    execute(query);
    commit();
    while (query.next()) {
        devices.remove(query.value("userId"_L1).toString(),
                       query.value("deviceId"_L1).toString());
    }
    return devices;
}

void Database::updateOlmSession(const QByteArray& senderKey,
                                const QOlmSession& session)
{
    auto query = prepareQuery(
        QStringLiteral("UPDATE olm_sessions SET pickle=:pickle WHERE senderKey=:senderKey AND sessionId=:sessionId;"));
    query.bindValue(":pickle"_L1, session.pickle(m_picklingKey));
    query.bindValue(":senderKey"_L1, senderKey);
    query.bindValue(":sessionId"_L1, session.sessionId());
    transaction();
    execute(query);
    commit();
}

void Database::setSessionVerified(const QString& edKeyId)
{
    auto query = prepareQuery(QStringLiteral("UPDATE tracked_devices SET verified=true WHERE edKeyId=:edKeyId;"));
    query.bindValue(":edKeyId"_L1, edKeyId);
    transaction();
    execute(query);
    commit();
}

bool Database::isSessionVerified(const QString& edKey)
{
    auto query = prepareQuery(QStringLiteral("SELECT verified FROM tracked_devices WHERE edKey=:edKey"));
    query.bindValue(":edKey"_L1, edKey);
    execute(query);
    return query.next() && query.value("verified"_L1).toBool();
}

QString Database::edKeyForKeyId(const QString& userId, const QString& edKeyId)
{
    auto query = prepareQuery(QStringLiteral("SELECT edKey FROM tracked_devices WHERE matrixId=:userId and edKeyId=:edKeyId;"));
    query.bindValue(":matrixId"_L1, userId);
    query.bindValue(":edKeyId"_L1, edKeyId);
    execute(query);
    if (!query.next()) {
        return {};
    }
    return query.value("edKey"_L1).toString();
}

void Database::storeEncrypted(const QString& name, const QByteArray& key)
{
    auto iv = getRandom<AesBlockSize>();
    auto result =
        aesCtr256Encrypt(key, asCBytes(m_picklingKey).first<Aes256KeySize>(),
                         iv);
    if (!result.has_value())
        return;

    auto cipher = result.value().toBase64();
    auto query = prepareQuery(QStringLiteral("INSERT INTO encrypted(name, cipher, iv) VALUES(:name, :cipher, :iv);"));
    auto deleteQuery = prepareQuery(QStringLiteral("DELETE FROM encrypted WHERE name=:name;"));
    deleteQuery.bindValue(":name"_L1, name);
    query.bindValue(":name"_L1, name);
    query.bindValue(":cipher"_L1, cipher);
    query.bindValue(":iv"_L1, iv.toBase64());
    transaction();
    execute(deleteQuery);
    execute(query);
    commit();
}

QByteArray Database::loadEncrypted(const QString& name)
{
    auto query = prepareQuery("SELECT cipher, iv FROM encrypted WHERE name=:name;"_L1);
    query.bindValue(":name"_L1, name);
    execute(query);
    if (!query.next()) {
        return {};
    }
    auto cipher = QByteArray::fromBase64(query.value("cipher"_L1).toString().toLatin1());
    auto iv = QByteArray::fromBase64(query.value("iv"_L1).toString().toLatin1());
    if (iv.size() < AesBlockSize) {
        qCWarning(E2EE) << "Corrupt iv at the database record for" << name;
        return {};
    }

    return aesCtr256Decrypt(cipher, asCBytes(m_picklingKey).first<Aes256KeySize>(),
                            asCBytes<AesBlockSize>(iv))
        .move_value_or({});
}

void Database::setMasterKeyVerified(const QString& masterKey)
{
    auto query = prepareQuery(QStringLiteral("UPDATE master_keys SET verified=true WHERE key=:key;"));
    query.bindValue(":key"_L1, masterKey);
    transaction();
    execute(query);
    commit();
}

QString Database::userSigningPublicKey()
{
    auto query = prepareQuery(QStringLiteral("SELECT key FROM user_signing_keys WHERE userId=:userId;"));
    query.bindValue(":userId"_L1, m_userId);
    execute(query);
    return query.next() ? query.value("key"_L1).toString() : QString();
}

QString Database::selfSigningPublicKey()
{
    auto query = prepareQuery(QStringLiteral("SELECT key FROM self_signing_keys WHERE userId=:userId;"));
    query.bindValue(":userId"_L1, m_userId);
    execute(query);
    return query.next() ? query.value("key"_L1).toString() : QString();
}

QString Database::edKeyForMegolmSession(const QString& sessionId)
{
    auto query = prepareQuery(QStringLiteral("SELECT senderClaimedEd25519Key FROM inbound_megolm_sessions WHERE sessionId=:sessionId;"));
    query.bindValue(":sessionId"_L1, sessionId.toLatin1());
    execute(query);
    return query.next() ? query.value("senderClaimedEd25519Key"_L1).toString() : QString();
}

QString Database::senderKeyForMegolmSession(const QString& sessionId)
{
    auto query = prepareQuery(QStringLiteral("SELECT senderKey FROM inbound_megolm_sessions WHERE sessionId=:sessionId;"));
    query.bindValue(":sessionId"_L1, sessionId.toLatin1());
    execute(query);
    return query.next() ? query.value("senderKey"_L1).toString() : QString();
}
