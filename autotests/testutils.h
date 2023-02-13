// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <connection.h>
#include <networkaccessmanager.h>

#include <QtTest/QSignalSpy>

using namespace Quotient;

#define CREATE_CONNECTION(VAR, USERNAME, SECRET, DEVICE_NAME)                  \
    NetworkAccessManager::instance()->ignoreSslErrors(true);                   \
    auto VAR = std::make_shared<Connection>();                                 \
    (VAR)->resolveServer("@" USERNAME ":localhost:1234"_ls);                   \
    connect((VAR).get(), &Connection::loginFlowsChanged, this, [=] {           \
        (VAR)->loginWithPassword((USERNAME), SECRET, DEVICE_NAME, QString());  \
    });                                                                        \
    connect((VAR).get(), &Connection::networkError, [](const QString& error) { \
        QWARN(qUtf8Printable(error));                                          \
        QFAIL("Network error: make sure synapse is running");                  \
    });                                                                        \
    connect((VAR).get(), &Connection::loginError, [](const QString& error) {   \
        QWARN(qUtf8Printable(error));                                          \
        QFAIL("Login failed");                                                 \
    });                                                                        \
    QSignalSpy spy##VAR((VAR).get(), &Connection::loginFlowsChanged);          \
    QSignalSpy spy2##VAR((VAR).get(), &Connection::connected);                 \
    QVERIFY(spy##VAR.wait(10000));                                             \
    QVERIFY(spy2##VAR.wait(10000));
