// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QByteArray>
#include <variant>
#include "olm/olm.h"
#include "olm/errors.h"
#include "olm/e2ee.h"

namespace Quotient {

//! An in-bound group session is responsible for decrypting incoming
//! communication in a Megolm session.
struct QOlmInboundGroupSession
{
public:
    //! Creates a new instance of `OlmInboundGroupSession`.
    static std::variant<QOlmInboundGroupSession, OlmError> create(const QString &key);
    //! Import an inbound group session, from a previous export.
    static std::variant<QOlmInboundGroupSession, OlmError> import(const QString &key);
    //! Serialises an `OlmInboundGroupSession` to encrypted Base64.
    std::variant<QByteArray, OlmError> pickle(const PicklingMode &mode) const;
    //! Deserialises from encrypted Base64 that was previously obtained by pickling
    //! an `OlmInboundGroupSession`.
    static std::variant<QOlmInboundGroupSession, OlmError> unpicke(QByteArray &picked, const PicklingMode &mode);
    //! Decrypts ciphertext received for this group session.
    std::variant<std::pair<QString, uint32_t>, OlmError> decrypt(QString &message);
    //! Export the base64-encoded ratchet key for this session, at the given index,
    //! in a format which can be used by import.
    std::variant<QByteArray, OlmError> exportSession(uint32_t messageIndex);
    //! Get the first message index we know how to decrypt.
    uint32_t firstKnownIndex() const;
    //! Get a base64-encoded identifier for this session.
    std::variant<QByteArray, OlmError> sessionId() const;
    bool isVerified() const;
private:
    QOlmInboundGroupSession(OlmInboundGroupSession *session, const QByteArray &buffer);
    OlmInboundGroupSession *m_groupSession;
    QByteArray m_buffer;
};
} // namespace Quotient
