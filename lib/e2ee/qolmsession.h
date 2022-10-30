// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "e2ee/e2ee_common.h"
#include "e2ee/qolmmessage.h"

struct OlmSession;

namespace Quotient {

class QOlmAccount;

//! Either an outbound or inbound session for secure communication.
class QUOTIENT_API QOlmSession
{
public:
    //! Serialises an `QOlmSession` to encrypted Base64.
    QByteArray pickle(const PicklingMode &mode) const;

    //! Deserialises from encrypted Base64 previously made with pickle()
    static QOlmExpected<QOlmSession> unpickle(QByteArray&& pickled,
                                              const PicklingMode& mode);

    //! Encrypts a plaintext message using the session.
    QOlmMessage encrypt(const QByteArray& plaintext) const;

    //! Decrypts a message using this session. Decoding is lossy, meaning if
    //! the decrypted plaintext contains invalid UTF-8 symbols, they will
    //! be returned as `U+FFFD` (�).
    QOlmExpected<QByteArray> decrypt(const QOlmMessage &message) const;

    //! Get a base64-encoded identifier for this session.
    QByteArray sessionId() const;

    //! Checker for any received messages for this session.
    bool hasReceivedMessage() const;

    //! Checks if the 'prekey' message is for this in-bound session.
    bool matchesInboundSession(const QOlmMessage& preKeyMessage) const;

    //! Checks if the 'prekey' message is for this in-bound session.
    bool matchesInboundSessionFrom(
        const QString& theirIdentityKey, const QOlmMessage& preKeyMessage) const;

    friend bool operator<(const QOlmSession& lhs, const QOlmSession& rhs)
    {
        return lhs.sessionId() < rhs.sessionId();
    }

    OlmErrorCode lastErrorCode() const;
    const char* lastError() const;

private:
    QOlmSession();
    CStructPtr<OlmSession> olmDataHolder;
    OlmSession* olmData = olmDataHolder.get();

    friend class QOlmAccount;
};
} //namespace Quotient
