// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2017 Roman Plášil <me@rplasil.name>
// SPDX-FileCopyrightText: 2019 Ville Ranki <ville.ranki@iki.fi>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "connection.h"
#include "connectiondata.h"
#include "settings.h"
#include "syncdata.h"

#include "csapi/capabilities.h"
#include "csapi/logout.h"
#include "csapi/wellknown.h"

#ifdef Quotient_E2EE_ENABLED
#include "e2ee/qolmaccount.h"
#include "database.h"
#endif

#include "csapi/account-data.h"

#include "events/encryptedevent.h"

#include <QCoreApplication>
#include <QPointer>

class Q_DECL_HIDDEN Quotient::Connection::Private {
public:
    explicit Private(std::unique_ptr<ConnectionData>&& connection)
        : data(std::move(connection))
    {}

    Connection* q = nullptr;
    std::unique_ptr<ConnectionData> data;
    // A complex key below is a pair of room name and whether its
    // state is Invited. The spec mandates to keep Invited room state
    // separately; specifically, we should keep objects for Invite and
    // Leave state of the same room if the two happen to co-exist.
    QHash<std::pair<QString, bool>, Room*> roomMap;
    /// Mapping from serverparts to alias/room id mappings,
    /// as of the last sync
    QHash<QString, QString> roomAliasMap;
    QVector<QString> roomIdsToForget;
    QVector<QString> pendingStateRoomIds;
    QMap<QString, User*> userMap;
    DirectChatsMap directChats;
    DirectChatUsersMap directChatUsers;
    // The below two variables track local changes between sync completions.
    // See https://github.com/quotient-im/libQuotient/wiki/Handling-direct-chat-events
    DirectChatsMap dcLocalAdditions;
    DirectChatsMap dcLocalRemovals;
    UnorderedMap<QString, EventPtr> accountData;
    QMetaObject::Connection syncLoopConnection {};
    int syncTimeout = -1;

#ifdef Quotient_E2EE_ENABLED
    QSet<QString> trackedUsers;
    QSet<QString> outdatedUsers;
    QHash<QString, QHash<QString, DeviceKeys>> deviceKeys;
    QueryKeysJob *currentQueryKeysJob = nullptr;
    bool encryptionUpdateRequired = false;
    Database *database = nullptr;
    QHash<QString, int> oneTimeKeysCount;
    std::vector<std::unique_ptr<EncryptedEvent>> pendingEncryptedEvents;
    void handleEncryptedToDeviceEvent(const EncryptedEvent& event);
    template <typename... ArgTs>
    KeyVerificationSession* setupKeyVerificationSession(ArgTs&&... sessionArgs);
    bool processIfVerificationEvent(const Event &evt, bool encrypted);

    // A map from SenderKey to vector of InboundSession
    UnorderedMap<QString, std::vector<QOlmSession>> olmSessions;

    QHash<QString, KeyVerificationSession*> verificationSessions;
    QSet<std::pair<QString, QString>> triedDevices;

    std::unique_ptr<QOlmAccount> olmAccount;
    bool isUploadingKeys = false;
    bool firstSync = true;
#endif // Quotient_E2EE_ENABLED

    GetCapabilitiesJob* capabilitiesJob = nullptr;
    GetCapabilitiesJob::Capabilities capabilities;

    QVector<GetLoginFlowsJob::LoginFlow> loginFlows;


    QPointer<GetWellknownJob> resolverJob = nullptr;
    QPointer<GetLoginFlowsJob> loginFlowsJob = nullptr;

    SyncJob* syncJob = nullptr;
    QPointer<LogoutJob> logoutJob = nullptr;

    bool cacheState = true;
    bool cacheToBinary =
        SettingsGroup("libQuotient"_ls).get("cache_type"_ls,
                                            SettingsGroup("libQMatrixClient"_ls).get<QString>("cache_type"_ls))
        != "json"_ls;
    bool lazyLoading = false;

