// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <variant>
#include <olm/olm.h>
#include "crypto/e2ee.h"
#include "crypto/errors.h"

namespace Quotient {

//! An encrypted Olm message.
struct Message {
    QByteArray message;
};

//! A encrypted Olm pre-key message.
//!
//! This message, unlike a normal Message, can be used to create new Olm sessions.
struct PreKeyMessage
{
    QByteArray message;
};

enum OlmMessageType
{
    PreKeyType,
    MessageType,
};

using OlmMessage = std::variant<Message, PreKeyMessage>;

std::optional<OlmMessage> fromTypeAndCipthertext(size_t messageType, const QByteArray &ciphertext);

std::pair<OlmMessageType, QByteArray> toPair(const OlmMessage &message);

//class QOlmSession
//{
//    /// Creates an inbound session for sending/receiving messages from a received 'prekey' message.
//    static std::variant<std::unique_ptr<QOlmSession>, OlmError> createInboundSession(const QOlmAccount &account,
//            PreKeyMessage &message);
//
////private:
//    //static std::variant<std::unique_ptr<QOlmSession>, OlmError> createSessionWith(std::function<std::variant<size_t(OlmSession *)>> func);
//}

}
