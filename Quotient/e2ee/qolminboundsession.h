// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Quotient/e2ee/e2ee_common.h>

struct OlmInboundGroupSession;

namespace Quotient {

//! An in-bound group session is responsible for decrypting incoming
//! communication in a Megolm session.
class QUOTIENT_API QOlmInboundGroupSession
{
public:
    //! Creates a new instance of `OlmInboundGroupSession`.
    static QOlmExpected<QOlmInboundGroupSession> create(const QByteArray& key);
    //! Import an inbound group session, from a previous export.
    static QOlmExpected<QOlmInboundGroupSession> importSession(
        const QByteArray& key);
    //! Serialises an `OlmInboundGroupSession` to encrypted Base64.
    QByteArray pickle(const PicklingKey& key) const;
    //! Deserialises from encrypted Base64 that was previously obtained by pickling
    //! an `OlmInboundGroupSession`.
    static QOlmExpected<QOlmInboundGroupSession> unpickle(
        QByteArray&& pickled, const PicklingKey& key);
    //! Decrypts ciphertext received for this group session.
    QOlmExpected<std::pair<QByteArray, uint32_t> > decrypt(const QByteArray& message);
    //! Export the base64-encoded ratchet key for this session, at the given index,
    //! in a format which can be used by import.
    QOlmExpected<QByteArray> exportSession(uint32_t messageIndex);
    //! Get the first message index we know how to decrypt.
    uint32_t firstKnownIndex() const;
    //! Get a base64-encoded identifier for this session.
    QByteArray sessionId() const;
    bool isVerified() const;

    //! The olm session that this session was received from.
    //! Required to get the device this session is from.
    QByteArray olmSessionId() const;
    void setOlmSessionId(const QByteArray& newOlmSessionId);

    //! The sender of this session.
    QString senderId() const;
    void setSenderId(const QString& senderId);

    OlmErrorCode lastErrorCode() const;
    const char* lastError() const;

    //TODO: why?
    QOlmInboundGroupSession();
private:
    CStructPtr<OlmInboundGroupSession> m_groupSession;
    QByteArray m_olmSessionId;
    QString m_senderId;
    OlmInboundGroupSession* olmData = m_groupSession.get();
};

using QOlmInboundGroupSessionPtr = std::unique_ptr<QOlmInboundGroupSession>;
} // namespace Quotient
