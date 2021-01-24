// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef Quotient_E2EE_ENABLED
#include "olm/utils.h"
#include <QDebug>
#include <openssl/rand.h>

using namespace Quotient;

QByteArray Quotient::toKey(const Quotient::PicklingMode &mode)
{
    if (std::holds_alternative<Quotient::Unencrypted>(mode)) {
        return "";
    }
    return std::get<Quotient::Encrypted>(mode).key;
}

QByteArray Quotient::getRandom(size_t bufferSize)
{
    QByteArray buffer(bufferSize, '0');
    RAND_bytes(reinterpret_cast<uint8_t *>(buffer.data()), buffer.size());
    return buffer;
}
#endif
