// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QtTest/QtTest>

class TestOlmSession : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void olmOutboundSessionCreation();
    void olmEncryptDecrypt();
    void correctSessionOrdering();
};
