// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef Quotient_E2EE_ENABLED
#pragma once

#include <QtCore/QObject>

#include <functional>
#include <memory>

namespace Quotient {
class Connection;
class QOlmAccount;

class EncryptionManager : public QObject {
    Q_OBJECT

public:
    explicit EncryptionManager(QObject* parent = nullptr);
    ~EncryptionManager();
    QString sessionDecryptMessage(const QJsonObject& personalCipherObject,
                                  const QByteArray& senderKey, std::unique_ptr<QOlmAccount>& account);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace Quotient
#endif // Quotient_E2EE_ENABLED
