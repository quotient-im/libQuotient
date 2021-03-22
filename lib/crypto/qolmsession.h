// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QDebug>
#include <olm/olm.h> // FIXME: OlmSession
#include "crypto/e2ee.h"
#include "crypto/qolmmessage.h"
#include "crypto/qolmerrors.h"
#include "crypto/qolmaccount.h"

namespace Quotient {

class QOlmAccount;
class QOlmSession;


//! Either an outbound or inbound session for secure communication.
class QOlmSession
{
public:
    ~QOlmSession();
    //! Creates an inbound session for sending/receiving messages from a received 'prekey' message.
    static std::variant<std::unique_ptr<QOlmSession>, QOlmError> createInboundSession(QOlmAccount *account, const QOlmMessage &preKeyMessage);
    static std::variant<std::unique_ptr<QOlmSession>, QOlmError> createInboundSessionFrom(QOlmAccount *account, const QString &theirIdentityKey, const QOlmMessage &preKeyMessage);
    static std::variant<std::unique_ptr<QOlmSession>, QOlmError> createOutboundSession(QOlmAccount *account, const QString &theirIdentityKey, const QString &theirOneTimeKey);
    //! Serialises an `QOlmSession` to encrypted Base64.
    std::variant<QByteArray, QOlmError> pickle(const PicklingMode &mode);
    //! Deserialises from encrypted Base64 that was previously obtained by pickling a `QOlmSession`.
    static std::variant<std::unique_ptr<QOlmSession>, QOlmError> unpickle(const QByteArray &pickled, const PicklingMode &mode);
    //! Encrypts a plaintext message using the session.
    QOlmMessage encrypt(const QString &plaintext);

    //! Decrypts a message using this session. Decoding is lossy, meaing if
    //! the decrypted plaintext contains invalid UTF-8 symbols, they will
    //! be returned as `U+FFFD` (�).
    std::variant<QString, QOlmError> decrypt(const QOlmMessage &message) const;

    //! Get a base64-encoded identifier for this session.
    QByteArray sessionId() const;

    //! The type of the next message that will be returned from encryption.
    QOlmMessage::Type encryptMessageType();

    //! Checker for any received messages for this session.
    bool hasReceivedMessage() const;

    //! Checks if the 'prekey' message is for this in-bound session.
    std::variant<bool, QOlmError> matchesInboundSession(const QOlmMessage &preKeyMessage) const;

    //! Checks if the 'prekey' message is for this in-bound session.
    std::variant<bool, QOlmError> matchesInboundSessionFrom(const QString &theirIdentityKey, const QOlmMessage &preKeyMessage) const;

    friend bool operator<(const QOlmSession& lhs, const QOlmSession& rhs)
    {
        return lhs.sessionId() < rhs.sessionId();
    }

    friend bool operator<(const std::unique_ptr<QOlmSession> &lhs, const std::unique_ptr<QOlmSession> &rhs) {
        return *lhs < *rhs;
    }

    OlmSession *raw() const
    {
        return m_session;
    }
    QOlmSession(OlmSession* session);
private:
    //! Helper function for creating new sessions and handling errors.
    static OlmSession* create();
    static std::variant<std::unique_ptr<QOlmSession>, QOlmError> createInbound(QOlmAccount *account, const QOlmMessage& preKeyMessage, bool from = false, const QString& theirIdentityKey = "");
    OlmSession* m_session;
};


//using QOlmSessionPtr = std::unique_ptr<QOlmSession>;

} //namespace Quotient
