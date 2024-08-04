// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Quotient/e2ee/e2ee_common.h>
#include <Quotient/e2ee/qolmmessage.h>

#include <vodozemac/vodozemac.h>

struct OlmSession;

namespace Quotient {

class QOlmAccount;

//! Either an outbound or inbound session for secure communication.
class QUOTIENT_API QOlmSession
{
public:
    //! Serialises an `QOlmSession` to encrypted Base64.
    QByteArray pickle(const PicklingKey& key) const;

    //! Deserialises from encrypted Base64 previously made with pickle()
    static QOlmExpected<QOlmSession> unpickle(QByteArray&& pickled,
                                              const PicklingKey& key);

    //! Encrypts a plaintext message using the session.
    QOlmMessage encrypt(const QByteArray& plaintext);

    //! Decrypts a message using this session. Decoding is lossy, meaning if
    //! the decrypted plaintext contains invalid UTF-8 symbols, they will
    //! be returned as `U+FFFD`.
    QOlmExpected<QByteArray> tryDecrypt(const QOlmMessage &message);

    //! Get a base64-encoded identifier for this session.
    QByteArray sessionId() const;

    friend bool operator<(const QOlmSession& lhs, const QOlmSession& rhs)
    {
        return lhs.sessionId() < rhs.sessionId();
    }

    OlmErrorCode lastErrorCode() const;
    const char* lastError() const;

    explicit QOlmSession(rust::Box<olm::Session> session);
private:
    QOlmSession();
    rust::Box<olm::Session> m_session;

    friend class QOlmAccount;
};
} //namespace Quotient
