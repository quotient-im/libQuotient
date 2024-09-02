#pragma once

#include "connection.h"
#include "database.h"
#include "logging_categories_p.h"

#include "e2ee/qolmaccount.h"
#include "e2ee/qolmsession.h"

#include "events/encryptedevent.h"

namespace Quotient {

struct DevicesList;

namespace _impl {
    class ConnectionEncryptionData {
    public:
        static QFuture<bool> setup(Connection* connection, bool mock,
                                   std::unique_ptr<ConnectionEncryptionData>& result);

        Connection* q;
        QOlmAccount* olmAccount;
        // No easy way in C++ to discern between SQL SELECT from UPDATE, too bad
        mutable Database database;
        std::unordered_map<QByteArray, std::vector<QOlmSession>> olmSessions;
        //! A map from SenderKey to vector of InboundSession
        QHash<QString, KeyVerificationSession*> verificationSessions{};
        QSet<QString> trackedUsers{};
        QSet<QString> outdatedUsers{};
        QHash<QString, QHash<QString, DeviceKeys>> deviceKeys{};
        JobHandle<QueryKeysJob> currentQueryKeysJob{};
        QSet<std::pair<QString, QString>> triedDevices{};
        //! An update of internal tracking structures (trackedUsers, e.g.) is
        //! needed
        bool encryptionUpdateRequired = false;
        QHash<QString, int> oneTimeKeysCount{};
        std::vector<std::unique_ptr<EncryptedEvent>> pendingEncryptedEvents{};
        bool isUploadingKeys = false;
        bool firstSync = true;
        QHash<QString, QHash<QString, bool>> selfVerifiedDevices;
        QHash<QString, QHash<QString, bool>> verifiedDevices;

        void saveDevicesList();
        void loadDevicesList();
        QString curveKeyForUserDevice(const QString& userId,
                                      const QString& device) const;
        bool isKnownCurveKey(const QString& userId,
                             const QString& curveKey) const;
        bool hasOlmSession(const QString &user, const QString &deviceId) const;

        void onSyncSuccess(SyncData &syncResponse);
        void loadOutdatedUserDevices();
        void consumeToDeviceEvent(EventPtr toDeviceEvent);
        void encryptionUpdate(const QList<QString>& forUsers);

        bool createOlmSession(const QString& targetUserId,
                              const QString& targetDeviceId,
                              const OneTimeKeys& oneTimeKeyObject);
        void saveSession(const QOlmSession& session, const QByteArray& senderKey)
        {
            database.saveOlmSession(senderKey, session,
                                    QDateTime::currentDateTime());
        }
        void saveOlmAccount();
        void reloadDevices();

        std::pair<QByteArray, QByteArray> sessionDecryptMessage(
            const QJsonObject& personalCipherObject,
            const QByteArray& senderKey);
        std::pair<EventPtr, QByteArray> sessionDecryptMessage(const EncryptedEvent& encryptedEvent);

        QJsonObject assembleEncryptedContent(
            QJsonObject payloadJson, const QString& targetUserId,
            const QString& targetDeviceId);
        void sendSessionKeyToDevices(
            const QString& roomId,
            const QOlmOutboundGroupSession& outboundSession,
            const QMultiHash<QString, QString>& devices);

        template <typename... ArgTs>
        KeyVerificationSession* setupKeyVerificationSession(
            ArgTs&&... sessionArgs)
        {
            auto session =
                new KeyVerificationSession(std::forward<ArgTs>(sessionArgs)...);
            qCDebug(E2EE) << "Incoming key verification session from" << session->remoteDeviceId();
            verificationSessions.insert(session->transactionId(), session);
            QObject::connect(session, &QObject::destroyed, q,
                             [this, txnId = session->transactionId()] {
                                 verificationSessions.remove(txnId);
                             });
            emit q->newKeyVerificationSession(session);
            return session;
        }

        // This is only public to enable std::make_unique; do not use directly,
        // get an instance from setup() instead
        ConnectionEncryptionData(Connection* connection,
                                 PicklingKey&& picklingKey);
        bool hasConflictingDeviceIdsAndCrossSigningKeys(const QString& userId);

        void handleQueryKeys(const QueryKeysJob::Response& keys);

        void handleMasterKeys(const QHash<QString, CrossSigningKey>& masterKeys);
        void handleSelfSigningKeys(const QHash<QString, CrossSigningKey>& selfSigningKeys);
        void handleUserSigningKeys(const QHash<QString, CrossSigningKey>& userSigningKeys);
        void handleDevicesList(
            const QHash<QString, QHash<QString, QueryKeysJob::DeviceInformation>>& newDeviceKeys);
        void checkVerifiedMasterKeys(const QHash<QString, CrossSigningKey>& masterKeys);

    private:
        void consumeDevicesList(const DevicesList &devicesList);
        bool processIfVerificationEvent(const Event& evt, bool encrypted);
        void handleEncryptedToDeviceEvent(const EncryptedEvent& event);

        // This function assumes that an olm session with (user, device) exists
        std::pair<size_t, QByteArray> olmEncryptMessage(
            const QString& userId, const QString& device,
            const QByteArray& message);

        void doSendSessionKeyToDevices(const QString& roomId, const QByteArray& sessionId,
            const QByteArray &sessionKey, uint32_t messageIndex,
            const QMultiHash<QString, QString>& devices);
    };
} // namespace _impl
} // namespace Quotient
