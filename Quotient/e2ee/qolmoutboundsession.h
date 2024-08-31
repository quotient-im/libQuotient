// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Quotient/e2ee/e2ee_common.h>

#include <vodozemac.h>

namespace Quotient {

//! An out-bound group session is responsible for encrypting outgoing
//! communication in a Megolm session.
class QUOTIENT_API QOlmOutboundGroupSession
{
public:
    //! Serialises a `QOlmOutboundGroupSession` to encrypted Base64.
    QByteArray pickle(const PicklingKey &key) const;
    //! Deserialises from encrypted Base64 that was previously obtained by
    //! pickling a `QOlmOutboundGroupSession`.
    static QOlmExpected<QOlmOutboundGroupSession> unpickle(QByteArray&& pickled, const PicklingKey& key);

    static QOlmExpected<QOlmOutboundGroupSession> create();

    //! Encrypts a plaintext message using the session.
    QByteArray encrypt(const QByteArray& plaintext);

    //! Get the current message index for this session.
    //!
    //! Each message is sent with an increasing index; this returns the
    //! index for the next message.
    uint32_t sessionMessageIndex() const;

    //! Get a base64-encoded identifier for this session.
    QByteArray sessionId() const;

    //! Get the base64-encoded current ratchet key for this session.
    //!
    //! Each message is sent with a different ratchet key. This function returns the
    //! ratchet key that will be used for the next message.
    QByteArray sessionKey() const;

    int messageCount() const;
    void setMessageCount(int messageCount);

    QDateTime creationTime() const;
    void setCreationTime(const QDateTime& creationTime);

private:
    int m_messageCount = 0;
    QDateTime m_creationTime = QDateTime::currentDateTime();
    rust::Box<megolm::GroupSession> olmData;
    QOlmOutboundGroupSession(rust::Box<megolm::GroupSession> session);
};

} // namespace Quotient
