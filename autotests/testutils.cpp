// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2022 Kitsune Ral <kitsune-ral@users.sf.net>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testutils.h"

#include <Quotient/connection.h>
#include <Quotient/networkaccessmanager.h>

#include <QtTest/QSignalSpy>

using Quotient::Connection;

bool waitForSignal(auto objPtr, auto signal)
{
    return QSignalSpy(std::to_address(objPtr), signal).wait(10000);
}

std::shared_ptr<Quotient::Connection> Quotient::createTestConnection(
    const QString& localUserName, const QString& secret,
    const QString& deviceName)
{
    static constexpr auto homeserverAddr = "localhost:1234"_ls;
    NetworkAccessManager::instance()->ignoreSslErrors(true);
    auto c = std::make_shared<Connection>();
    const QString userId{ u'@' % localUserName % u':' % homeserverAddr };
    c->setHomeserver(QUrl(u"https://" % homeserverAddr));
    if (!waitForSignal(c, &Connection::loginFlowsChanged)
        || !c->supportsPasswordAuth()) {
        qCritical().noquote() << "Can't use password login at" << homeserverAddr
                              << "- check that the homeserver is running";
        return nullptr;
    }
    c->loginWithPassword(localUserName, secret, deviceName);
    if (!waitForSignal(c, &Connection::connected)) {
        qCritical().noquote()
            << "Could not achieve the logged in state for" << userId
            << "- check the credentials in the test code and at the homeserver";
        return nullptr;
    }
    return c;
}
