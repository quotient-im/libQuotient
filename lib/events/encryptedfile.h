// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPl-2.1-or-later

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
struct JsonObjectConverter<EncryptedFile> {
    static void dumpTo(QJsonObject& jo, const EncryptedFile& pod)
    {
        addParam<>(jo, QStringLiteral("url"), pod.url);
        addParam<>(jo, QStringLiteral("key"), pod.key);
        addParam<>(jo, QStringLiteral("iv"), pod.iv);
        addParam<>(jo, QStringLiteral("hashes"), pod.hashes);
        addParam<>(jo, QStringLiteral("v"), pod.v);
    }
    static void fillFrom(const QJsonObject& jo, EncryptedFile& pod)
    {
        fromJson(jo.value("url"_ls), pod.url);
        fromJson(jo.value("key"_ls), pod.key);
        fromJson(jo.value("iv"_ls), pod.iv);
        fromJson(jo.value("hashes"_ls), pod.hashes);
        fromJson(jo.value("v"_ls), pod.v);
    }
};

template <>
struct JsonObjectConverter<JWK> {
    static void dumpTo(QJsonObject& jo, const JWK& pod)
    {
        addParam<>(jo, QStringLiteral("kty"), pod.kty);
        addParam<>(jo, QStringLiteral("key_ops"), pod.keyOps);
        addParam<>(jo, QStringLiteral("alg"), pod.alg);
        addParam<>(jo, QStringLiteral("k"), pod.k);
        addParam<>(jo, QStringLiteral("ext"), pod.ext);
    }
    static void fillFrom(const QJsonObject& jo, JWK& pod)
    {
        fromJson(jo.value("kty"_ls), pod.kty);
        fromJson(jo.value("key_ops"_ls), pod.keyOps);
        fromJson(jo.value("alg"_ls), pod.alg);
        fromJson(jo.value("k"_ls), pod.k);
        fromJson(jo.value("ext"_ls), pod.ext);
    }
};
} // namespace Quotient
