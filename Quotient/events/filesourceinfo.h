// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Quotient/converters.h>

#include <array>

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

struct QUOTIENT_API EncryptedFileMetadata {
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
};

#ifdef Quotient_E2EE_ENABLED
QUOTIENT_API std::pair<EncryptedFileMetadata, QByteArray> encryptFile(
    const QByteArray& plainText);
QUOTIENT_API QByteArray decryptFile(const QByteArray& ciphertext,
                                    const EncryptedFileMetadata& metadata);
#endif

template <>
struct QUOTIENT_API JsonObjectConverter<EncryptedFileMetadata> {
    static void dumpTo(QJsonObject& jo, const EncryptedFileMetadata& pod);
    static void fillFrom(const QJsonObject& jo, EncryptedFileMetadata& pod);
};

template <>
struct QUOTIENT_API JsonObjectConverter<JWK> {
    static void dumpTo(QJsonObject& jo, const JWK& pod);
    static void fillFrom(const QJsonObject& jo, JWK& pod);
};

using FileSourceInfo = std::variant<QUrl, EncryptedFileMetadata>;

QUOTIENT_API QUrl getUrlFromSourceInfo(const FileSourceInfo& fsi);

QUOTIENT_API void setUrlInSourceInfo(FileSourceInfo& fsi, const QUrl& newUrl);

// The way FileSourceInfo is stored in JSON requires an extra parameter so
// the original template is not applicable
template <>
void fillJson(QJsonObject&, const FileSourceInfo&) = delete;

//! \brief Export FileSourceInfo to a JSON object
//!
//! Depending on what is stored inside FileSourceInfo, this function will insert
//! - a key-to-string pair where key is taken from jsonKeys[0] and the string
//!   is the URL, if FileSourceInfo stores a QUrl;
//! - a key-to-object mapping where key is taken from jsonKeys[1] and the object
//!   is the result of converting EncryptedFileMetadata to JSON,
//!   if FileSourceInfo stores EncryptedFileMetadata
QUOTIENT_API void fillJson(QJsonObject& jo,
                           const std::array<QLatin1String, 2>& jsonKeys,
                           const FileSourceInfo& fsi);

} // namespace Quotient
