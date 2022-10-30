// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "e2ee/e2ee_common.h"

#include "logging.h"

#include <QtCore/QRandomGenerator>

#include <openssl/crypto.h>

using namespace Quotient;

QByteArray Quotient::toKey(const Quotient::PicklingMode &mode)
{
    if (std::holds_alternative<Quotient::Unencrypted>(mode)) {
        return {};
    }
    return std::get<Quotient::Encrypted>(mode).key;
}

RandomBuffer::RandomBuffer(size_t size) : QByteArray(bufferForOlm(size))
{
    QRandomGenerator::system()->generate(begin(), end());
}

QByteArray Quotient::bufferForOlm(size_t bufferSize)
{
    if (bufferSize < std::numeric_limits<QByteArray::size_type>::max())
        return { static_cast<QByteArray::size_type>(bufferSize), '\0' };

    qCritical(E2EE) << "Too large buffer size:" << bufferSize;
    // Zero-length QByteArray is an almost guaranteed way to cause
    // an internal error in QOlm* classes, unless checked
    return {};
}
