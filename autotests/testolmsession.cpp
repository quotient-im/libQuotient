// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Quotient/e2ee/qolmsession.h>
#include <Quotient/e2ee/qolmaccount.h>
#include "testolmsession.h"

using namespace Quotient;
using namespace Qt::Literals::StringLiterals;

void TestOlmSession::olmEncryptDecrypt()
{
    auto alice = QOlmAccount::newAccount(nullptr, u"@alice:foo.bar"_s, u"ABCDEF"_s);
    auto bob = QOlmAccount::newAccount(nullptr, u"@bob:foo.bar"_s, u"ABCDEF"_s);

    bob->generateOneTimeKeys(1);
    auto otk = bob->oneTimeKeys().keys.values()[0];
    auto outboundSession = alice->createOutboundSession(bob->identityKeys().curve25519.toLatin1(), otk.toLatin1());
    auto message = outboundSession.encrypt("Hello World!"_ba);
    auto [inboundSession, plaintext] = bob->createInboundSession(alice->identityKeys().curve25519.toLatin1(), message);
    QCOMPARE(plaintext.toLatin1(), "Hello World!"_ba);
}

QTEST_GUILESS_MAIN(TestOlmSession)