    /** \brief Check the homeserver and resolve it if needed, before connecting
     *
     * A single entry for functions that need to check whether the homeserver
     * is valid before running. May execute connectFn either synchronously
     * or asynchronously. In case of errors, emits resolveError() if
     * the homeserver URL is not valid and cannot be resolved from userId, or
     * the homeserver doesn't support the requested login flow.
     *
     * \param userId    fully-qualified MXID to resolve HS from
     * \param connectFn a function to execute once the HS URL is good
     * \param flow      optionally, a login flow that should be supported for
     *                  connectFn to work; `none`, if there's no login flow
     *                  requirements
     * \sa resolveServer, resolveError
     */
    void checkAndConnect(const QString &userId,
                         const std::function<void ()> &connectFn,
                         const std::optional<LoginFlow> &flow = none);
    template <typename... LoginArgTs>
    void loginToServer(LoginArgTs&&... loginArgs);
    void completeSetup(const QString &mxId);
    void removeRoom(const QString& roomId);

    void consumeRoomData(SyncDataList&& roomDataList, bool fromCache);
    void consumeAccountData(Events&& accountDataEvents);
    void consumePresenceData(Events&& presenceData);
    void consumeToDeviceEvents(Events&& toDeviceEvents);
    void consumeDevicesList(DevicesList&& devicesList);

    void packAndSendAccountData(EventPtr&& event)
    {
        const auto eventType = event->matrixType();
        q->callApi<SetAccountDataJob>(data->userId(), eventType,
                                      event->contentJson());
        accountData[eventType] = std::move(event);
        emit q->accountDataChanged(eventType);
    }

    template <EventClass EventT, typename ContentT>
    void packAndSendAccountData(ContentT&& content)
    {
        packAndSendAccountData(
            makeEvent<EventT>(std::forward<ContentT>(content)));
    }
    QString topLevelStatePath() const
    {
        return q->stateCacheDir().filePath("state.json"_ls);
    }

    std::pair<EventPtr, QString> sessionDecryptMessage(const EncryptedEvent& encryptedEvent);

    void saveAccessTokenToKeychain() const
    {
        qCDebug(MAIN) << "Saving access token to keychain for" << q->userId();
        auto job = new QKeychain::WritePasswordJob(qAppName());
        job->setAutoDelete(true);
        job->setKey(q->userId());
        job->setBinaryData(data->accessToken());
        job->start();
        //TODO error handling
    }
    void dropAccessToken()
    {
        qCDebug(MAIN) << "Removing access token from keychain for" << q->userId();
        auto job = new QKeychain::DeletePasswordJob(qAppName());
        job->setAutoDelete(true);
        job->setKey(q->userId());
        job->start();

        auto pickleJob = new QKeychain::DeletePasswordJob(qAppName());
        pickleJob->setAutoDelete(true);
        pickleJob->setKey(q->userId() + "-Pickle"_ls);
        pickleJob->start();
        //TODO error handling

        data->setToken({});}

#ifdef Quotient_E2EE_ENABLED
    void saveOlmAccount();

    void loadSessions();
    void saveSession(const QOlmSession& session, const QString& senderKey) const;

    template <typename FnT>
    std::pair<QString, QString> doDecryptMessage(const QOlmSession& session,
                                                 const QOlmMessage& message,
                                                 FnT&& andThen) const;

    std::pair<QString, QString> sessionDecryptMessage(
        const QJsonObject& personalCipherObject, const QByteArray& senderKey);

    bool isKnownCurveKey(const QString& userId, const QString& curveKey) const;

    void loadOutdatedUserDevices();
    void saveDevicesList();
    void loadDevicesList();
    void handleQueryKeys(const QueryKeysJob* job);

    // This function assumes that an olm session with (user, device) exists
    std::pair<QOlmMessage::Type, QByteArray> olmEncryptMessage(
        const QString& userId, const QString& device,
        const QByteArray& message) const;
    bool createOlmSession(const QString& targetUserId,
                          const QString& targetDeviceId,
                          const OneTimeKeys &oneTimeKeyObject);
    QString curveKeyForUserDevice(const QString& userId,
                                  const QString& device) const;
    QJsonObject assembleEncryptedContent(QJsonObject payloadJson,
                                         const QString& targetUserId,
                                         const QString& targetDeviceId) const;
#endif
};
