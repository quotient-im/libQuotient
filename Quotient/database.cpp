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

#include <vodozemac.h>

using namespace Qt::Literals::StringLiterals;

using namespace Quotient;

Database::Database(const QString& userId, const QString& deviceId,
                   PicklingKey&& picklingKey)
    : m_userId(userId)
    , m_deviceId(deviceId)
    , m_picklingKey(std::move(picklingKey))
{
    auto db = QSqlDatabase::addDatabase(u"QSQLITE"_s, "Quotient_"_L1 + m_userId);
    auto dbDir = m_userId;
    dbDir.replace(u':', u'_');
    const QString databasePath{ QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
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
    case 9: migrateTo10(); [[fallthrough]];
    case 10: migrateTo11();
    }
}

int Database::version()
{
    auto query = execute(u"PRAGMA user_version;"_s);
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
    execute(u"CREATE TABLE accounts (pickle TEXT);"_s);
    execute(u"CREATE TABLE olm_sessions (senderKey TEXT, sessionId TEXT, pickle TEXT);"_s);
    execute(u"CREATE TABLE inbound_megolm_sessions (roomId TEXT, senderKey TEXT, sessionId TEXT, pickle TEXT);"_s);
    execute(u"CREATE TABLE outbound_megolm_sessions (roomId TEXT, senderKey TEXT, sessionId TEXT, pickle TEXT);"_s);
    execute(u"CREATE TABLE group_session_record_index (roomId TEXT, sessionId TEXT, i INTEGER, eventId TEXT, ts INTEGER);"_s);
    execute(u"CREATE TABLE tracked_users (matrixId TEXT);"_s);
    execute(u"CREATE TABLE outdated_users (matrixId TEXT);"_s);
    execute(u"CREATE TABLE tracked_devices (matrixId TEXT, deviceId TEXT, curveKeyId TEXT, curveKey TEXT, edKeyId TEXT, edKey TEXT);"_s);

    execute(u"PRAGMA user_version = 1;"_s);
    commit();
}

void Database::migrateTo2()
{
    qCDebug(DATABASE) << "Migrating database to version 2";
    transaction();

    execute(u"ALTER TABLE inbound_megolm_sessions ADD ed25519Key TEXT"_s);
    execute(u"ALTER TABLE olm_sessions ADD lastReceived TEXT"_s);

    // Add indexes for improving queries speed on larger database
    execute(u"CREATE INDEX sessions_session_idx ON olm_sessions(sessionId)"_s);
    execute(u"CREATE INDEX outbound_room_idx ON outbound_megolm_sessions(roomId)"_s);
    execute(u"CREATE INDEX inbound_room_idx ON inbound_megolm_sessions(roomId)"_s);
    execute(u"CREATE INDEX group_session_idx ON group_session_record_index(roomId, sessionId, i)"_s);
    execute(u"PRAGMA user_version = 2;"_s);
    commit();
}

void Database::migrateTo3()
{
    qCDebug(DATABASE) << "Migrating database to version 3";
    transaction();

    execute(u"CREATE TABLE inbound_megolm_sessions_temp AS SELECT roomId, sessionId, pickle FROM inbound_megolm_sessions;"_s);
    execute(u"DROP TABLE inbound_megolm_sessions;"_s);
    execute(u"ALTER TABLE inbound_megolm_sessions_temp RENAME TO inbound_megolm_sessions;"_s);
    execute(u"ALTER TABLE inbound_megolm_sessions ADD olmSessionId TEXT;"_s);
    execute(u"ALTER TABLE inbound_megolm_sessions ADD senderId TEXT;"_s);
    execute(u"PRAGMA user_version = 3;"_s);
    commit();
}

void Database::migrateTo4()
{
    qCDebug(DATABASE) << "Migrating database to version 4";
    transaction();

    execute(u"CREATE TABLE sent_megolm_sessions (roomId TEXT, userId TEXT, deviceId TEXT, identityKey TEXT, sessionId TEXT, i INTEGER);"_s);
    execute(u"ALTER TABLE outbound_megolm_sessions ADD creationTime TEXT;"_s);
    execute(u"ALTER TABLE outbound_megolm_sessions ADD messageCount INTEGER;"_s);
    execute(u"PRAGMA user_version = 4;"_s);
    commit();
}

