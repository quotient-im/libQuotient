// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef Quotient_E2EE_ENABLED
#include "olm/message.h"

using namespace Quotient;

Message::Message(const QByteArray &ciphertext, Message::Type type)
    : QByteArray(std::move(ciphertext))
    , m_messageType(type)
{
    Q_ASSERT_X(!ciphertext.isEmpty(), "olm message", "Ciphertext is empty");
}

Message::Type Message::type() const
{
    return m_messageType;
}

QByteArray Message::toCiphertext() const
{
    return QByteArray(*this);
}

Message Message::fromCiphertext(const QByteArray &ciphertext)
{
    return Message(ciphertext, Message::General);
}


#endif // Quotient_E2EE_ENABLED



