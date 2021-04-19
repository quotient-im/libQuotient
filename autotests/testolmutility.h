// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QTest>

class TestOlmUtility : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void canonicalJSON();
    void verifySignedOneTimeKey();
    void validUploadKeysRequest();
};
