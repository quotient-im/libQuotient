// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "qolmmessage.h"

#include <Quotient/util.h>

using namespace Quotient;

QOlmMessage::QOlmMessage(QByteArray ciphertext, size_t type)
    : QByteArray(std::move(ciphertext))
    , m_messageType(type)
{
    Q_ASSERT_X(!isEmpty(), "olm message", "Ciphertext is empty");
}

size_t QOlmMessage::type() const
{
    return m_messageType;
}

QByteArray QOlmMessage::toCiphertext() const
{
    return SLICE(*this, QByteArray);
}

QOlmMessage QOlmMessage::fromCiphertext(const QByteArray &ciphertext)
{
    return QOlmMessage(ciphertext, 0 /* TODO correct?*/);
}
