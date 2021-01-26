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

//! Either an outbound or inbound session for secure communication.
class QOlmSession
{
public:
    ~QOlmSession();
    //! Creates an inbound session for sending/receiving messages from a received 'prekey' message.
    static std::unique_ptr<QOlmSession> createInboundSession(QOlmAccount& account, const Message& preKeyMessage);
    static std::unique_ptr<QOlmSession> createInboundSessionFrom(QOlmAccount& account, const QString& theirIdentityKey, const Message& preKeyMessage);
    static std::unique_ptr<QOlmSession> createOutboundSession(QOlmAccount& account, const QString& theirIdentityKey, const QString& theirOneTimeKey);
    //! Serialises an `QOlmSession` to encrypted Base64.
    std::variant<QByteArray, OlmError> pickle(const PicklingMode &mode);
    //! Deserialises from encrypted Base64 that was previously obtained by pickling a `QOlmSession`.
    static std::variant<std::unique_ptr<QOlmSession>, OlmError> unpickle(QByteArray &pickled, const PicklingMode &mode);
    //! Encrypts a plaintext message using the session.
    std::variant<Message, OlmError> encrypt(const QString &plaintext);
    // TODO: WiP

    //! Get a base64-encoded identifier for this session.
    QByteArray sessionId() const;

    QOlmSession(OlmSession* session);
private:
    //! Helper function for creating new sessions and handling errors.
    static OlmSession* create();
    static std::unique_ptr<QOlmSession> createInbound(QOlmAccount& account, const Message& preKeyMessage, bool from = false, const QString& theirIdentityKey = "");
    OlmSession* m_session;
};

} //namespace Quotient

#endif // Quotient_E2EE_ENABLED
