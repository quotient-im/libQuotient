// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-license-identifier: lgpl-2.1-or-later

#include "crypto/encryptionmanager_p.h"
#include "events/eventloader.h"
#include "syncdata.h"
#include "connection.h"

using namespace Quotient;

constexpr int MAX_ONETIME_KEYS = 50;

EncryptionManager::EncryptionManager(const QString &userId,
        const QString &deviceId, Connection *connection)
    : QObject(connection)
    , m_accountSettings(AccountSettings(userId))
    , m_connection(connection)
{
    // Init olmAccount
    m_olmAccount = std::make_unique<QOlmAccount>(userId, deviceId);

    if (m_accountSettings.encryptionAccountPickle().isEmpty()) {
        // create new account and save unpickle data
        m_olmAccount->createNewAccount();
        // TODO handle pickle errors

        // Upload one time keys for the device.
        m_olmAccount->generateOneTimeKeys(MAX_ONETIME_KEYS);
        auto job = m_olmAccount->createUploadKeyRequest();
        m_connection->run(job);
        connect(job, &BaseJob::result, this, [job, this] {
            m_olmAccount->markKeysAsPublished();
            m_accountSettings.setEncryptionAccountPickle(std::get<QByteArray>(m_olmAccount->pickle(Unencrypted{})));
        });
        connect(job, &BaseJob::failure, this, [job, this] {
            // todo handle failure
            // 404 -> server doesn't support key upload
            // other -> it's a fatal error
        });
        m_accountSettings.setEncryptionAccountPickle(std::get<QByteArray>(m_olmAccount->pickle(Unencrypted{})));
    } else {
        // account already existing
        auto pickle = m_accountSettings.encryptionAccountPickle();
        m_olmAccount->unpickle(pickle, Unencrypted{});
    }
}

EncryptionManager::~EncryptionManager()
{}

QOlmAccount *EncryptionManager::olmAccount() const
{
    return m_olmAccount.get();
}

void EncryptionManager::uploadIdentityKeys() const
{
    // Upload idenitty keys.
    OneTimeKeys unused;
    auto request = m_olmAccount->createUploadKeyRequest(unused);
    m_connection->run(request);
}

void EncryptionManager::uploadOneTimeKeys()
{

}

void EncryptionManager::ensureOneTimeKeyCount(const QHash<QString, int> &counts) const
{
    for (const auto &[algorithm, nbUnclaimedKeys] : asKeyValueRange(counts)) {
        if (nbUnclaimedKeys < MAX_ONETIME_KEYS) {
            const int nkeys = MAX_ONETIME_KEYS - nbUnclaimedKeys;

            m_olmAccount->generateOneTimeKeys(nkeys);

            auto request = m_olmAccount->createUploadKeyRequest();
            auto job = m_connection->run(request);
            connect(request, &BaseJob::result, this, [job, this] {
                m_olmAccount->markKeysAsPublished();
            });
        }
    }
}

void EncryptionManager::trackUserDevices(const QString &userId)
{
    // 1. It first sets a flag to record that it is now tracking Bob's device list, and a separate flag to indicate that its list of Bob's devices is outdated. Both flags should be in storage which persists over client restarts.
    if (m_trackedUserDevices.contains(userId)) {
        return;
    }
    m_trackedUserDevices.append(userId);
    m_outdatedUserDevices.append(userId);

    // 2. It then makes a request to /keys/query, passing Bob's user ID in the device_keys parameter. When the request completes, it stores the resulting list of devices in persistent storage, and clears the 'outdated' flag.
    QHash<QString, QStringList> deviceKeys;
    deviceKeys[userId] = QStringList();
    auto job =  m_connection->callApi<QueryKeysJob>(deviceKeys);
    m_queryKeysJob.insert(userId, job);
    connect(job, &BaseJob::result, this, [this, userId, job] {
        auto devices = job->deviceKeys()[userId];
        m_devices.insert(userId, devices);
        m_outdatedUserDevices.removeOne(userId);
        m_queryKeysJob.remove(userId);
        save();
    });
}

void EncryptionManager::trackUsersDevices(const QStringList &users)
{
    QStringList usersToTrack;
    for (const auto &userId : qAsConst(users)) {
        // 1. It first sets a flag to record that it is now tracking Bob's device list, and a separate flag to indicate that its list of Bob's devices is outdated. Both flags should be in storage which persists over client restarts.
        if (m_trackedUserDevices.contains(userId) || usersToTrack.contains(userId)) {
            continue;
        }
        m_trackedUserDevices.append(userId);
        m_outdatedUserDevices.append(userId);
        usersToTrack.append(userId);
    }
    if (usersToTrack.isEmpty()) {
        return;
    }

    // 2. It then makes a request to /keys/query, passing Bob's user ID in the device_keys parameter. When the request completes, it stores the resulting list of devices in persistent storage, and clears the 'outdated' flag.
    QHash<QString, QStringList> deviceKeys;
    for (const auto &userId : qAsConst(usersToTrack)) {
        deviceKeys[userId] = QStringList();
    }
    auto job =  m_connection->callApi<QueryKeysJob>(deviceKeys);
    for (const auto &userId : qAsConst(usersToTrack)) {
        m_queryKeysJob.insert(userId, job);
    }
    connect(job, &BaseJob::result, this, [this, usersToTrack, job] {
        for (const auto &userId : qAsConst(usersToTrack)) {
            auto devices = job->deviceKeys()[userId];
            m_devices.insert(userId, devices);
            m_outdatedUserDevices.removeOne(userId);
            m_queryKeysJob.remove(userId);
        }
        save();
    });
}

