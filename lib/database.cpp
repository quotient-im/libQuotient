// SPDX-FileCopyrightText: 2021 Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "database.h"

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QStandardPaths>
#include <QtCore/QDebug>
#include <QtCore/QDir>

#include "e2ee/e2ee.h"
#include "e2ee/qolmsession.h"
#include "e2ee/qolminboundsession.h"
#include "e2ee/qolmoutboundsession.h"

using namespace Quotient;
Database::Database(const QString& matrixId, const QString& deviceId, QObject* parent)
    : QObject(parent)
    , m_matrixId(matrixId)
{
    m_matrixId.replace(':', '_');
    QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("Quotient_%1").arg(m_matrixId));
    QString databasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/%1").arg(m_matrixId);
    QDir(databasePath).mkpath(databasePath);
    database().setDatabaseName(databasePath + QStringLiteral("/quotient_%1.db3").arg(deviceId));
    database().open();

    switch(version()) {
    case 0: migrateTo1(); [[fallthrough]];
    case 1: migrateTo2(); [[fallthrough]];
    case 2: migrateTo3(); [[fallthrough]];
    case 3: migrateTo4(); [[fallthrough]];
    case 4: migrateTo5();
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
        qCritical() << "Failed to check database version";
    }
    return -1;
}

QSqlQuery Database::execute(const QString &queryString)
{
    auto query = database().exec(queryString);
    if (query.lastError().type() != QSqlError::NoError) {
        qCritical() << "Failed to execute query";
        qCritical() << query.lastQuery();
        qCritical() << query.lastError();
    }
    return query;
}