void Database::migrateTo5()
{
    qCDebug(DATABASE) << "Migrating database to version 5";
    transaction();

    execute(u"ALTER TABLE tracked_devices ADD verified BOOL;"_s);
    execute(u"PRAGMA user_version = 5"_s);
    commit();
}

void Database::migrateTo6()
{
    qCDebug(DATABASE) << "Migrating database to version 6";
    transaction();

    execute(u"CREATE TABLE encrypted (name TEXT, cipher TEXT, iv TEXT);"_s);
    execute(u"PRAGMA user_version = 6"_s);
    commit();
}

void Database::migrateTo7()
{
    qCDebug(DATABASE) << "Migrating database to version 7";
    transaction();
    execute(u"CREATE TABLE master_keys (userId TEXT, key TEXT, verified INTEGER);"_s);
    execute(u"CREATE TABLE self_signing_keys (userId TEXT, key TEXT);"_s);
    execute(u"CREATE TABLE user_signing_keys (userId TEXT, key TEXT);"_s);
    execute(u"INSERT INTO outdated_users SELECT * FROM tracked_users;"_s);
    execute(u"ALTER TABLE tracked_devices ADD selfVerified INTEGER;"_s);
    execute(u"PRAGMA user_version = 7;"_s);

    commit();
}

void Database::migrateTo8()
{
    qCDebug(DATABASE) << "Migrating database to version 8";
    transaction();

    execute(u"ALTER TABLE inbound_megolm_sessions ADD senderKey TEXT;"_s);
    auto query = prepareQuery(u"SELECT sessionId, olmSessionId FROM inbound_megolm_sessions;"_s);
    execute(query);
    while (query.next()) {
        if (query.value(u"olmSessionId"_s).toString().startsWith(u"BACKUP")) {
            continue;
        }
        auto senderKeyQuery = prepareQuery(u"SELECT senderKey FROM olm_sessions WHERE sessionId=:olmSessionId;"_s);
        senderKeyQuery.bindValue(u":olmSessionId"_s, query.value(u"olmSessionId"_s).toByteArray());
        execute(senderKeyQuery);
        if (!senderKeyQuery.next()) {
            continue;
        }
        auto updateQuery = prepareQuery(u"UPDATE inbound_megolm_sessions SET senderKey=:senderKey WHERE sessionId=:sessionId;"_s);
        updateQuery.bindValue(u":sessionId"_s, query.value(u"sessionId"_s).toByteArray());
        updateQuery.bindValue(u":senderKey"_s, senderKeyQuery.value(u"senderKey"_s).toByteArray());

        execute(updateQuery);
    }
    execute(u"PRAGMA user_version = 8;"_s);
    commit();
}

void Database::migrateTo9()
{
    qCDebug(DATABASE) << "Migrating database to version 9";
    transaction();

    auto query = prepareQuery(u"SELECT curveKey FROM tracked_devices WHERE matrixId=:matrixId AND deviceId=:deviceId;"_s);
    query.bindValue(u":matrixId"_s, m_userId);
    query.bindValue(u":deviceId"_s, m_deviceId);
    execute(query);
    if (!query.next()) {
        return;
    }
    auto curveKey = query.value(u"curveKey"_s).toByteArray();
    query = prepareQuery(u"UPDATE inbound_megolm_sessions SET senderKey=:senderKey WHERE olmSessionId=:self;"_s);
    query.bindValue(u":senderKey"_s, curveKey);
    query.bindValue(u":self"_s, "SELF"_ba);
    execute(u"PRAGMA user_version = 9;"_s);
    execute(query);
    commit();
}

