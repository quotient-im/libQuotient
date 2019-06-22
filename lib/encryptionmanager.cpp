#include "encryptionmanager.h"

#include <functional>
#include <memory>
#include <QtCore/QStringBuilder>
#include <QtCore/QHash>
#include <account.h> // QtOlm

#include "csapi/keys.h"
#include "connection.h"

using namespace QMatrixClient;
using namespace QtOlm;
using std::move;

static const auto ed25519Name = QStringLiteral("ed25519");
static const auto Curve25519Name = QStringLiteral("curve25519");
static const auto SignedCurve25519Name = QStringLiteral("signed_curve25519");
static const auto OlmCurve25519AesSha256AlgoName = QStringLiteral("m.olm.curve25519-aes-sha256");
static const auto MegolmV1AesShaAlgoName = QStringLiteral("m.megolm.v1.aes-sha");
static const QStringList SupportedAlgorithms = { OlmCurve25519AesSha256AlgoName, MegolmV1AesShaAlgoName };

class EncryptionManager::Private
{
    public:
        explicit Private(const QByteArray& encryptionAccountPickle, float signedKeysProportion, float oneTimeKeyThreshold)
            : olmAccount(new Account(encryptionAccountPickle)), // TODO: passphrase even with qtkeychain?
              signedKeysProportion(move(signedKeysProportion)),
              oneTimeKeyThreshold(move(oneTimeKeyThreshold)),
              targetKeysNumber(olmAccount->maxOneTimeKeys()) // 2 // see note below
        {
            Q_ASSERT((0 <= signedKeysProportion) && (signedKeysProportion <= 1));
            Q_ASSERT((0 <= oneTimeKeyThreshold) && (oneTimeKeyThreshold <= 1));
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
        }
        ~Private()
        {
            delete olmAccount;
        }

        UploadKeysJob* uploadIdentityKeysJob = nullptr;
        UploadKeysJob* uploadOneTimeKeysJob = nullptr;

        Account* olmAccount;
        const QByteArray encryptionAccountPickle;

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
        QHash<QString, int> targetOneTimeKeyCounts
        {
            {SignedCurve25519Name, qRound(signedKeysProportion * targetKeysNumber)},
            {Curve25519Name, qRound((1-signedKeysProportion) * targetKeysNumber)}
        };
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
                Curve25519Name + QStringLiteral(":") + connection->deviceId(),
                        d->olmAccount->curve25519IdentityKey()
            },
            {
                ed25519Name + QStringLiteral(":") + connection->deviceId(),
                        d->olmAccount->ed25519IdentityKey()
            }
        },
        /*
         * Signatures for the device key object.
         * A map from user ID, to a map from <algorithm>:<device_id> to the signature.
         * The signature is calculated using the process called Signing JSON.
         */
        {
            {
                connection->userId(),
                {
                    {
                        ed25519Name + QStringLiteral(":") + connection->deviceId(),
                                d->olmAccount->sign(toJson(deviceKeys))
                    }
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
    // TODO
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
