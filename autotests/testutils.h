// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <connection.h>
#include <networkaccessmanager.h>

using namespace Quotient;

std::shared_ptr<Connection> createTestConnection(QObject* testObject,
                                 const QString& localUserName,
                                 const QString& secret,
                                 const QString& deviceName);

#define CREATE_CONNECTION(VAR, USERNAME, SECRET, DEVICE_NAME) \
    auto VAR = createTestConnection(this, USERNAME, SECRET, DEVICE_NAME);
