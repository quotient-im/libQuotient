// SPDX-FileCopyrightText: 2022 Tobias Fella <fella@posteo.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QtTest/QtTest>

class TestFileCrypto : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void encryptDecryptData();
};
