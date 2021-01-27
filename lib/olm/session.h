// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#ifdef Quotient_E2EE_ENABLED

#include "olm/e2ee.h"
#include "olm/message.h"
#include "olm/errors.h"
#include "olm/qolmaccount.h"

namespace Quotient {

class QOlmAccount;
class QOlmSession;


//! Either an outbound or inbound session for secure communication.
class QOlmSession
{
public:
    ~QOlmSession();
    //! Creates an inbound session for sending/receiving messages from a received 'prekey' message.
    static std::variant<std::unique_ptr<QOlmSession>, OlmError> createInboundSession(QOlmAccount *account, const Message &preKeyMessage);
    static std::variant<std::unique_ptr<QOlmSession>, OlmError> createInboundSessionFrom(QOlmAccount *account, const QString &theirIdentityKey, const Message &preKeyMessage);
    static std::variant<std::unique_ptr<QOlmSession>, OlmError> createOutboundSession(QOlmAccount *account, const QString &theirIdentityKey, const QString &theirOneTimeKey);
    //! Serialises an `QOlmSession` to encrypted Base64.
    std::variant<QByteArray, OlmError> pickle(const PicklingMode &mode);
    //! Deserialises from encrypted Base64 that was previously obtained by pickling a `QOlmSession`.
    static std::variant<std::unique_ptr<QOlmSession>, OlmError> unpickle(QByteArray &pickled, const PicklingMode &mode);
    //! Encrypts a plaintext message using the session.
    Message encrypt(const QString &plaintext);

    //! Decrypts a message using this session. Decoding is lossy, meaing if
    //! the decrypted plaintext contains invalid UTF-8 symbols, they will
    //! be returned as `U+FFFD` (�).
    std::variant<QString, OlmError> decrypt(const Message &message) const;

    //! Get a base64-encoded identifier for this session.
    QByteArray sessionId() const;

    //! The type of the next message that will be returned from encryption.
    Message::Type encryptMessageType();

    //! Checker for any received messages for this session.
    bool hasReceivedMessage() const;

    //! Checks if the 'prekey' message is for this in-bound session.
    std::variant<bool, OlmError> matchesInboundSession(Message &preKeyMessage);

    QOlmSession(OlmSession* session);
private:
    //! Helper function for creating new sessions and handling errors.
    static OlmSession* create();
    static std::variant<std::unique_ptr<QOlmSession>, OlmError> createInbound(QOlmAccount *account, const Message& preKeyMessage, bool from = false, const QString& theirIdentityKey = "");
    OlmSession* m_session;
};

//using QOlmSessionPtr = std::unique_ptr<QOlmSession>;

} //namespace Quotient

#endif // Quotient_E2EE_ENABLED
