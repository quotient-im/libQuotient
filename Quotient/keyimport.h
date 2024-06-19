// SPDX-FileCopyrightText: 2024 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QObject>

#include "expected.h"
#include "quotient_export.h"

class TestKeyImport;

namespace Quotient
{
class Connection;
}

namespace Quotient
{

class QUOTIENT_API KeyImport : public QObject
{
    Q_OBJECT

public:
    enum Error {
        Success,
        InvalidPassphrase,
        InvalidData,
        OtherError,
    };
    Q_ENUM(Error)

    using QObject::QObject;

    Q_INVOKABLE Error importKeys(QString data, const QString& passphrase,
                                 const Quotient::Connection* connection);
    Q_INVOKABLE Quotient::Expected<QByteArray, Error> exportKeys(const QString& passphrase, Quotient::Connection* connection);

    friend class ::TestKeyImport;
private:
    Quotient::Expected<QJsonArray, Error> decrypt(QString data, const QString& passphrase);
    Quotient::Expected<QByteArray, Error> encrypt(QJsonArray sessions, const QString& passphrase);
};

}
