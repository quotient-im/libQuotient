// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2022 Kitsune Ral <kitsune-ral@users.sf.net>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testutils.h"

#include <QtTest/QSignalSpy>

#include <QtTest>

std::shared_ptr<Connection> createTestConnection(QObject* testObject,
                                                 const QString& localUserName,
                                                 const QString& secret,
                                                 const QString& deviceName)
{
    NetworkAccessManager::instance()->ignoreSslErrors(true);
    auto c = std::make_shared<Connection>();
    c->resolveServer('@' % localUserName % ":localhost:1234");
    QObject::connect(c.get(), &Connection::loginFlowsChanged, testObject,
                     [c, localUserName, secret, deviceName] {
                         c->loginWithPassword(localUserName, secret, deviceName);
                     });
    QObject::connect(c.get(), &Connection::networkError,
                     [](const QString& error) {
                         QWARN(qUtf8Printable(error));
                         QFAIL("Network error: make sure synapse is running");
                     });
    QObject::connect(c.get(), &Connection::loginError, [](const QString& error) {
        QWARN(qUtf8Printable(error));
        QFAIL("Login failed");
    });
    QSignalSpy spy(c.get(), &Connection::loginFlowsChanged);
    QSignalSpy spy2(c.get(), &Connection::connected);
    if (!QTest::qVerify(static_cast<bool>(spy.wait(10000)), "loginFlows spy",
                        "", __FILE__, __LINE__))
        return nullptr;
    if (!QTest::qVerify(static_cast<bool>(spy2.wait(10000)), "connected() spy",
                        "", __FILE__, __LINE__))
        return nullptr;
    return c;
}
