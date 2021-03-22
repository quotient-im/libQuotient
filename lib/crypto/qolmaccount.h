// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "csapi/keys.h"
#include "crypto/e2ee.h"
#include "crypto/qolmerrors.h"
#include "crypto/qolmmessage.h"
#include "crypto/qolmsession.h"
#include <QObject>

struct OlmAccount;

namespace Quotient {

class QOlmSession;
class Connection;

//! An olm account manages all cryptographic keys used on a device.
//! \code{.cpp}
//! const auto olmAccount = new QOlmAccount(this);
//! \endcode
class QOlmAccount
{
public:
    QOlmAccount(const QString &userId, const QString &deviceId);
    ~QOlmAccount();

    //! Creates a new instance of OlmAccount. During the instantiation
    //! the Ed25519 fingerprint key pair and the Curve25519 identity key
    //! pair are generated. For more information see <a
    //! href="https://matrix.org/docs/guides/e2e_implementation.html#keys-used-in-end-to-end-encryption">here</a>.
    //! This needs to be called before any other action or use unpickle() instead.
    void createNewAccount();

    //! Deserialises from encrypted Base64 that was previously obtained by pickling a `QOlmAccount`.
    //! This needs to be called before any other action or use createNewAccount() instead.
    void unpickle(QByteArray &picked, const PicklingMode &mode);

    //! Serialises an OlmAccount to encrypted Base64.
    std::variant<QByteArray, QOlmError> pickle(const PicklingMode &mode);

    //! Returns the account's public identity keys already formatted as JSON
    IdentityKeys identityKeys() const;

    //! Returns the signature of the supplied message.
    QByteArray sign(const QByteArray &message) const;
    QByteArray sign(const QJsonObject& message) const;

    //! Sign identity keys.
    QByteArray signIdentityKeys() const;

    //! Maximum number of one time keys that this OlmAccount can
    //! currently hold.
    size_t maxNumberOfOneTimeKeys() const;

    //! Generates the supplied number of one time keys.
    size_t generateOneTimeKeys(size_t numberOfKeys) const;

    //! Gets the OlmAccount's one time keys formatted as JSON.
    OneTimeKeys oneTimeKeys() const;

    //! Sign all time key.
    QMap<QString, SignedOneTimeKey> signOneTimeKeys(const OneTimeKeys &keys) const;

    //! Sign one time key.
    QByteArray signOneTimeKey(const QString &key) const;

    SignedOneTimeKey signedOneTimeKey(const QByteArray &key, const QString &signature) const;

    UploadKeysJob *createUploadKeyRequest(const OneTimeKeys &oneTimeKeys);

    DeviceKeys getDeviceKeys() const;

    //! Remove the one time key used to create the supplied session.
    [[nodiscard]] std::optional<QOlmError> removeOneTimeKeys(const std::unique_ptr<QOlmSession> &session) const;

    //! Creates an inbound session for sending/receiving messages from a received 'prekey' message.
    //!
    //! \param message An Olm pre-key message that was encrypted for this account.
    std::variant<std::unique_ptr<QOlmSession>, QOlmError> createInboundSession(const QOlmMessage &preKeyMessage);

    //! Creates an inbound session for sending/receiving messages from a received 'prekey' message.
    //!
    //! \param theirIdentityKey - The identity key of an Olm account that
    //! encrypted this Olm message.
    std::variant<std::unique_ptr<QOlmSession>, QOlmError> createInboundSessionFrom(const QByteArray &theirIdentityKey, const QOlmMessage &preKeyMessage);

    //! Creates an outbound session for sending messages to a specific
    /// identity and one time key.
    std::variant<std::unique_ptr<QOlmSession>, QOlmError> createOutboundSession(const QByteArray &theirIdentityKey, const QByteArray &theirOneTimeKey);

    // HACK do not use directly
    QOlmAccount(OlmAccount *account);
    OlmAccount *data();
private:
    OlmAccount *m_account = nullptr; // owning
    QString m_userId;
    QString m_deviceId;
};

bool verifyIdentitySignature(const DeviceKeys &deviceKeys,
                             const QString &deviceId,
                             const QString &userId);

//! checks if the signature is signed by the signing_key
bool ed25519VerifySignature(const QString &signingKey,
                            const QJsonObject &obj,
                            const QString &signature);

} // namespace Quotient