void Database::migrateTo10()
{
    qCDebug(DATABASE) << "Migrating database to version 10";

    transaction();

    execute(u"ALTER TABLE inbound_megolm_sessions ADD senderClaimedEd25519Key TEXT;"_s);

    auto query = prepareQuery(u"SELECT DISTINCT senderKey FROM inbound_megolm_sessions;"_s);
    execute(query);

    QStringList keys;
    while (query.next()) {
        keys += query.value(u"senderKey"_s).toString();
    }
    for (const auto& key : keys) {
        auto edKeyQuery = prepareQuery(u"SELECT edKey FROM tracked_devices WHERE curveKey=:curveKey;"_s);
        edKeyQuery.bindValue(u":curveKey"_s, key);
        execute(edKeyQuery);
        if (!edKeyQuery.next()) {
            continue;
        }
        const auto &edKey = edKeyQuery.value(u"edKey"_s).toByteArray();

        auto updateQuery = prepareQuery(u"UPDATE inbound_megolm_sessions SET senderClaimedEd25519Key=:senderClaimedEd25519Key WHERE senderKey=:senderKey;"_s);
        updateQuery.bindValue(u":senderKey"_s, key.toLatin1());
        updateQuery.bindValue(u":senderClaimedEd25519Key"_s, edKey);
        execute(updateQuery);
    }

    execute(u"pragma user_version = 10"_s);
    commit();

}

void Database::migrateTo11()
{
    qCDebug(DATABASE) << "Migrating database to version 11";
    //TODO: This is terrible :(
    std::array<std::uint8_t, 32> vodoKey;
    std::copy(m_picklingKey.data(), m_picklingKey.data() + 32, vodoKey.begin());

    transaction();
    auto query = prepareQuery(u"SELECT pickle FROM accounts;"_s);
    execute(query);
    if (query.next()) {
        auto olmAccountPickle = query.value(u"pickle"_s).toString();
        auto account = olm::account_from_olm_pickle(rust::String((const char *) olmAccountPickle.toLatin1().data(), olmAccountPickle.size()), rust::Slice((const unsigned char*) m_picklingKey.data(), m_picklingKey.size()));
        auto pickle = account->pickle(vodoKey);
        execute(u"DELETE FROM accounts;"_s);
        query = prepareQuery(u"INSERT INTO accounts(pickle) VALUES(:pickle);"_s);
        query.bindValue(u":pickle"_s, QString::fromLatin1({pickle.data(), (qsizetype) pickle.size()}));
        execute(query);
    }

    query = prepareQuery(u"SELECT pickle FROM inbound_megolm_sessions;"_s);
    execute(query);
    QStringList inboundPickles;
    while(query.next()) {
        inboundPickles += query.value(0).toString();
    }
    for (const auto &olmPickle : inboundPickles) {
        auto session = megolm::inbound_group_session_from_olm_pickle((rust::String((const char *) olmPickle.toLatin1().data(), olmPickle.size())), rust::Slice((const unsigned char*) m_picklingKey.data(), m_picklingKey.size()));
        auto pickle = session->pickle(vodoKey);
        auto replaceQuery = prepareQuery(u"UPDATE inbound_megolm_sessions SET pickle=:pickle WHERE pickle=:olmPickle;"_s);
        replaceQuery.bindValue(u":pickle"_s, QString::fromLatin1({pickle.data(), (qsizetype) pickle.size()}));
        replaceQuery.bindValue(u":olmPickle"_s, olmPickle.toLatin1());
        execute(replaceQuery);
    }

    query = prepareQuery(u"SELECT pickle FROM outbound_megolm_sessions;"_s);
    execute(query);

    QStringList outboundPickles;
    while(query.next()) {
        outboundPickles += query.value(0).toString();
    }

    for (const auto &olmPickle : outboundPickles) {
        auto session = megolm::group_session_from_olm_pickle((rust::String((const char *) olmPickle.toLatin1().data(), olmPickle.size())), rust::Slice((const unsigned char*) m_picklingKey.data(), m_picklingKey.size()));
        auto pickle = session->pickle(vodoKey);
        auto replaceQuery = prepareQuery(u"UPDATE outbound_megolm_sessions SET pickle=:pickle WHERE pickle=:olmPickle;"_s);
        replaceQuery.bindValue(u":pickle"_s, QString::fromLatin1({pickle.data(), (qsizetype) pickle.size()}));
        replaceQuery.bindValue(u":olmPickle"_s, olmPickle.toLatin1());
        execute(replaceQuery);
    }

    query = prepareQuery(u"SELECT pickle FROM olm_sessions;"_s);
    execute(query);

    QStringList olmPickles;
    while (query.next()) {
     olmPickles += query.value(0).toString();
    }
    for (const auto &olmPickle : olmPickles) {
        auto session = olm::session_from_olm_pickle((rust::String((const char *) olmPickle.toLatin1().data(), olmPickle.size())), rust::Slice((const unsigned char*) m_picklingKey.data(), m_picklingKey.size()));
        auto pickle = session->pickle(vodoKey);
        auto replaceQuery = prepareQuery(u"UPDATE olm_sessions SET pickle=:pickle WHERE pickle=:olmPickle;"_s);
        replaceQuery.bindValue(u":pickle"_s, QString::fromLatin1({pickle.data(), (qsizetype) pickle.size()}));
        replaceQuery.bindValue(u":olmPickle"_s, olmPickle.toLatin1());
        execute(replaceQuery);
    }
    execute(QStringLiteral("pragma user_version = 11;"));
    commit();
}


