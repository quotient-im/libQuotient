#ifndef ENCRYPTIONMANAGER_H
#define ENCRYPTIONMANAGER_H

#pragma once

#include <QObject>

namespace QMatrixClient {
class EncryptionManager : public QObject {
  Q_OBJECT
 public:
  explicit EncryptionManager(QString userId, QObject* parent = nullptr);

  QString userId() const;
  QString deviceId() const;

  bool isValid() const;

  bool initialize(QString deviceId);

  const QJsonObject& publicIdentityKeys() const;

  void load(const QByteArray& data);
  QByteArray save();

 signals:

 public slots:

 private:
  class Private;
  Private* d;
};
}  // namespace QMatrixClient

#endif  // ENCRYPTIONMANAGER_H
