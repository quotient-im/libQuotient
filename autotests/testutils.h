// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtTest/QTest>

#include <memory>

namespace Quotient {

class Connection;

std::shared_ptr<Connection> createTestConnection(QLatin1StringView localUserName,
                                                 QLatin1StringView secret,
                                                 QLatin1StringView deviceName);
}

#define CREATE_CONNECTION(VAR, USERNAME, SECRET, DEVICE_NAME)             \
    const auto VAR = createTestConnection(USERNAME, SECRET, DEVICE_NAME); \
    if (!VAR)                                                             \
        QFAIL("Could not set up test connection");
