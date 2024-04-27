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
                                        "Quotient_"_ls + m_userId);
    auto dbDir = m_userId;
    dbDir.replace(u':', u'_');
    const QString databasePath{ QStandardPaths::writableLocation(
                                    QStandardPaths::AppDataLocation)
                                % u'/' % dbDir };
    QDir(databasePath).mkpath("."_ls);
    db.setDatabaseName(databasePath + "/quotient_%1.db3"_ls.arg(m_deviceId));
    db.open(); // Further accessed via database()

    switch(version()) {
    case 0: migrateTo1(); [[fallthrough]];
    case 1: migrateTo2(); [[fallthrough]];
    case 2: migrateTo3(); [[fallthrough]];
    case 3: migrateTo4(); [[fallthrough]];
    case 4: migrateTo5(); [[fallthrough]];
    case 5: migrateTo6();
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
    auto query = database().exec(queryString);
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

void Database::storeOlmAccount(const QOlmAccount& olmAccount)
{
    auto deleteQuery = prepareQuery(QStringLiteral("DELETE FROM accounts;"));
    auto query = prepareQuery(QStringLiteral("INSERT INTO accounts(pickle) VALUES(:pickle);"));
    query.bindValue(":pickle"_ls, olmAccount.pickle(m_picklingKey));
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
    auto query = prepareQuery(QStringLiteral("DELETE FROM accounts;"));
    auto sessionsQuery = prepareQuery(QStringLiteral("DELETE FROM olm_sessions;"));
    auto megolmSessionsQuery = prepareQuery(QStringLiteral("DELETE FROM inbound_megolm_sessions;"));
    auto groupSessionIndexRecordQuery = prepareQuery(QStringLiteral("DELETE FROM group_session_record_index;"));

    transaction();
    execute(query);
    execute(sessionsQuery);
    execute(megolmSessionsQuery);
    execute(groupSessionIndexRecordQuery);
    commit();

}

void Database::saveOlmSession(const QByteArray& senderKey,
                              const QOlmSession& session,
                              const QDateTime& timestamp)
{
    auto query = prepareQuery(QStringLiteral("INSERT INTO olm_sessions(senderKey, sessionId, pickle, lastReceived) VALUES(:senderKey, :sessionId, :pickle, :lastReceived);"));
    query.bindValue(":senderKey"_ls, senderKey);
    query.bindValue(":sessionId"_ls, session.sessionId());
    query.bindValue(":pickle"_ls, session.pickle(m_picklingKey));
    query.bindValue(":lastReceived"_ls, timestamp);
    transaction();
    execute(query);
    commit();
}

UnorderedMap<QByteArray, std::vector<QOlmSession>> Database::loadOlmSessions()
{
    auto query = prepareQuery(QStringLiteral(
        "SELECT * FROM olm_sessions ORDER BY lastReceived DESC;"));
    transaction();
    execute(query);
    commit();
    UnorderedMap<QByteArray, std::vector<QOlmSession>> sessions;
    while (query.next()) {
        if (auto&& expectedSession =
                QOlmSession::unpickle(query.value("pickle"_ls).toByteArray(),
                                      m_picklingKey)) {
            sessions[query.value("senderKey"_ls).toByteArray()].emplace_back(
                std::move(*expectedSession));
        } else
            qCWarning(E2EE)
                << "Failed to unpickle olm session:" << expectedSession.error();
    }
    return sessions;
}

UnorderedMap<QByteArray, QOlmInboundGroupSession> Database::loadMegolmSessions(
    const QString& roomId)
{
    auto query = prepareQuery(QStringLiteral("SELECT * FROM inbound_megolm_sessions WHERE roomId=:roomId;"));
    query.bindValue(":roomId"_ls, roomId);
    transaction();
    execute(query);
    commit();
    UnorderedMap<QByteArray, QOlmInboundGroupSession> sessions;
    while (query.next()) {
        if (auto&& expectedSession = QOlmInboundGroupSession::unpickle(
                query.value("pickle"_ls).toByteArray(), m_picklingKey)) {
            const auto sessionId = query.value("sessionId"_ls).toByteArray();
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
                query.value("olmSessionId"_ls).toByteArray());
            expectedSession->setSenderId(query.value("senderId"_ls).toString());
            sessions.try_emplace(query.value("sessionId"_ls).toByteArray(),
                                 std::move(*expectedSession));
        } else
            qCWarning(E2EE) << "Failed to unpickle megolm session:"
                            << expectedSession.error();
    }
    return sessions;
}

