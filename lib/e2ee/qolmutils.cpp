// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "e2ee/qolmutils.h"
#include <QtCore/QRandomGenerator>

using namespace Quotient;

QByteArray Quotient::toKey(const Quotient::PicklingMode &mode)
{
    if (std::holds_alternative<Quotient::Unencrypted>(mode)) {
        return {};
    }
    return std::get<Quotient::Encrypted>(mode).key;
}

QByteArray Quotient::getRandom(size_t bufferSize)
{
    QByteArray buffer(bufferSize, '0');
    QRandomGenerator::system()->generate(buffer.begin(), buffer.end());
    return buffer;
}
