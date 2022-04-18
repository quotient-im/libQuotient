// SPDX-FileCopyrightText: 2021 Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtCore/QObject>
#include <QtSql/QSqlQuery>
#include <QtCore/QVector>

#include <QtCore/QHash>

#include "e2ee/e2ee.h"

#include "e2ee/qolmoutboundsession.h"

namespace Quotient {
class User;
class Room;

class QUOTIENT_API Database : public QObject
{
    Q_OBJECT
public:
    Database(const QString& matrixId, const QString& deviceId, QObject* parent);

    int version();
    void transaction();
    void commit();
    QSqlQuery execute(const QString &queryString);
    QSqlQuery execute(QSqlQuery &query);
    QSqlDatabase database();
    QSqlQuery prepareQuery(const QString& quaryString);

    QByteArray accountPickle();
    void setAccountPickle(const QByteArray &pickle);
    void clear();
    void saveOlmSession(const QString& senderKey, const QString& sessionId, const QByteArray& pickle, const QDateTime& timestamp);
    UnorderedMap<QString, std::vector<QOlmSessionPtr>> loadOlmSessions(const PicklingMode& picklingMode);
    UnorderedMap<QString, QOlmInboundGroupSessionPtr> loadMegolmSessions(const QString& roomId, const PicklingMode& picklingMode);
    void saveMegolmSession(const QString& roomId, const QString& sessionId, const QByteArray& pickle, const QString& senderId, const QString& olmSessionId);
    void addGroupSessionIndexRecord(const QString& roomId, const QString& sessionId, uint32_t index, const QString& eventId, qint64 ts);
    std::pair<QString, qint64> groupSessionIndexRecord(const QString& roomId, const QString& sessionId, qint64 index);
    void clearRoomData(const QString& roomId);
    void setOlmSessionLastReceived(const QString& sessionId, const QDateTime& timestamp);
    QOlmOutboundGroupSessionPtr loadCurrentOutboundMegolmSession(const QString& roomId, const PicklingMode& picklingMode);
    void saveCurrentOutboundMegolmSession(const QString& roomId, const PicklingMode& picklingMode, const QOlmOutboundGroupSessionPtr& data);
    void updateOlmSession(const QString& senderKey, const QString& sessionId, const QByteArray& pickle);

    // Returns a map User -> [Device] that have not received key yet
    QHash<QString, QStringList> devicesWithoutKey(Room* room, const QString &sessionId);
    void setDevicesReceivedKey(const QString& roomId, QHash<User *, QStringList> devices, const QString& sessionId, int index);

    bool isSessionVerified(const QString& edKey);
    void setSessionVerified(const QString& edKeyId);

private:
    void migrateTo1();
    void migrateTo2();
    void migrateTo3();
    void migrateTo4();
    void migrateTo5();

    QString m_matrixId;
};
}
