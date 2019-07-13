#include "encryptionmanager.h"

#include <functional>
#include <memory>
#include <QtCore/QStringBuilder>
#include <QtCore/QHash>
#include <account.h> // QtOlm

#include "csapi/keys.h"
#include "connection.h"
#include "e2ee.h"

using namespace QMatrixClient;
using namespace QtOlm;
using std::move;

class EncryptionManager::Private
{
    public:
        explicit Private(const QByteArray& encryptionAccountPickle, float signedKeysProportion, float oneTimeKeyThreshold)
            : signedKeysProportion(move(signedKeysProportion)),
              oneTimeKeyThreshold(move(oneTimeKeyThreshold))
        {
            Q_ASSERT((0 <= signedKeysProportion) && (signedKeysProportion <= 1));
            Q_ASSERT((0 <= oneTimeKeyThreshold) && (oneTimeKeyThreshold <= 1));
            if (encryptionAccountPickle.isEmpty())
            {
                olmAccount.reset(new Account());
            } else {
                olmAccount.reset(new Account(encryptionAccountPickle)); // TODO: passphrase even with qtkeychain?
            }
            /*
             * Note about targetKeysNumber:
             *
             * From: https://github.com/Zil0/matrix-python-sdk/
             * File: matrix_client/crypto/olm_device.py
             *
             * Try to maintain half the number of one-time keys libolm can hold uploaded
             * on the HS. This is because some keys will be claimed by peers but not
             * used instantly, and we want them to stay in libolm, until the limit is reached
             * and it starts discarding keys, starting by the oldest.
             */
             targetKeysNumber = olmAccount->maxOneTimeKeys(); // 2 // see note below
             targetOneTimeKeyCounts =
             {
                 {SignedCurve25519Key, qRound(signedKeysProportion * targetKeysNumber)},
                 {Curve25519Key, qRound((1-signedKeysProportion) * targetKeysNumber)}
             };
        }
        ~Private() = default;

        UploadKeysJob* uploadIdentityKeysJob = nullptr;
        UploadKeysJob* uploadOneTimeKeysJob = nullptr;

        QScopedPointer<Account> olmAccount;

        float signedKeysProportion;
        float oneTimeKeyThreshold;
        int targetKeysNumber;

        void updateKeysToUpload();
        bool oneTimeKeyShouldUpload();

        QHash<QString, int> oneTimeKeyCounts;
        void setOneTimeKeyCounts(const QHash<QString, int> oneTimeKeyCountsNewValue)
        {
            oneTimeKeyCounts = oneTimeKeyCountsNewValue;
            updateKeysToUpload();
        }
        QHash<QString, int> oneTimeKeysToUploadCounts;
        QHash<QString, int> targetOneTimeKeyCounts;
};

EncryptionManager::EncryptionManager(const QByteArray &encryptionAccountPickle, float signedKeysProportion, float oneTimeKeyThreshold,
                                     QObject* parent)
    : QObject(parent),
    d(std::make_unique<Private>(std::move(encryptionAccountPickle), std::move(signedKeysProportion), std::move(oneTimeKeyThreshold)))
{

}

EncryptionManager::~EncryptionManager() = default;