void Database::saveMegolmSession(const QString& roomId,
                                 const QOlmInboundGroupSession& session)
{
    auto deleteQuery = prepareQuery(QStringLiteral("DELETE FROM inbound_megolm_sessions WHERE roomId=:roomId AND sessionId=:sessionId;"));
    deleteQuery.bindValue(":roomId"_ls, roomId);
    deleteQuery.bindValue(":sessionId"_ls, session.sessionId());
    auto query = prepareQuery(
        QStringLiteral("INSERT INTO inbound_megolm_sessions(roomId, sessionId, pickle, senderId, olmSessionId) VALUES(:roomId, :sessionId, :pickle, :senderId, :olmSessionId);"));
    query.bindValue(":roomId"_ls, roomId);
    query.bindValue(":sessionId"_ls, session.sessionId());
    query.bindValue(":pickle"_ls, session.pickle(m_picklingKey));
    query.bindValue(":senderId"_ls, session.senderId());
    query.bindValue(":olmSessionId"_ls, session.olmSessionId());
    transaction();
    execute(deleteQuery);
    execute(query);
    commit();
}

void Database::addGroupSessionIndexRecord(const QString& roomId, const QString& sessionId, uint32_t index, const QString& eventId, qint64 ts)
{
    auto query = prepareQuery("INSERT INTO group_session_record_index(roomId, sessionId, i, eventId, ts) VALUES(:roomId, :sessionId, :index, :eventId, :ts);"_ls);
    query.bindValue(":roomId"_ls, roomId);
    query.bindValue(":sessionId"_ls, sessionId);
    query.bindValue(":index"_ls, index);
    query.bindValue(":eventId"_ls, eventId);
    query.bindValue(":ts"_ls, ts);
    transaction();
    execute(query);
    commit();
}

std::pair<QString, qint64> Database::groupSessionIndexRecord(const QString& roomId, const QString& sessionId, qint64 index)
{
    auto query = prepareQuery(QStringLiteral("SELECT * FROM group_session_record_index WHERE roomId=:roomId AND sessionId=:sessionId AND i=:index;"));
    query.bindValue(":roomId"_ls, roomId);
    query.bindValue(":sessionId"_ls, sessionId);
    query.bindValue(":index"_ls, index);
    transaction();
    execute(query);
    commit();
    if (!query.next()) {
        return {};
    }
    return {query.value("eventId"_ls).toString(), query.value("ts"_ls).toLongLong()};
}

