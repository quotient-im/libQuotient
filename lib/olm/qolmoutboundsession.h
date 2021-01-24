// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include "olm/olm.h" // from Olm
#include "olm/errors.h"
#include "olm/e2ee.h"

namespace Quotient {

//! An out-bound group session is responsible for encrypting outgoing
//! communication in a Megolm session.
class QOlmOutboundGroupSession
{
public:
    ~QOlmOutboundGroupSession();
    //! Creates a new instance of `QOlmOutboundGroupSession`.
    static std::variant<QOlmOutboundGroupSession, OlmError> create();
    //! Serialises an `QOlmOutboundGroupSession` to encrypted Base64.
    std::variant<QByteArray, OlmError> pickle(const PicklingMode &mode);
    //! Deserialises from encrypted Base64 that was previously obtained by
    //! pickling an `QOlmOutboundGroupSession`.
    static std::variant<QOlmOutboundGroupSession, OlmError> unpickle(QByteArray &pickled, const PicklingMode &mode);
    //! Encrypts a plaintext message using the session.
    std::variant<QString, OlmError> encrypt(QString &plaintext);

    //! Get the current message index for this session.
    //!
    //! Each message is sent with an increasing index; this returns the
    //! index for the next message.
    uint32_t sessionMessageIndex() const;

    //! Get a base64-encoded identifier for this session.
    std::variant<QByteArray, OlmError> sessionId() const;

    //! Get the base64-encoded current ratchet key for this session.
    //!
    //! Each message is sent with a different ratchet key. This function returns the
    //! ratchet key that will be used for the next message.
    std::variant<QByteArray, OlmError> sessionKey() const;
private:
    QOlmOutboundGroupSession(OlmOutboundGroupSession *groupSession, const QByteArray &groupSessionBuf);
    OlmOutboundGroupSession *m_groupSession;
    QByteArray m_buffer;
};
}
