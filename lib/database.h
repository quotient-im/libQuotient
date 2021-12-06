// SPDX-FileCopyrightText: 2021 Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtCore/QObject>
#include <QtSql/QSqlQuery>
#include <QtCore/QVector>

#include "crypto/e2ee.h"

namespace Quotient {
class Database : public QObject
{
    Q_OBJECT

public:
    static Database &instance()
    {
        static Database _instance;
        return _instance;
    }

    int version();
    void transaction();
    void commit();
    QSqlQuery execute(const QString &queryString);
    QSqlQuery execute(QSqlQuery &query);

    QByteArray accountPickle(const QString &id);
    void setAccountPickle(const QString &id, const QByteArray &pickle);
    void clear(const QString &id);
    void saveOlmSession(const QString& matrixId, const QString& senderKey, const QString& sessionId, const QByteArray &pickle);
    UnorderedMap<QString, std::vector<QOlmSessionPtr>> loadOlmSessions(const QString& matrixId, const PicklingMode& picklingMode);
    UnorderedMap<QPair<QString, QString>, QOlmInboundGroupSessionPtr> loadMegolmSessions(const QString& matrixId, const QString& roomId, const PicklingMode& picklingMode);
    void saveMegolmSession(const QString& matrixId, const QString& roomId, const QString& senderKey, const QString& sessionKey, const QByteArray& pickle);
    void addGroupSessionIndexRecord(const QString& matrixId, const QString& roomId, const QString& sessionId, uint32_t index, const QString& eventId, qint64 ts);
    QPair<QString, qint64> groupSessionIndexRecord(const QString& matrixId, const QString& roomId, const QString& sessionId, qint64 index);


private:
    Database();

    void migrateTo1();
};
}