QSqlQuery Database::execute(QSqlQuery &query)
{
    if (!query.exec()) {
        qCritical() << "Failed to execute query";
        qCritical() << query.lastQuery();
        qCritical() << query.lastError();
    }
    return query;
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

QByteArray Database::accountPickle()
{
    auto query = prepareQuery(QStringLiteral("SELECT pickle FROM accounts;"));
    execute(query);
    if (query.next()) {
        return query.value(QStringLiteral("pickle")).toByteArray();
    }
    return {};
}

void Database::setAccountPickle(const QByteArray &pickle)
{
    auto deleteQuery = prepareQuery(QStringLiteral("DELETE FROM accounts;"));
    auto query = prepareQuery(QStringLiteral("INSERT INTO accounts(pickle) VALUES(:pickle);"));
    query.bindValue(":pickle", pickle);
    transaction();
    execute(deleteQuery);
    execute(query);
    commit();
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

void Database::saveOlmSession(const QString& senderKey, const QString& sessionId, const QByteArray &pickle, const QDateTime& timestamp)
{
    auto query = prepareQuery(QStringLiteral("INSERT INTO olm_sessions(senderKey, sessionId, pickle, lastReceived) VALUES(:senderKey, :sessionId, :pickle, :lastReceived);"));
    query.bindValue(":senderKey", senderKey);
    query.bindValue(":sessionId", sessionId);
    query.bindValue(":pickle", pickle);
    query.bindValue(":lastReceived", timestamp);
    transaction();
    execute(query);
    commit();
}

UnorderedMap<QString, std::vector<QOlmSessionPtr>> Database::loadOlmSessions(const PicklingMode& picklingMode)
{
    auto query = prepareQuery(QStringLiteral("SELECT * FROM olm_sessions ORDER BY lastReceived DESC;"));
    transaction();
    execute(query);
    commit();
    UnorderedMap<QString, std::vector<QOlmSessionPtr>> sessions;
    while (query.next()) {
        if (auto expectedSession =
                QOlmSession::unpickle(query.value("pickle").toByteArray(),
                                      picklingMode)) {
            sessions[query.value("senderKey").toString()].emplace_back(
                std::move(*expectedSession));
        } else
            qCWarning(E2EE)
                << "Failed to unpickle olm session:" << expectedSession.error();
    }
    return sessions;
}

UnorderedMap<QString, QOlmInboundGroupSessionPtr> Database::loadMegolmSessions(const QString& roomId, const PicklingMode& picklingMode)
{
    auto query = prepareQuery(QStringLiteral("SELECT * FROM inbound_megolm_sessions WHERE roomId=:roomId;"));
    query.bindValue(":roomId", roomId);
    transaction();
    execute(query);
    commit();
    UnorderedMap<QString, QOlmInboundGroupSessionPtr> sessions;
    while (query.next()) {
        if (auto expectedSession = QOlmInboundGroupSession::unpickle(
                query.value("pickle").toByteArray(), picklingMode)) {
            auto& sessionPtr = sessions[query.value("sessionId").toString()] =
                std::move(*expectedSession);
            sessionPtr->setOlmSessionId(query.value("olmSessionId").toString());
            sessionPtr->setSenderId(query.value("senderId").toString());
        } else
            qCWarning(E2EE) << "Failed to unpickle megolm session:"
                            << expectedSession.error();
    }
    return sessions;
}

void Database::saveMegolmSession(const QString& roomId, const QString& sessionId, const QByteArray& pickle, const QString& senderId, const QString& olmSessionId)
{
    auto query = prepareQuery(QStringLiteral("INSERT INTO inbound_megolm_sessions(roomId, sessionId, pickle, senderId, olmSessionId) VALUES(:roomId, :sessionId, :pickle, :senderId, :olmSessionId);"));
    query.bindValue(":roomId", roomId);
    query.bindValue(":sessionId", sessionId);
    query.bindValue(":pickle", pickle);
    query.bindValue(":senderId", senderId);
    query.bindValue(":olmSessionId", olmSessionId);
    transaction();
    execute(query);
    commit();
}

void Database::addGroupSessionIndexRecord(const QString& roomId, const QString& sessionId, uint32_t index, const QString& eventId, qint64 ts)
{
    auto query = prepareQuery("INSERT INTO group_session_record_index(roomId, sessionId, i, eventId, ts) VALUES(:roomId, :sessionId, :index, :eventId, :ts);");
    query.bindValue(":roomId", roomId);
    query.bindValue(":sessionId", sessionId);
    query.bindValue(":index", index);
    query.bindValue(":eventId", eventId);
    query.bindValue(":ts", ts);
    transaction();
    execute(query);
    commit();
}

std::pair<QString, qint64> Database::groupSessionIndexRecord(const QString& roomId, const QString& sessionId, qint64 index)
{
    auto query = prepareQuery(QStringLiteral("SELECT * FROM group_session_record_index WHERE roomId=:roomId AND sessionId=:sessionId AND i=:index;"));
    query.bindValue(":roomId", roomId);
    query.bindValue(":sessionId", sessionId);
    query.bindValue(":index", index);
    transaction();
    execute(query);
    commit();
    if (!query.next()) {
        return {};
    }
    return {query.value("eventId").toString(), query.value("ts").toLongLong()};
}

QSqlDatabase Database::database()
{
    return QSqlDatabase::database(QStringLiteral("Quotient_%1").arg(m_matrixId));
}

QSqlQuery Database::prepareQuery(const QString& queryString)
{
    QSqlQuery query(database());
    query.prepare(queryString);
    return query;
}

void Database::clearRoomData(const QString& roomId)
{
    auto query = prepareQuery(QStringLiteral("DELETE FROM inbound_megolm_sessions WHERE roomId=:roomId;"));
    auto query2 = prepareQuery(QStringLiteral("DELETE FROM outbound_megolm_sessions WHERE roomId=:roomId;"));
    auto query3 = prepareQuery(QStringLiteral("DELETE FROM group_session_record_index WHERE roomId=:roomId;"));
    transaction();
    execute(query);
    execute(query2);
    execute(query3);
    commit();
}

void Database::setOlmSessionLastReceived(const QString& sessionId, const QDateTime& timestamp)
{
    auto query = prepareQuery(QStringLiteral("UPDATE olm_sessions SET lastReceived=:lastReceived WHERE sessionId=:sessionId;"));
    query.bindValue(":lastReceived", timestamp);
    query.bindValue(":sessionId", sessionId);
    transaction();
    execute(query);
    commit();
}

void Database::saveCurrentOutboundMegolmSession(
    const QString& roomId, const PicklingMode& picklingMode,
    const QOlmOutboundGroupSession& session)
{
    const auto pickle = session.pickle(picklingMode);
    auto deleteQuery = prepareQuery(QStringLiteral("DELETE FROM outbound_megolm_sessions WHERE roomId=:roomId AND sessionId=:sessionId;"));
    deleteQuery.bindValue(":roomId", roomId);
    deleteQuery.bindValue(":sessionId", session.sessionId());

    auto insertQuery = prepareQuery(QStringLiteral("INSERT INTO outbound_megolm_sessions(roomId, sessionId, pickle, creationTime, messageCount) VALUES(:roomId, :sessionId, :pickle, :creationTime, :messageCount);"));
    insertQuery.bindValue(":roomId", roomId);
    insertQuery.bindValue(":sessionId", session.sessionId());
    insertQuery.bindValue(":pickle", pickle);
    insertQuery.bindValue(":creationTime", session.creationTime());
    insertQuery.bindValue(":messageCount", session.messageCount());

    transaction();
    execute(deleteQuery);
    execute(insertQuery);
    commit();
}

QOlmOutboundGroupSessionPtr Database::loadCurrentOutboundMegolmSession(const QString& roomId, const PicklingMode& picklingMode)
{
    auto query = prepareQuery(QStringLiteral("SELECT * FROM outbound_megolm_sessions WHERE roomId=:roomId ORDER BY creationTime DESC;"));
    query.bindValue(":roomId", roomId);
    execute(query);
    if (query.next()) {
        auto sessionResult = QOlmOutboundGroupSession::unpickle(query.value("pickle").toByteArray(), picklingMode);
        if (sessionResult) {
            auto session = std::move(*sessionResult);
            session->setCreationTime(query.value("creationTime").toDateTime());
            session->setMessageCount(query.value("messageCount").toInt());
            return session;
        }
    }
    return nullptr;
}

void Database::setDevicesReceivedKey(const QString& roomId, const QVector<std::tuple<QString, QString, QString>>& devices, const QString& sessionId, int index)
{
    transaction();
    for (const auto& [user, device, curveKey] : devices) {
        auto query = prepareQuery(QStringLiteral("INSERT INTO sent_megolm_sessions(roomId, userId, deviceId, identityKey, sessionId, i) VALUES(:roomId, :userId, :deviceId, :identityKey, :sessionId, :i);"));
        query.bindValue(":roomId", roomId);
        query.bindValue(":userId", user);
        query.bindValue(":deviceId", device);
        query.bindValue(":identityKey", curveKey);
        query.bindValue(":sessionId", sessionId);
        query.bindValue(":i", index);
        execute(query);
    }
    commit();
}

QMultiHash<QString, QString> Database::devicesWithoutKey(
    const QString& roomId, QMultiHash<QString, QString> devices,
    const QString& sessionId)
{
    auto query = prepareQuery(QStringLiteral("SELECT userId, deviceId FROM sent_megolm_sessions WHERE roomId=:roomId AND sessionId=:sessionId"));
    query.bindValue(":roomId", roomId);
    query.bindValue(":sessionId", sessionId);
    transaction();
    execute(query);
    commit();
    while (query.next()) {
        devices.remove(query.value("userId").toString(),
                       query.value("deviceId").toString());
    }
    return devices;
}

void Database::updateOlmSession(const QString& senderKey, const QString& sessionId, const QByteArray& pickle)
{
    auto query = prepareQuery(QStringLiteral("UPDATE olm_sessions SET pickle=:pickle WHERE senderKey=:senderKey AND sessionId=:sessionId;"));
    query.bindValue(":pickle", pickle);
    query.bindValue(":senderKey", senderKey);
    query.bindValue(":sessionId", sessionId);
    transaction();
    execute(query);
    commit();
}

void Database::setSessionVerified(const QString& edKeyId)
{
    auto query = prepareQuery(QStringLiteral("UPDATE tracked_devices SET verified=true WHERE edKeyId=:edKeyId;"));
    query.bindValue(":edKeyId", edKeyId);
    transaction();
    execute(query);
    commit();
}

bool Database::isSessionVerified(const QString& edKey)
{
    auto query = prepareQuery(QStringLiteral("SELECT verified FROM tracked_devices WHERE edKey=:edKey"));
    query.bindValue(":edKey", edKey);
    execute(query);
    return query.next() && query.value("verified").toBool();
}
