// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QtTest/QtTest>

class TestOlmAccount : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pickleUnpickedTest();
    void identityKeysValid();
    void signatureValid();
    void oneTimeKeysValid();
    //void removeOneTimeKeys();
    void deviceKeys();
    void encryptedFile();
};
