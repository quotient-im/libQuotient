// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef Quotient_E2EE_ENABLED
#include "qolmmessage.h"

using namespace Quotient;

QOlmMessage::QOlmMessage(const QByteArray &ciphertext, QOlmMessage::Type type)
    : QByteArray(std::move(ciphertext))
    , m_messageType(type)
{
    Q_ASSERT_X(!ciphertext.isEmpty(), "olm message", "Ciphertext is empty");
}

QOlmMessage::QOlmMessage(const QOlmMessage &message)
    : QByteArray(message)
    , m_messageType(message.type())
{
}

QOlmMessage::Type QOlmMessage::type() const
{
    return m_messageType;
}

QByteArray QOlmMessage::toCiphertext() const
{
    return QByteArray(*this);
}

QOlmMessage QOlmMessage::fromCiphertext(const QByteArray &ciphertext)
{
    return QOlmMessage(ciphertext, QOlmMessage::General);
}


#endif // Quotient_E2EE_ENABLED