QSqlDatabase Database::database() const
{
    return QSqlDatabase::database("Quotient_"_ls + m_userId);
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
    query.bindValue(":lastReceived"_ls, timestamp);
    query.bindValue(":sessionId"_ls, sessionId);
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
    deleteQuery.bindValue(":roomId"_ls, roomId);
    deleteQuery.bindValue(":sessionId"_ls, session.sessionId());

    auto insertQuery = prepareQuery(
        QStringLiteral("INSERT INTO outbound_megolm_sessions(roomId, sessionId, pickle, creationTime, messageCount) VALUES(:roomId, :sessionId, :pickle, :creationTime, :messageCount);"));
    insertQuery.bindValue(":roomId"_ls, roomId);
    insertQuery.bindValue(":sessionId"_ls, session.sessionId());
    insertQuery.bindValue(":pickle"_ls, pickle);
    insertQuery.bindValue(":creationTime"_ls, session.creationTime());
    insertQuery.bindValue(":messageCount"_ls, session.messageCount());

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
    query.bindValue(":roomId"_ls, roomId);
    execute(query);
    if (query.next()) {
        if (auto&& session = QOlmOutboundGroupSession::unpickle(
                query.value("pickle"_ls).toByteArray(), m_picklingKey)) {
            session->setCreationTime(
                query.value("creationTime"_ls).toDateTime());
            session->setMessageCount(query.value("messageCount"_ls).toInt());
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
        query.bindValue(":roomId"_ls, roomId);
        query.bindValue(":userId"_ls, user);
        query.bindValue(":deviceId"_ls, device);
        query.bindValue(":identityKey"_ls, curveKey);
        query.bindValue(":sessionId"_ls, sessionId);
        query.bindValue(":i"_ls, index);
        execute(query);
    }
    commit();
}

QMultiHash<QString, QString> Database::devicesWithoutKey(
    const QString& roomId, QMultiHash<QString, QString> devices,
    const QByteArray& sessionId)
{
    auto query = prepareQuery(QStringLiteral("SELECT userId, deviceId FROM sent_megolm_sessions WHERE roomId=:roomId AND sessionId=:sessionId"));
    query.bindValue(":roomId"_ls, roomId);
    query.bindValue(":sessionId"_ls, sessionId);
    transaction();
    execute(query);
    commit();
    while (query.next()) {
        devices.remove(query.value("userId"_ls).toString(),
                       query.value("deviceId"_ls).toString());
    }
    return devices;
}

void Database::updateOlmSession(const QByteArray& senderKey,
                                const QOlmSession& session)
{
    auto query = prepareQuery(
        QStringLiteral("UPDATE olm_sessions SET pickle=:pickle WHERE senderKey=:senderKey AND sessionId=:sessionId;"));
    query.bindValue(":pickle"_ls, session.pickle(m_picklingKey));
    query.bindValue(":senderKey"_ls, senderKey);
    query.bindValue(":sessionId"_ls, session.sessionId());
    transaction();
    execute(query);
    commit();
}

void Database::setSessionVerified(const QString& edKeyId)
{
    auto query = prepareQuery(QStringLiteral("UPDATE tracked_devices SET verified=true WHERE edKeyId=:edKeyId;"));
    query.bindValue(":edKeyId"_ls, edKeyId);
    transaction();
    execute(query);
    commit();
}

bool Database::isSessionVerified(const QString& edKey)
{
    auto query = prepareQuery(QStringLiteral("SELECT verified FROM tracked_devices WHERE edKey=:edKey"));
    query.bindValue(":edKey"_ls, edKey);
    execute(query);
    return query.next() && query.value("verified"_ls).toBool();
}

QString Database::edKeyForKeyId(const QString& userId, const QString& edKeyId)
{
    auto query = prepareQuery(QStringLiteral("SELECT edKey FROM tracked_devices WHERE matrixId=:userId and edKeyId=:edKeyId;"));
    query.bindValue(":matrixId"_ls, userId);
    query.bindValue(":edKeyId"_ls, edKeyId);
    execute(query);
    if (!query.next()) {
        return {};
    }
    return query.value("edKey"_ls).toString();
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
    deleteQuery.bindValue(":name"_ls, name);
    query.bindValue(":name"_ls, name);
    query.bindValue(":cipher"_ls, cipher);
    query.bindValue(":iv"_ls, iv.toBase64());
    transaction();
    execute(deleteQuery);
    execute(query);
    commit();
}

QByteArray Database::loadEncrypted(const QString& name)
{
    auto query = prepareQuery("SELECT cipher, iv FROM encrypted WHERE name=:name;"_ls);
    query.bindValue(":name"_ls, name);
    execute(query);
    if (!query.next()) {
        return {};
    }
    auto cipher = QByteArray::fromBase64(query.value("cipher"_ls).toString().toLatin1());
    auto iv = QByteArray::fromBase64(query.value("iv"_ls).toString().toLatin1());
    if (iv.size() < AesBlockSize) {
        qCWarning(E2EE) << "Corrupt iv at the database record for" << name;
        return {};
    }

    return aesCtr256Decrypt(cipher, asCBytes(m_picklingKey).first<Aes256KeySize>(),
                            asCBytes<AesBlockSize>(iv))
        .move_value_or({});
}
