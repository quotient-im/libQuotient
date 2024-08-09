// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2017 Roman Plášil <me@rplasil.name>
// SPDX-FileCopyrightText: 2019 Ville Ranki <ville.ranki@iki.fi>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "avatar.h"
#include "connection.h"
#include "connectiondata.h"
#include "connectionencryptiondata_p.h"
#include "settings.h"
#include "syncdata.h"

#include "csapi/account-data.h"
#include "csapi/capabilities.h"
#include "csapi/logout.h"
#include "csapi/versions.h"
#include "csapi/wellknown.h"

#include <QtCore/QCoreApplication>

namespace Quotient {

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
    // It's an ordered map to make Connection::userIds() return an already sorted list
    QMap<QString, User*> userMap;
    std::unordered_map<QString, Avatar> userAvatarMap;
    DirectChatsMap directChats;
    QMultiHash<QString, QString> directChatMemberIds;
    // The below two variables track local changes between sync completions.
    // See https://github.com/quotient-im/libQuotient/wiki/Handling-direct-chat-events
    DirectChatsMap dcLocalAdditions;
    DirectChatsMap dcLocalRemovals;
    std::unordered_map<QString, EventPtr> accountData;
    QMetaObject::Connection syncLoopConnection {};
    int syncTimeout = -1;

    GetVersionsJob::Response apiVersions{};
    GetCapabilitiesJob::Capabilities capabilities{};

    QVector<GetLoginFlowsJob::LoginFlow> loginFlows;

    static inline bool encryptionDefault = false;
    bool useEncryption = encryptionDefault;
    static inline bool directChatEncryptionDefault = false;
    bool encryptDirectChats = directChatEncryptionDefault;
    std::unique_ptr<_impl::ConnectionEncryptionData> encryptionData;

    JobHandle<GetWellknownJob> resolverJob = nullptr;
    JobHandle<GetLoginFlowsJob> loginFlowsJob = nullptr;

    SyncJob* syncJob = nullptr;
    JobHandle<LogoutJob> logoutJob = nullptr;

    bool cacheState = true;
    bool cacheToBinary =
        SettingsGroup("libQuotient"_ls).get("cache_type"_ls,
                                            SettingsGroup("libQMatrixClient"_ls).get<QString>("cache_type"_ls))
        != "json"_ls;
    bool lazyLoading = false;

    //! \brief Check the homeserver and resolve it if needed, before connecting
    //!
    //! A single entry for functions that need to check whether the homeserver is valid before
    //! running. Emits resolveError() if the homeserver URL is not valid and cannot be resolved
    //! from \p userId; loginError() if the homeserver is accessible but doesn't support \p flow.
    //!
    //! \param userId    fully-qualified MXID to resolve HS from
    //! \param flow      optionally, a login flow that should be supported;
    //!                  `std::nullopt`, if there are no login flow requirements
    //! \return a future that becomes ready once the homeserver is available; if the homeserver
    //!         URL is incorrect or other problems occur, the future is never resolved and is
    //!         deleted (along with associated continuations) as soon as the problem becomes
    //!         apparent
    //! \sa resolveServer, resolveError, loginError
    QFuture<void> ensureHomeserver(const QString& userId, const std::optional<LoginFlow>& flow = {});
    QFuture<void> ensureHomeserverFromUrl(const QString& url, const std::optional<LoginFlow>& flow = {});

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
