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
#    include "connectionencryptiondata_p.h"
#endif

#include "csapi/account-data.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QPointer>

namespace Quotient {

class EncryptedEvent;

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
    QMultiHash<QString, QString> directChatMemberIds;
    DirectChatUsersMap directChatUsers;
    // The below two variables track local changes between sync completions.
    // See https://github.com/quotient-im/libQuotient/wiki/Handling-direct-chat-events
    DirectChatsMap dcLocalAdditions;
    DirectChatsMap dcLocalRemovals;
    UnorderedMap<QString, EventPtr> accountData;
    QMetaObject::Connection syncLoopConnection {};
    int syncTimeout = -1;

    GetCapabilitiesJob* capabilitiesJob = nullptr;
    GetCapabilitiesJob::Capabilities capabilities;

    QVector<GetLoginFlowsJob::LoginFlow> loginFlows;

#ifdef Quotient_E2EE_ENABLED
    static inline bool encryptionDefault = false;
    bool useEncryption = encryptionDefault;
    std::unique_ptr<_impl::ConnectionEncryptionData> encryptionData;
#else
    static constexpr bool useEncryption = false;
#endif

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
    void completeSetup(const QString &mxId, bool mock = false);
    void removeRoom(const QString& roomId);

    void consumeRoomData(SyncDataList&& roomDataList, bool fromCache);
    void consumeAccountData(Events&& accountDataEvents);
    void consumePresenceData(Events&& presenceData);
    void consumeToDeviceEvents(Events&& toDeviceEvents);

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

    void saveAccessTokenToKeychain() const;
    void dropAccessToken();
};
} // namespace Quotient
