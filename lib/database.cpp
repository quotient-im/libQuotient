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

//TODO: delete room specific data when leaving room

using namespace Quotient;
Database::Database(const QString& matrixId, QObject* parent)
    : QObject(parent)
    , m_matrixId(matrixId)
{
    m_matrixId.replace(':', '_');
    QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("Quotient_%1").arg(m_matrixId));
    QString databasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/%1").arg(m_matrixId);
    QDir(databasePath).mkpath(databasePath);
    database().setDatabaseName(databasePath + QStringLiteral("/quotient.db3"));
    database().open();

    switch(version()) {
        case 0: migrateTo1();
    }
}

int Database::version()
{
    auto query = execute(QStringLiteral("PRAGMA user_version;"));
    if (query.next()) {
        bool ok;
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
    execute(QStringLiteral("PRAGMA user_version = 1;"));
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

void Database::saveOlmSession(const QString& senderKey, const QString& sessionId, const QByteArray &pickle)
{
    auto query = prepareQuery(QStringLiteral("INSERT INTO olm_sessions(senderKey, sessionId, pickle) VALUES(:senderKey, :sessionId, :pickle);"));
    query.bindValue(":senderKey", senderKey);
    query.bindValue(":sessionId", sessionId);
    query.bindValue(":pickle", pickle);
    transaction();
    execute(query);
    commit();
}

UnorderedMap<QString, std::vector<QOlmSessionPtr>> Database::loadOlmSessions(const PicklingMode& picklingMode)
{
    QSqlQuery query = prepareQuery(QStringLiteral("SELECT * FROM olm_sessions;"));
    transaction();
    execute(query);
    commit();
    UnorderedMap<QString, std::vector<QOlmSessionPtr>> sessions;
    while (query.next()) {
        auto session = QOlmSession::unpickle(query.value("pickle").toByteArray(), picklingMode);
        if (std::holds_alternative<QOlmError>(session)) {
            qCWarning(E2EE) << "Failed to unpickle olm session";
            continue;
        }
        sessions[query.value("senderKey").toString()].push_back(std::move(std::get<QOlmSessionPtr>(session)));
    }
    return sessions;
}

UnorderedMap<QPair<QString, QString>, QOlmInboundGroupSessionPtr> Database::loadMegolmSessions(const QString& roomId, const PicklingMode& picklingMode)
{
    auto query = prepareQuery(QStringLiteral("SELECT * FROM inbound_megolm_sessions WHERE roomId=:roomId;"));
    query.bindValue(":roomId", roomId);
    transaction();
    execute(query);
    commit();
    UnorderedMap<QPair<QString, QString>, QOlmInboundGroupSessionPtr> sessions;
    while (query.next()) {
        auto session = QOlmInboundGroupSession::unpickle(query.value("pickle").toByteArray(), picklingMode);
        if (std::holds_alternative<QOlmError>(session)) {
            qCWarning(E2EE) << "Failed to unpickle megolm session";
            continue;
        }
        sessions[{query.value("senderKey").toString(), query.value("sessionId").toString()}] = std::move(std::get<QOlmInboundGroupSessionPtr>(session));
    }
    return sessions;
}

void Database::saveMegolmSession(const QString& roomId, const QString& senderKey, const QString& sessionId, const QByteArray& pickle)
{
    auto query = prepareQuery(QStringLiteral("INSERT INTO inbound_megolm_sessions(roomId, senderKey, sessionId, pickle) VALUES(:roomId, :senderKey, :sessionId, :pickle);"));
    query.bindValue(":roomId", roomId);
    query.bindValue(":senderKey", senderKey);
    query.bindValue(":sessionId", sessionId);
    query.bindValue(":pickle", pickle);
    transaction();
    execute(query);
    commit();
}

void Database::addGroupSessionIndexRecord(const QString& roomId, const QString& sessionId, uint32_t index, const QString& eventId, qint64 ts)
{
    QSqlQuery query = prepareQuery("INSERT INTO group_session_record_index(roomId, sessionId, i, eventId, ts) VALUES(:roomId, :sessionId, :index, :eventId, :ts);");
    query.bindValue(":roomId", roomId);
    query.bindValue(":sessionId", sessionId);
    query.bindValue(":index", index);
    query.bindValue(":eventId", eventId);
    query.bindValue(":ts", ts);
    transaction();
    execute(query);
    commit();
}

QPair<QString, qint64> Database::groupSessionIndexRecord(const QString& roomId, const QString& sessionId, qint64 index)
{
    QSqlQuery query(database());
    query.prepare(QStringLiteral("SELECT * FROM group_session_record_index WHERE roomId=:roomId AND sessionId=:sessionId AND i=:index;"));
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
