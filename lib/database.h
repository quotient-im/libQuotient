// SPDX-FileCopyrightText: 2021 Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtCore/QObject>
#include <QtSql/QSqlQuery>
#include <QtCore/QVector>

#include <QtCore/QHash>

#include "e2ee/e2ee_common.h"

namespace Quotient {

class QOlmAccount;
class QOlmSession;
class QOlmInboundGroupSession;
class QOlmOutboundGroupSession;

class QUOTIENT_API Database : public QObject
{
    Q_OBJECT
public:
    Database(const QString& userId, const QString& deviceId,
             PicklingKey&& picklingKey, QObject* parent);

    int version();
    void transaction();
    void commit();
    QSqlQuery execute(const QString &queryString);
    QSqlQuery execute(QSqlQuery &query);
    QSqlDatabase database();
    QSqlQuery prepareQuery(const QString& quaryString);

    void storeOlmAccount(const QOlmAccount& olmAccount);
    Omittable<OlmErrorCode> setupOlmAccount(QOlmAccount &olmAccount);
    void clear();
    void saveOlmSession(const QString& senderKey, const QOlmSession& session,
                        const QDateTime& timestamp);
    UnorderedMap<QString, std::vector<QOlmSession>> loadOlmSessions();
    UnorderedMap<QString, QOlmInboundGroupSession> loadMegolmSessions(
        const QString& roomId);
    void saveMegolmSession(const QString& roomId,
                           const QOlmInboundGroupSession& session);
    void addGroupSessionIndexRecord(const QString& roomId,
                                    const QString& sessionId, uint32_t index,
                                    const QString& eventId, qint64 ts);
    std::pair<QString, qint64> groupSessionIndexRecord(const QString& roomId,
                                                       const QString& sessionId,
                                                       qint64 index);
    void clearRoomData(const QString& roomId);
    void setOlmSessionLastReceived(const QString& sessionId,
                                   const QDateTime& timestamp);
    Omittable<QOlmOutboundGroupSession> loadCurrentOutboundMegolmSession(
        const QString& roomId);
    void saveCurrentOutboundMegolmSession(
        const QString& roomId, const QOlmOutboundGroupSession& session);
    void updateOlmSession(const QString& senderKey, const QOlmSession& session);

    // Returns a map UserId -> [DeviceId] that have not received key yet
    QMultiHash<QString, QString> devicesWithoutKey(
        const QString& roomId, QMultiHash<QString, QString> devices,
        const QString& sessionId);
    // 'devices' contains tuples {userId, deviceId, curveKey}
    void setDevicesReceivedKey(
        const QString& roomId,
        const QVector<std::tuple<QString, QString, QString>>& devices,
        const QString& sessionId, int index);

    bool isSessionVerified(const QString& edKey);
    void setSessionVerified(const QString& edKeyId);

private:
    void migrateTo1();
    void migrateTo2();
    void migrateTo3();
    void migrateTo4();
    void migrateTo5();

    QString m_userId;
    QString m_deviceId;
    PicklingKey m_picklingKey;
};
} // namespace Quotient
