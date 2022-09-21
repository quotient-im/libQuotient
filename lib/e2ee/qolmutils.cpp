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

RandomBuffer::RandomBuffer(size_t size)
    : QByteArray(static_cast<int>(size), '\0')
{
    QRandomGenerator::system()->generate(begin(), end());
}