void EncryptionManager::save()
{
    // save state from encryption manager (e.g. device keys)
    QJsonObject obj;
    obj.insert("trackedUserDevices", QJsonArray::fromStringList(m_trackedUserDevices));
    obj.insert("outdatedUserDevices", QJsonArray::fromStringList(m_outdatedUserDevices));
    obj.insert("nextBatch", m_nextBatch);
    //obj.insert("devices", fillJson(objDevices));

    QFile deviceFile { m_connection->stateCacheDir().filePath("devices.json") };
    if (deviceFile.open(QFile::WriteOnly | QFile::Text)) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        // TODO maybe save as binary too?
        const auto deviceData = QJsonDocument(obj).toJson(QJsonDocument::Compact);
#else
        QJsonDocument json(obj);
        const auto deviceData = json.toJson(QJsonDocument::Compact);
#endif
        deviceFile.write(deviceData.data(), deviceData.size());
    }
}

void EncryptionManager::load()
{
    QFile f { m_connection->stateCacheDir().filePath("devices.json") };
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&f);
        const auto doc = QJsonDocument::fromJson(in.readAll().toUtf8());
        if (!doc.isNull()) {
            const auto obj = doc.object();
            m_trackedUserDevices = fromJson<QStringList>(obj["trackedUserDevices"]);
            m_outdatedUserDevices = fromJson<QStringList>(obj["outdatedUserDevices"]);
            m_nextBatch = fromJson<QString>(obj["nextBatch"]);
            m_devices = fromJson<QHash<QString, QHash<QString, QueryKeysJob::DeviceInformation>>>(obj["devices"]);
        }
    }
}

void EncryptionManager::getKeysChangesSince(const QString &nextBatch)
{
    auto getKeysChangesJob = m_connection->callApi<GetKeysChangesJob>(m_nextBatch, nextBatch);
    connect(getKeysChangesJob, &BaseJob::result, this, [this, nextBatch, getKeysChangesJob] {
        updateDevices(getKeysChangesJob->changed(), nextBatch);
    });
}

void EncryptionManager::updateDevices(const QStringList &changed, const QString &nextBatch)
{
    m_nextBatch = nextBatch;
    QStringList usersToDownload;
    for (const auto &userId : changed) {
        if (m_trackedUserDevices.contains(userId)) {
            // Mark as outdated
            m_outdatedUserDevices.append(userId);

            // query keys for user
            if (m_queryKeysJob.contains(userId)) {
                // cancel job if it exists
                auto job = m_queryKeysJob[userId];
                job->abandon();

                // A job can be shared across multiple users. Delete it everywhere.
                QStringList keysToDelete;
                for (const auto &[key, value] : asKeyValueRange(m_queryKeysJob)) {
                    if (value == job) {
                        keysToDelete.append(key);
                    }
                }
                for (const auto &key : keysToDelete) {
                    m_queryKeysJob.remove(key);
                }
            }

            usersToDownload.append(userId);
        }
    }
    if (usersToDownload.isEmpty()) {
        save();
        return;
    }

    QHash<QString, QStringList> deviceKeys;
    for (const auto &userId : qAsConst(usersToDownload)) {
        deviceKeys[userId] = QStringList();
    }
    auto job = m_connection->callApi<QueryKeysJob>(deviceKeys);
    for (const auto &userId : qAsConst(usersToDownload)) {
        m_queryKeysJob.insert(userId, job);
    }
    connect(job, &BaseJob::result, this, [this, usersToDownload, job] {
        for (const auto &userId : qAsConst(usersToDownload)) {
            if (!job->deviceKeys().contains(userId)) {
                continue;
            }
            auto devices = job->deviceKeys()[userId];
            m_devices.insert(userId, devices);
            m_outdatedUserDevices.removeOne(userId);
        }
        save();
    });
}

EventPtr EncryptionManager::handleOlmMessage(const EncryptedEvent& encryptedEvent)
{
    // handle olm message
    const auto myKey = m_olmAccount->identityKeys().curve25519;

    const auto ciphertext = encryptedEvent.ciphertext(myKey);

    if (!ciphertext.has_value()) {
        return {};
    }

    // 0 = > OLM_PRE_KEY, 1 => OLM_MESSAGE
    const auto type = ciphertext->value("type").toInt();

    const QOlmMessage message(ciphertext->value("body").toString().toUtf8(),
                              type == 0 ? QOlmMessage::Type::PreKey : QOlmMessage::Type::General);

    auto document = tryToHandle(encryptedEvent.senderKey(), message);

    if (document.isEmpty()) {
        // Check for prekey message
        if (type == 0) {
            document = handlePreKeyOlmMessage(encryptedEvent.senderId(), encryptedEvent.senderKey().toUtf8(), message);
        }
    }


    return {};
}

