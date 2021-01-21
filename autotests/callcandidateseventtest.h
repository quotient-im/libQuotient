// SPDX-FileCopyrightText: 2020 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QtTest/QtTest>

class TestCallCandidatesEvent : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fromJson();
};
