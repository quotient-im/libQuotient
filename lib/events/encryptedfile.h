// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "converters.h"

namespace Quotient {
/**
 * JSON Web Key object as specified in
 * https://spec.matrix.org/unstable/client-server-api/#extensions-to-mroommessage-msgtypes
 * The only currently relevant member is `k`, the rest needs to be set to the defaults specified in the spec.
 */
struct JWK
{
    Q_GADGET
    Q_PROPERTY(QString kty MEMBER kty CONSTANT)
    Q_PROPERTY(QStringList keyOps MEMBER keyOps CONSTANT)
    Q_PROPERTY(QString alg MEMBER alg CONSTANT)
    Q_PROPERTY(QString k MEMBER k CONSTANT)
    Q_PROPERTY(bool ext MEMBER ext CONSTANT)

public:
    QString kty;
    QStringList keyOps;
    QString alg;
    QString k;
    bool ext;
};

struct QUOTIENT_API EncryptedFile
{
    Q_GADGET
    Q_PROPERTY(QUrl url MEMBER url CONSTANT)
    Q_PROPERTY(JWK key MEMBER key CONSTANT)
    Q_PROPERTY(QString iv MEMBER iv CONSTANT)
    Q_PROPERTY(QHash<QString, QString> hashes MEMBER hashes CONSTANT)
    Q_PROPERTY(QString v MEMBER v CONSTANT)

public:
    QUrl url;
    JWK key;
    QString iv;
    QHash<QString, QString> hashes;
    QString v;

    QByteArray decryptFile(const QByteArray &ciphertext) const;
};

template <>
struct QUOTIENT_API JsonObjectConverter<EncryptedFile> {
    static void dumpTo(QJsonObject& jo, const EncryptedFile& pod);
    static void fillFrom(const QJsonObject& jo, EncryptedFile& pod);
};

template <>
struct QUOTIENT_API JsonObjectConverter<JWK> {
    static void dumpTo(QJsonObject& jo, const JWK& pod);
    static void fillFrom(const QJsonObject& jo, JWK& pod);
};
} // namespace Quotient