QJsonDocument EncryptionManager::tryToHandle(const QString &senderKey, const QOlmMessage &message)
{
    const auto sessionIds = getOlmSessions(senderKey);

    for (const auto &sessionId : sessionIds) {
        auto session = getOlmSession(senderKey, sessionId);
        if (!session) {
            continue;
        }

        auto result = session->decrypt(message);
        if (std::holds_alternative<QOlmError>(result)) {
            qDebug() << "failed to decrypt olm message with sessionId:" << sessionId;
            continue;
        }
        const auto text = std::get<QString>(result);

        // save new session
        // TODO error handling
        saveOlmSession(sessionId, std::move(session), QDateTime::currentMSecsSinceEpoch());

        return QJsonDocument::fromJson(text.toUtf8());
    }
    return {};
}

void EncryptionManager::saveAccount()
{
    m_accountSettings.setEncryptionAccountPickle(std::get<QByteArray>(m_olmAccount->pickle(Unencrypted{})));
}

QJsonDocument EncryptionManager::handlePreKeyOlmMessage(const QString &sender, const QByteArray &senderKey,
                                                        const QOlmMessage &message)
{

    auto inboundSessionResult = m_olmAccount->createInboundSessionFrom(senderKey, message);

    if (std::holds_alternative<QOlmError>(inboundSessionResult)) {
        qDebug() << "failed to create inbound session with" << sender;
        return {};
    }
    // We also remove the one time key used to establish that
    // session so we'll have to update our copy of the account object.
    saveAccount();

    auto inboundSession = std::get<std::unique_ptr<QOlmSession>>(std::move(inboundSessionResult));
    auto matches = inboundSession->matchesInboundSessionFrom(senderKey, message);
    if (auto res = std::get_if<bool>(&matches); res) {
        qDebug() << "inbound olm session doesn't match sender's key" << sender;
    }

    auto textResult = inboundSession->decrypt(message);

    if (std::holds_alternative<QOlmError>(textResult)) {
        qDebug() << "failed to decrypt olm message" << message;
        return {};
    }

    auto doc = QJsonDocument::fromJson(std::get<QString>(textResult).toUtf8());

    saveOlmSession(senderKey, std::move(inboundSession), QDateTime::currentMSecsSinceEpoch());
    return doc;
}

void EncryptionManager::saveOlmSession(const QString &curve25519, std::unique_ptr<QOlmSession> olmSession,
        uint64_t timestamp)
{
    const auto sessionPickle = std::get<QByteArray>(olmSession->pickle(Unencrypted {}));
    const auto sessionId = olmSession->sessionId();

    StoredOlmSession storedOlmSession;
    storedOlmSession.lastMessageTimestamp = timestamp;
    storedOlmSession.pickedSession = sessionPickle;

    QFile f {  m_connection->stateCacheDir().filePath(QStringLiteral("curve25519/") + curve25519 + QStringLiteral(".json")) };

    if (f.open(QFile::ReadWrite | QFile::Text)) {
        QTextStream in(&f);
        auto doc = QJsonDocument::fromJson(in.readAll().toUtf8());

        auto sessionObj = doc.object();
        sessionObj.insert(sessionId, toJson(storedOlmSession));

        doc.setObject(sessionObj);
        const auto data = doc.toJson();

        f.write(data.data(), data.size());
    } else {
        qDebug() << "Couldn't save olm session";
    }
}

std::unique_ptr<QOlmSession> EncryptionManager::getOlmSession(const QString &curve25519,
        const QString &sessionId)
{
    QFile f {  m_connection->stateCacheDir().filePath(QStringLiteral("curve25519/") + curve25519 + QStringLiteral(".json")) };
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&f);
        const auto doc = QJsonDocument::fromJson(in.readAll().toUtf8());
        const auto sessionObj = doc.object();
        if (!sessionObj.contains(sessionId)) {
            return nullptr;
        }

        const auto storedOlmSession = fromJson<StoredOlmSession>(sessionObj.value(sessionId));

        auto olmSession = std::get<std::unique_ptr<QOlmSession>>(QOlmSession::unpickle(storedOlmSession.pickedSession, Unencrypted {}));
        // todo error handling
        return olmSession;
    }
    return nullptr;
}

QStringList EncryptionManager::getOlmSessions(const QString &curve25519)
{
    // TODO opening and closing a file for each session key is not optional
    // Investigate caching it in memory with getOlmSessions and persisting
    // after each save.
    QFile f {  m_connection->stateCacheDir().filePath(QStringLiteral("curve25519/") + curve25519 + QStringLiteral(".json")) };
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&f);
        const auto doc = QJsonDocument::fromJson(in.readAll().toUtf8());
        return doc.object().keys();
    }
    return {};
}
