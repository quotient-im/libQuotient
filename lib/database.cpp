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
Database::Database()
{
    QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    QString databasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(databasePath).mkpath(databasePath);
    QSqlDatabase::database().setDatabaseName(databasePath + QStringLiteral("/database.db3"));
    QSqlDatabase::database().open();

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
        qDebug() << "Database version" << value;
        if (ok)
            return value;
    } else {
        qCritical() << "Failed to check database version";
    }
    return -1;
}

QSqlQuery Database::execute(const QString &queryString)
{
    auto query = QSqlDatabase::database().exec(queryString);
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
    QSqlDatabase::database().transaction();
}

void Database::commit()
{
    QSqlDatabase::database().commit();
}

void Database::migrateTo1()
{
    qDebug() << "Migrating database to version 1";
    transaction();
    execute(QStringLiteral("CREATE TABLE accounts (matrixId TEXT UNIQUE, pickle TEXT);"));
    execute(QStringLiteral("CREATE TABLE olm_sessions (matrixId TEXT, senderKey TEXT, sessionId TEXT, pickle TEXT);"));
    execute(QStringLiteral("CREATE TABLE inbound_megolm_sessions (matrixId TEXT, roomId TEXT, senderKey TEXT, sessionId TEXT, pickle TEXT);"));
    execute(QStringLiteral("CREATE TABLE OutboundMegolmSessions (matrixId TEXT, roomId TEXT, senderKey TEXT, sessionId TEXT, pickle TEXT);"));
    execute(QStringLiteral("CREATE TABLE group_session_record_index (matrixId TEXT, roomId TEXT, sessionId TEXT, i INTEGER, eventId TEXT, ts INTEGER);"));
    execute(QStringLiteral("PRAGMA user_version = 1;"));
    commit();
}

QByteArray Database::accountPickle(const QString &matrixId)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT pickle FROM accounts WHERE matrixId=:matrixId;"));
    query.bindValue(":matrixId", matrixId);
    execute(query);
    if (query.next()) {
        return query.value(QStringLiteral("pickle")).toByteArray();
    }
    return {};
}

void Database::setAccountPickle(const QString &matrixId, const QByteArray &pickle)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO accounts(matrixId, pickle) VALUES(:matrixId, :pickle) ON CONFLICT (matrixId) DO UPDATE SET pickle=:pickle WHERE matrixId=:matrixId;"));
    query.bindValue(":matrixId", matrixId);
    query.bindValue(":pickle", pickle);
    transaction();
    execute(query);
    commit();
}

void Database::clear(const QString &matrixId)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM accounts WHERE matrixId=:matrixId;"));
    query.bindValue(":matrixId", matrixId);

    QSqlQuery sessionsQuery;
    sessionsQuery.prepare(QStringLiteral("DELETE FROM olm_sessions WHERE matrixId=:matrixId;"));
    sessionsQuery.bindValue(":matrixId", matrixId);

    QSqlQuery megolmSessionsQuery;
    megolmSessionsQuery.prepare(QStringLiteral("DELETE FROM inbound_megolm_sessions WHERE matrixId=:matrixId;"));
    megolmSessionsQuery.bindValue(":matrixId", matrixId);

    QSqlQuery groupSessionIndexRecordQuery;
    groupSessionIndexRecordQuery.prepare(QStringLiteral("DELETE FROM group_session_record_index WHERE matrixId=:matrixId;"));
    groupSessionIndexRecordQuery.bindValue(":matrixId", matrixId);

    transaction();
    execute(query);
    execute(sessionsQuery);
    execute(megolmSessionsQuery);
    execute(groupSessionIndexRecordQuery);
    commit();

}

void Database::saveOlmSession(const QString& matrixId, const QString& senderKey, const QString& sessionId, const QByteArray &pickle)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO olm_sessions(matrixId, senderKey, sessionId, pickle) VALUES(:matrixId, :senderKey, :sessionId, :pickle);"));
    query.bindValue(":matrixId", matrixId);
    query.bindValue(":senderKey", senderKey);
    query.bindValue(":sessionId", sessionId);
    query.bindValue(":pickle", pickle);
    transaction();
    execute(query);
    commit();
}

UnorderedMap<QString, std::vector<QOlmSessionPtr>> Database::loadOlmSessions(const QString& matrixId, const PicklingMode& picklingMode)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM olm_sessions WHERE matrixId=:matrixId;"));
    query.bindValue(":matrixId", matrixId);
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

UnorderedMap<QPair<QString, QString>, QOlmInboundGroupSessionPtr> Database::loadMegolmSessions(const QString& matrixId, const QString& roomId, const PicklingMode& picklingMode)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM inbound_megolm_sessions WHERE matrixId=:matrixId AND roomId=:roomId;"));
    query.bindValue(":matrixId", matrixId);
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

void Database::saveMegolmSession(const QString& matrixId, const QString& roomId, const QString& senderKey, const QString& sessionId, const QByteArray& pickle)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO inbound_megolm_sessions(matrixId, roomId, senderKey, sessionId, pickle) VALUES(:matrixId, :roomId, :senderKey, :sessionId, :pickle);"));
    query.bindValue(":matrixId", matrixId);
    query.bindValue(":roomId", roomId);
    query.bindValue(":senderKey", senderKey);
    query.bindValue(":sessionId", sessionId);
    query.bindValue(":pickle", pickle);
    transaction();
    execute(query);
    commit();
}

void Database::addGroupSessionIndexRecord(const QString& matrixId, const QString& roomId, const QString& sessionId, uint32_t index, const QString& eventId, qint64 ts)
{
    QSqlQuery query;
    query.prepare("INSERT INTO group_session_record_index(matrixId, roomId, sessionId, i, eventId, ts) VALUES(:matrixId, :roomId, :sessionId, :index, :eventId, :ts);");
    query.bindValue(":matrixId", matrixId);
    query.bindValue(":roomId", roomId);
    query.bindValue(":sessionId", sessionId);
    query.bindValue(":index", index);
    query.bindValue(":eventId", eventId);
    query.bindValue(":ts", ts);
    transaction();
    execute(query);
    commit();
}

QPair<QString, qint64> Database::groupSessionIndexRecord(const QString& matrixId, const QString& roomId, const QString& sessionId, qint64 index)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM group_session_record_index WHERE matrixId=:matrixId AND roomId=:roomId AND sessionId=:sessionId AND i=:index;"));
    query.bindValue(":matrixId", matrixId);
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
