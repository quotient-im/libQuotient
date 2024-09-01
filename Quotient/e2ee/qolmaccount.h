// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later


#pragma once

#include <Quotient/e2ee/e2ee_common.h>
#include <Quotient/e2ee/qolmmessage.h>

#include <Quotient/csapi/keys.h>

#include <QtCore/QObject>

#include <vodozemac.h>

namespace olm
{
struct Account;
}

struct OlmAccount;

namespace Quotient {

class QOlmSession;

//! An olm account manages all cryptographic keys used on a device.
//! \code{.cpp}
//! const auto olmAccount = new QOlmAccount(this);
//! \endcode
class QUOTIENT_API QOlmAccount : public QObject
{
    Q_OBJECT
public:

    //! Creates a new instance of OlmAccount. During the instantiation
    //! the Ed25519 fingerprint key pair and the Curve25519 identity key
    //! pair are generated.
    //! \sa https://matrix.org/docs/guides/e2e_implementation.html#keys-used-in-end-to-end-encryption
    [[nodiscard]] static QOlmAccount *newAccount(QObject *parent, const QString& userId, const QString& deviceId);

    //! Deserialises from encrypted Base64 that was previously obtained by pickling a `QOlmAccount`.
    [[nodiscard]] static QOlmAccount *unpickle(QByteArray&& pickled,
                                        const PicklingKey& key, const QString& userId, const QString& deviceId, QObject* parent);

    //! Serialises an OlmAccount to encrypted Base64.
    QByteArray pickle(const PicklingKey& key) const;

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
    void generateOneTimeKeys(size_t numberOfKeys);

    //! Gets the OlmAccount's one time keys formatted as JSON.
    UnsignedOneTimeKeys oneTimeKeys() const;

    //! Sign all one time keys.
    OneTimeKeys signOneTimeKeys(const UnsignedOneTimeKeys &keys) const;

    UploadKeysJob* createUploadKeyRequest(const UnsignedOneTimeKeys& oneTimeKeys) const;

    DeviceKeys deviceKeys() const;

    //! Creates an inbound session for sending/receiving messages from a received 'prekey' message.
    //!
    //! \param theirIdentityKey - The identity key of the Olm account that
    //! encrypted this Olm message.
    std::pair<QOlmSession, QString> createInboundSession(
        const QByteArray& theirIdentityKey,
        const QOlmMessage& preKeyMessage);

    //! Creates an outbound session for sending messages to a specific
    /// identity and one time key.
    QOlmSession createOutboundSession(
        const QByteArray& theirIdentityKey,
        const QByteArray& theirOneTimeKey);

    void markKeysAsPublished();

Q_SIGNALS:
    void needsSave();

private:
    explicit QOlmAccount(QObject* parent = nullptr);
    QOlmAccount(rust::Box<olm::Account> account, QObject* parent = nullptr);
    rust::Box<olm::Account> olmData;
    QString m_userId;
    QString m_deviceId;

    QString accountId() const;
};

// TODO, 0.9: Move the two below to qolmutility.h

QUOTIENT_API bool verifyIdentitySignature(const DeviceKeys& deviceKeys,
                                          const QString& deviceId,
                                          const QString& userId);

//! checks if the signature is signed by the signing_key
QUOTIENT_API bool ed25519VerifySignature(const QString& signingKey,
                                         const QJsonObject& obj,
                                         const QString& signature);

} // namespace Quotient
