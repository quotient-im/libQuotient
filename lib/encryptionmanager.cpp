#include "encryptionmanager.h"

#include "olm/account.hh"

#include <random>
#include <functional>

#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QDataStream>

using namespace QMatrixClient;

using random_bytes_source = std::random_device;

class EncryptionManager::Private {
 public:
  explicit Private(QString id) : userId(id), valid(false), account(nullptr) {}

  QString userId;
  QString deviceId;
  bool valid;

  olm::Account* account;
  QJsonObject publicIdentityKeys;
};

EncryptionManager::EncryptionManager(QString userId, QObject* parent)
    : QObject(parent), d(new Private(std::move(userId))) {}

QString EncryptionManager::userId() const { return d->userId; }

QString EncryptionManager::deviceId() const { return d->deviceId; }

bool EncryptionManager::isValid() const { return d->valid; }

template <typename Cont>
inline uint8_t* _raw_bytes(Cont& cont) {
  return reinterpret_cast<uint8_t*>(cont.data());
}

template <typename Cont>
inline const uint8_t* _raw_bytes_const(const Cont& cont) {
  return reinterpret_cast<const uint8_t*>(cont.data());
}

bool EncryptionManager::initialize(QString deviceId) {
  d->deviceId = deviceId;

  {
    using random_bytes_type = random_bytes_source::result_type;

    auto olmRandomSize =
        d->account->new_account_random_length() * sizeof(uint8_t);
    auto stdRandomSize = olmRandomSize / sizeof(random_bytes_type);
    if (olmRandomSize % sizeof(random_bytes_type)) ++stdRandomSize;

    std::vector<random_bytes_type> randomData(stdRandomSize);
    random_bytes_source rbs;
    std::generate(begin(randomData), end(randomData), std::ref(rbs));
    d->account->new_account(_raw_bytes_const(randomData), olmRandomSize);
    // FIXME: randomData should be scrubbed up here.
    if (d->account->last_error != OLM_SUCCESS) {
      qDebug() << "OLM ERROR: ACCOUNT: creating: "
               << _olm_error_to_string(d->account->last_error);
      return false;
    }
    qDebug() << "OLM: successfully created keys";
  }

  {
    auto publicIdentitySize = d->account->get_identity_json_length();
    QByteArray keyData;
    keyData.resize(/*QByteArray::size_type*/ int(publicIdentitySize));
    d->account->get_identity_json(_raw_bytes(keyData), publicIdentitySize);
    if (d->account->last_error != OLM_SUCCESS) {
      qDebug() << "OLM ERROR: ACCOUNT: reading keys: "
               << _olm_error_to_string(d->account->last_error);
      return false;
    }
    auto json = QJsonDocument::fromJson(keyData);
    d->publicIdentityKeys = json.object();

    qDebug() << "OLM: successfully read keys:" << json.toJson();
  }

  d->valid = true;
  return true;
}

const QJsonObject& EncryptionManager::publicIdentityKeys() const {
  return d->publicIdentityKeys;
}

QByteArray EncryptionManager::save() {
  QByteArray bytes;
  QDataStream stream(&bytes, QIODevice::WriteOnly);
  stream << 0;  // data version
  stream << d->deviceId;
  stream << QJsonDocument(d->publicIdentityKeys).toBinaryData();

  auto pickleLength = olm::pickle_length(*d->account);
  QByteArray pickleData;
  pickleData.resize(/*QByteArray::size_type*/ int(pickleLength));
  olm::pickle(_raw_bytes(pickleData), *d->account);
  if (d->account->last_error != OLM_SUCCESS) {
    qDebug() << "OLM: ACCOUNT: Error while saving: "
             << _olm_error_to_string(d->account->last_error);
    return {};
  }
  stream << pickleData;
  return bytes;
}

void EncryptionManager::load(const QByteArray& data) {
  QDataStream stream(data);
  int streamVersion;
  stream >> streamVersion;
  stream >> d->deviceId;
  QByteArray keyjsondata;
  stream >> keyjsondata;
  d->publicIdentityKeys = QJsonDocument::fromBinaryData(keyjsondata).object();

  QByteArray accountPickleData;
  stream >> accountPickleData;
  if (stream.status() != QDataStream::Ok) {
    qDebug() << "OLM: Error while reading saved data: stream error";
    return;
  }
  olm::unpickle(_raw_bytes_const(accountPickleData),
                _raw_bytes_const(accountPickleData) + accountPickleData.size(),
                *d->account);
  if (d->account->last_error != OLM_SUCCESS) {
    qDebug() << "OLM: ACCOUNT: Error while loading: "
             << _olm_error_to_string(d->account->last_error);
    return;
  }

  d->valid = true;
}