void Database::storeOlmAccount(const QOlmAccount& olmAccount)
{
    auto deleteQuery = prepareQuery(u"DELETE FROM accounts;"_s);
    auto query = prepareQuery(u"INSERT INTO accounts(pickle) VALUES(:pickle);"_s);
    query.bindValue(u":pickle"_s, olmAccount.pickle(m_picklingKey));
    transaction();
    execute(deleteQuery);
    execute(query);
    commit();
}

std::pair<QOlmAccount*, bool> Database::setupOlmAccount(const QString& userId, const QString& deviceId)
{
    auto query = prepareQuery(u"SELECT pickle FROM accounts;"_s);
    execute(query);
    if (query.next())
        return {QOlmAccount::unpickle(query.value(QStringLiteral("pickle")).toByteArray(), m_picklingKey, userId, deviceId, nullptr), false};
    return {QOlmAccount::newAccount(nullptr/*TODO*/, userId, deviceId), true};
}

void Database::clear()
{
    // SQLite driver only supports one query at a time, so feed them one by one
    transaction();
    for (auto&& q : { u"DELETE FROM accounts;"_s, // @clang-format: one per line, plz
                      u"DELETE FROM olm_sessions;"_s, //
                      u"DELETE FROM inbound_megolm_sessions;"_s, //
                      u"DELETE FROM group_session_record_index;"_s, //
                      u"DELETE FROM master_keys;"_s, //
                      u"DELETE FROM self_signing_keys;"_s, //
                      u"DELETE FROM user_signing_keys;"_s })
        execute(q);
    commit();

}

void Database::saveOlmSession(const QByteArray& senderKey,
                              const QOlmSession& session,
                              const QDateTime& timestamp)
{
    auto query = prepareQuery(u"INSERT INTO olm_sessions(senderKey, sessionId, pickle, lastReceived) VALUES(:senderKey, :sessionId, :pickle, :lastReceived);"_s);
    query.bindValue(u":senderKey"_s, senderKey);
    query.bindValue(u":sessionId"_s, session.sessionId());
    query.bindValue(u":pickle"_s, session.pickle(m_picklingKey));
    query.bindValue(u":lastReceived"_s, timestamp);
    transaction();
    execute(query);
    commit();
}

