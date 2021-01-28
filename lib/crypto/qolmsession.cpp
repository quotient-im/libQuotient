// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "crypto/qolmsession.h"

using namespace Quotient;

std::optional<OlmMessage> fromTypeAndCipthertext(size_t messageType, const QByteArray &ciphertext)
{
    if (messageType == OLM_MESSAGE_TYPE_PRE_KEY) {
        return PreKeyMessage { ciphertext };
    } else if (messageType == OLM_MESSAGE_TYPE_MESSAGE) {
        return Message { ciphertext };
    }
    return std::nullopt;
}

std::pair<OlmMessageType, QByteArray> toPair(const OlmMessage &message)
{
    return std::visit([](auto &arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Message>) {
            return std::make_pair<OlmMessageType, QByteArray>(MessageType, QByteArray(arg.message));
        } else if constexpr (std::is_same_v<T, PreKeyMessage>) {
            return std::make_pair<OlmMessageType, QByteArray>(PreKeyType, QByteArray(arg.message));
        }
    }, message);
}
