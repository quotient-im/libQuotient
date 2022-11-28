// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later


#pragma once

#include "e2ee/e2ee_common.h"
#include "e2ee/qolmmessage.h"

#include "csapi/keys.h"

#include <QtCore/QObject>

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
    QOlmAccount(QStringView userId, QStringView deviceId,
                QObject* parent = nullptr);

    //! Creates a new instance of OlmAccount. During the instantiation
    //! the Ed25519 fingerprint key pair and the Curve25519 identity key
    //! pair are generated.
    //! \sa https://matrix.org/docs/guides/e2e_implementation.html#keys-used-in-end-to-end-encryption
    //! \note This needs to be called before any other action or use unpickle() instead.
    void setupNewAccount();

    //! Deserialises from encrypted Base64 that was previously obtained by pickling a `QOlmAccount`.
    //! \note This needs to be called before any other action or use setupNewAccount() instead.
    [[nodiscard]] OlmErrorCode unpickle(QByteArray&& pickled,
                                        const PicklingKey& key);

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
    size_t generateOneTimeKeys(size_t numberOfKeys);

    //! Gets the OlmAccount's one time keys formatted as JSON.
    UnsignedOneTimeKeys oneTimeKeys() const;

    //! Sign all one time keys.
    OneTimeKeys signOneTimeKeys(const UnsignedOneTimeKeys &keys) const;

    UploadKeysJob* createUploadKeyRequest(const UnsignedOneTimeKeys& oneTimeKeys) const;

    DeviceKeys deviceKeys() const;

    //! Remove the one time key used to create the supplied session.
    [[nodiscard]] OlmErrorCode removeOneTimeKeys(const QOlmSession& session);

    //! Creates an inbound session for sending/receiving messages from a received 'prekey' message.
    //!
    //! \param preKeyMessage An Olm pre-key message that was encrypted for this account.
    QOlmExpected<QOlmSession> createInboundSession(
        const QOlmMessage& preKeyMessage) const;

    //! Creates an inbound session for sending/receiving messages from a received 'prekey' message.
    //!
    //! \param theirIdentityKey - The identity key of the Olm account that
    //! encrypted this Olm message.
    QOlmExpected<QOlmSession> createInboundSessionFrom(
        const QByteArray& theirIdentityKey,
        const QOlmMessage& preKeyMessage) const;

    //! Creates an outbound session for sending messages to a specific
    /// identity and one time key.
    QOlmExpected<QOlmSession> createOutboundSession(
        const QByteArray& theirIdentityKey,
        const QByteArray& theirOneTimeKey) const;

    void markKeysAsPublished();

    OlmErrorCode lastErrorCode() const;
    const char* lastError() const;

Q_SIGNALS:
    void needsSave();

private:
    CStructPtr<OlmAccount> olmDataHolder;
    QString m_userId;
    QString m_deviceId;
    OlmAccount* olmData = olmDataHolder.get();

    QOlmExpected<QOlmSession> createInbound(QOlmMessage preKeyMessage,
        const QByteArray &theirIdentityKey = "") const;

    QString accountId() const;
};

} // namespace Quotient