std::unordered_map<QByteArray, std::vector<QOlmSession> > Database::loadOlmSessions()
{
    auto query = prepareQuery(u"SELECT * FROM olm_sessions ORDER BY lastReceived DESC;"_s);
    transaction();
    execute(query);
    commit();
    std::unordered_map<QByteArray, std::vector<QOlmSession>> sessions;
    while (query.next()) {
        if (auto&& expectedSession =
                QOlmSession::unpickle(query.value(u"pickle"_s).toByteArray(), m_picklingKey)) {
            sessions[query.value(u"senderKey"_s).toByteArray()].emplace_back(
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
    auto query = prepareQuery(u"SELECT * FROM inbound_megolm_sessions WHERE roomId=:roomId;"_s);
    query.bindValue(u":roomId"_s, roomId);
    transaction();
    execute(query);
    commit();
    decltype(Database::loadMegolmSessions({})) sessions;
    while (query.next()) {
        if (auto&& expectedSession = QOlmInboundGroupSession::unpickle(
                query.value(u"pickle"_s).toByteArray(), m_picklingKey)) {
            const auto sessionId = query.value(u"sessionId"_s).toByteArray();
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
                query.value(u"olmSessionId"_s).toByteArray());
            expectedSession->setSenderId(query.value(u"senderId"_s).toString());
            sessions.try_emplace(query.value(u"sessionId"_s).toByteArray(),
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
    auto deleteQuery = prepareQuery(u"DELETE FROM inbound_megolm_sessions WHERE roomId=:roomId AND sessionId=:sessionId;"_s);
    deleteQuery.bindValue(u":roomId"_s, roomId);
    deleteQuery.bindValue(u":sessionId"_s, session.sessionId());
    auto query = prepareQuery(
        u"INSERT INTO inbound_megolm_sessions(roomId, sessionId, pickle, senderId, olmSessionId, senderKey, senderClaimedEd25519Key) VALUES(:roomId, :sessionId, :pickle, :senderId, :olmSessionId, :senderKey, :senderClaimedEd25519Key);"_s);
    query.bindValue(u":roomId"_s, roomId);
    query.bindValue(u":sessionId"_s, session.sessionId());
    query.bindValue(u":pickle"_s, session.pickle(m_picklingKey));
    query.bindValue(u":senderId"_s, session.senderId());
    query.bindValue(u":olmSessionId"_s, session.olmSessionId());
    query.bindValue(u":senderKey"_s, senderKey);
    query.bindValue(u":senderClaimedEd25519Key"_s, senderClaimedEdKey);
    transaction();
    execute(deleteQuery);
    execute(query);
    commit();
}

void Database::addGroupSessionIndexRecord(const QString& roomId, const QString& sessionId, uint32_t index, const QString& eventId, qint64 ts)
{
    auto query = prepareQuery(u"INSERT INTO group_session_record_index(roomId, sessionId, i, eventId, ts) VALUES(:roomId, :sessionId, :index, :eventId, :ts);"_s);
    query.bindValue(u":roomId"_s, roomId);
    query.bindValue(u":sessionId"_s, sessionId);
    query.bindValue(u":index"_s, index);
    query.bindValue(u":eventId"_s, eventId);
    query.bindValue(u":ts"_s, ts);
    transaction();
    execute(query);
    commit();
}

std::pair<QString, qint64> Database::groupSessionIndexRecord(const QString& roomId, const QString& sessionId, qint64 index)
{
    auto query = prepareQuery(u"SELECT * FROM group_session_record_index WHERE roomId=:roomId AND sessionId=:sessionId AND i=:index;"_s);
    query.bindValue(u":roomId"_s, roomId);
    query.bindValue(u":sessionId"_s, sessionId);
    query.bindValue(u":index"_s, index);
    transaction();
    execute(query);
    commit();
    if (!query.next()) {
        return {};
    }
    return {query.value(u"eventId"_s).toString(), query.value(u"ts"_s).toLongLong()};
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
         { u"DELETE FROM inbound_megolm_sessions WHERE roomId=:roomId;"_s,
           u"DELETE FROM outbound_megolm_sessions WHERE roomId=:roomId;"_s,
           u"DELETE FROM group_session_record_index WHERE roomId=:roomId;"_s }) {
        auto q = prepareQuery(queryText);
        q.bindValue(u":roomId"_s, roomId);
        execute(q);
    }
    commit();
}

void Database::setOlmSessionLastReceived(const QByteArray& sessionId, const QDateTime& timestamp)
{
    auto query = prepareQuery(u"UPDATE olm_sessions SET lastReceived=:lastReceived WHERE sessionId=:sessionId;"_s);
    query.bindValue(u":lastReceived"_s, timestamp);
    query.bindValue(u":sessionId"_s, sessionId);
    transaction();
    execute(query);
    commit();
}

void Database::saveCurrentOutboundMegolmSession(const QString& roomId,
    const QOlmOutboundGroupSession& session)
{
    const auto pickle = session.pickle(m_picklingKey);
    auto deleteQuery = prepareQuery(
        u"DELETE FROM outbound_megolm_sessions WHERE roomId=:roomId AND sessionId=:sessionId;"_s);
    deleteQuery.bindValue(u":roomId"_s, roomId);
    deleteQuery.bindValue(u":sessionId"_s, session.sessionId());

    auto insertQuery = prepareQuery(
        u"INSERT INTO outbound_megolm_sessions(roomId, sessionId, pickle, creationTime, messageCount) VALUES(:roomId, :sessionId, :pickle, :creationTime, :messageCount);"_s);
    insertQuery.bindValue(u":roomId"_s, roomId);
    insertQuery.bindValue(u":sessionId"_s, session.sessionId());
    insertQuery.bindValue(u":pickle"_s, pickle);
    insertQuery.bindValue(u":creationTime"_s, session.creationTime());
    insertQuery.bindValue(u":messageCount"_s, session.messageCount());

    transaction();
    execute(deleteQuery);
    execute(insertQuery);
    commit();
}

std::optional<QOlmOutboundGroupSession> Database::loadCurrentOutboundMegolmSession(
    const QString& roomId)
{
    auto query = prepareQuery(
        u"SELECT * FROM outbound_megolm_sessions WHERE roomId=:roomId ORDER BY creationTime DESC;"_s);
    query.bindValue(u":roomId"_s, roomId);
    execute(query);
    if (query.next()) {
        if (auto&& session =
                QOlmOutboundGroupSession::unpickle(query.value(u"pickle"_s).toByteArray(),
                                                   m_picklingKey)) {
            session->setCreationTime(query.value(u"creationTime"_s).toDateTime());
            session->setMessageCount(query.value(u"messageCount"_s).toInt());
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
        auto query = prepareQuery(u"INSERT INTO sent_megolm_sessions(roomId, userId, deviceId, identityKey, sessionId, i) VALUES(:roomId, :userId, :deviceId, :identityKey, :sessionId, :i);"_s);
        query.bindValue(u":roomId"_s, roomId);
        query.bindValue(u":userId"_s, user);
        query.bindValue(u":deviceId"_s, device);
        query.bindValue(u":identityKey"_s, curveKey);
        query.bindValue(u":sessionId"_s, sessionId);
        query.bindValue(u":i"_s, index);
        execute(query);
    }
    commit();
}

QMultiHash<QString, QString> Database::devicesWithoutKey(
    const QString& roomId, QMultiHash<QString, QString> devices,
    const QByteArray& sessionId)
{
    auto query = prepareQuery(u"SELECT userId, deviceId FROM sent_megolm_sessions WHERE roomId=:roomId AND sessionId=:sessionId"_s);
    query.bindValue(u":roomId"_s, roomId);
    query.bindValue(u":sessionId"_s, sessionId);
    transaction();
    execute(query);
    commit();
    while (query.next())
        devices.remove(query.value(u"userId"_s).toString(), query.value(u"deviceId"_s).toString());

    return devices;
}

void Database::updateOlmSession(const QByteArray& senderKey,
                                const QOlmSession& session)
{
    auto query = prepareQuery(
        u"UPDATE olm_sessions SET pickle=:pickle WHERE senderKey=:senderKey AND sessionId=:sessionId;"_s);
    query.bindValue(u":pickle"_s, session.pickle(m_picklingKey));
    query.bindValue(u":senderKey"_s, senderKey);
    query.bindValue(u":sessionId"_s, session.sessionId());
    transaction();
    execute(query);
    commit();
}

void Database::setSessionVerified(const QString& edKeyId)
{
    auto query = prepareQuery(u"UPDATE tracked_devices SET verified=true WHERE edKeyId=:edKeyId;"_s);
    query.bindValue(u":edKeyId"_s, edKeyId);
    transaction();
    execute(query);
    commit();
}

bool Database::isSessionVerified(const QString& edKey)
{
    auto query = prepareQuery(u"SELECT verified FROM tracked_devices WHERE edKey=:edKey"_s);
    query.bindValue(u":edKey"_s, edKey);
    execute(query);
    return query.next() && query.value(u"verified"_s).toBool();
}

QString Database::edKeyForKeyId(const QString& userId, const QString& edKeyId)
{
    auto query = prepareQuery(u"SELECT edKey FROM tracked_devices WHERE matrixId=:userId and edKeyId=:edKeyId;"_s);
    query.bindValue(u":matrixId"_s, userId);
    query.bindValue(u":edKeyId"_s, edKeyId);
    execute(query);
    if (!query.next()) {
        return {};
    }
    return query.value(u"edKey"_s).toString();
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
    auto query = prepareQuery(u"INSERT INTO encrypted(name, cipher, iv) VALUES(:name, :cipher, :iv);"_s);
    auto deleteQuery = prepareQuery(u"DELETE FROM encrypted WHERE name=:name;"_s);
    deleteQuery.bindValue(u":name"_s, name);
    query.bindValue(u":name"_s, name);
    query.bindValue(u":cipher"_s, cipher);
    query.bindValue(u":iv"_s, iv.toBase64());
    transaction();
    execute(deleteQuery);
    execute(query);
    commit();
}

QByteArray Database::loadEncrypted(const QString& name)
{
    auto query = prepareQuery(u"SELECT cipher, iv FROM encrypted WHERE name=:name;"_s);
    query.bindValue(u":name"_s, name);
    execute(query);
    if (!query.next()) {
        return {};
    }
    auto cipher = QByteArray::fromBase64(query.value(u"cipher"_s).toString().toLatin1());
    auto iv = QByteArray::fromBase64(query.value(u"iv"_s).toString().toLatin1());
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
    auto query = prepareQuery(u"UPDATE master_keys SET verified=true WHERE key=:key;"_s);
    query.bindValue(u":key"_s, masterKey);
    transaction();
    execute(query);
    commit();
}

QString Database::userSigningPublicKey()
{
    auto query = prepareQuery(u"SELECT key FROM user_signing_keys WHERE userId=:userId;"_s);
    query.bindValue(u":userId"_s, m_userId);
    execute(query);
    return query.next() ? query.value(u"key"_s).toString() : QString();
}

QString Database::selfSigningPublicKey()
{
    auto query = prepareQuery(u"SELECT key FROM self_signing_keys WHERE userId=:userId;"_s);
    query.bindValue(u":userId"_s, m_userId);
    execute(query);
    return query.next() ? query.value(u"key"_s).toString() : QString();
}

QString Database::edKeyForMegolmSession(const QString& sessionId)
{
    auto query = prepareQuery(u"SELECT senderClaimedEd25519Key FROM inbound_megolm_sessions WHERE sessionId=:sessionId;"_s);
    query.bindValue(u":sessionId"_s, sessionId.toLatin1());
    execute(query);
    return query.next() ? query.value(u"senderClaimedEd25519Key"_s).toString() : QString();
}

QString Database::senderKeyForMegolmSession(const QString& sessionId)
{
    auto query = prepareQuery(u"SELECT senderKey FROM inbound_megolm_sessions WHERE sessionId=:sessionId;"_s);
    query.bindValue(u":sessionId"_s, sessionId.toLatin1());
    execute(query);
    return query.next() ? query.value(u"senderKey"_s).toString() : QString();
}