void EncryptionManager::uploadIdentityKeys(Connection* connection)
{
    // https://matrix.org/docs/spec/client_server/latest#post-matrix-client-r0-keys-upload
    DeviceKeys deviceKeys
    {
        /*
         * The ID of the user the device belongs to. Must match the user ID used when logging in.
         * The ID of the device these keys belong to. Must match the device ID used when logging in.
         * The encryption algorithms supported by this device.
         */
        connection->userId(), connection->deviceId(), SupportedAlgorithms,
        /*
         * Public identity keys. The names of the properties should be in the format <algorithm>:<device_id>.
         * The keys themselves should be encoded as specified by the key algorithm.
         */
        {
            {
                Curve25519Key + QStringLiteral(":") + connection->deviceId(),
                        d->olmAccount->curve25519IdentityKey()
            },
            {
                Ed25519Key + QStringLiteral(":") + connection->deviceId(),
                        d->olmAccount->ed25519IdentityKey()
            }
        },
        /* signatures should be provided after the unsigned deviceKeys generation */
        {}
    };

    QJsonObject deviceKeysJsonObject = toJson(deviceKeys);
    /* additionally removing signatures key,
     * since we could not initialize deviceKeys
     * without an empty signatures value:
     */
    deviceKeysJsonObject.remove(QStringLiteral("signatures"));
    /*
     * Signatures for the device key object.
     * A map from user ID, to a map from <algorithm>:<device_id> to the signature.
     * The signature is calculated using the process called Signing JSON.
     */
    deviceKeys.signatures =
    {
        {
            connection->userId(),
            {
                {
                    Ed25519Key + QStringLiteral(":") + connection->deviceId(),
                            d->olmAccount->sign(deviceKeysJsonObject)
                }
            }
        }
    };

    connect(d->uploadIdentityKeysJob, &BaseJob::success, this, [this] {
        d->setOneTimeKeyCounts(d->uploadIdentityKeysJob->oneTimeKeyCounts());
        qDebug() << QString("Uploaded identity keys.");
    });
    d->uploadIdentityKeysJob = connection->callApi<UploadKeysJob>(deviceKeys);
}

void EncryptionManager::uploadOneTimeKeys(Connection* connection, bool forceUpdate)
{
    if (forceUpdate || d->oneTimeKeyCounts.isEmpty())
    {
        auto job = connection->callApi<UploadKeysJob>();
        connect(job, &BaseJob::success, this, [job,this] {
            d->setOneTimeKeyCounts(job->oneTimeKeyCounts());
        });

    }

    int signedKeysToUploadCount = d->oneTimeKeysToUploadCounts.value(SignedCurve25519Key, 0);
    int unsignedKeysToUploadCount = d->oneTimeKeysToUploadCounts.value(Curve25519Key, 0);

    d->olmAccount->generateOneTimeKeys(signedKeysToUploadCount + unsignedKeysToUploadCount);

    QHash<QString, QVariant> oneTimeKeys = {};
    const auto& olmAccountCurve25519OneTimeKeys = d->olmAccount->curve25519OneTimeKeys();

    int oneTimeKeysCounter = 0;
    for (auto it = olmAccountCurve25519OneTimeKeys.cbegin(); it != olmAccountCurve25519OneTimeKeys.cend(); ++it)
    {
        QString keyId = it.key();
        QString keyType;
        QVariant key;
        if (oneTimeKeysCounter < signedKeysToUploadCount)
        {
            QJsonObject message
            {
                {QStringLiteral("key"), it.value().toString()}
            };
            key = d->olmAccount->sign(message);
            keyType = SignedCurve25519Key;

        } else {
            key = it.value();
            keyType = Curve25519Key;
        }
        ++oneTimeKeysCounter;
        oneTimeKeys.insert(QString("%1:%2").arg(keyType).arg(keyId), key);
    }

    d->uploadOneTimeKeysJob = connection->callApi<UploadKeysJob>(none, oneTimeKeys);
    d->olmAccount->markKeysAsPublished();
    qDebug() << QString("Uploaded new one-time keys: %1 signed, %2 unsigned.")
                .arg(signedKeysToUploadCount).arg(unsignedKeysToUploadCount);
}

QByteArray EncryptionManager::olmAccountPickle()
{
    return d->olmAccount->pickle(); // TODO: passphrase even with qtkeychain?
}

void EncryptionManager::Private::updateKeysToUpload()
{
    for (auto it = targetOneTimeKeyCounts.cbegin(); it != targetOneTimeKeyCounts.cend(); ++it)
    {
        int numKeys = oneTimeKeyCounts.value(it.key(), 0);
        int numToCreate = qMax(it.value() - numKeys, 0);
        oneTimeKeysToUploadCounts.insert(it.key(), numToCreate);
    }
}

bool EncryptionManager::Private::oneTimeKeyShouldUpload()
{
    if (oneTimeKeyCounts.empty())
        return true;
    for (auto it = targetOneTimeKeyCounts.cbegin(); it != targetOneTimeKeyCounts.cend(); ++it)
    {
        if (oneTimeKeyCounts.value(it.key(), 0) < it.value() * oneTimeKeyThreshold)
            return true;
    }
    return false;
}
