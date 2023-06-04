/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include <Quotient/converters.h>

namespace Quotient {
/// Cross signing key
struct CrossSigningKey {
    /// The ID of the user the key belongs to.
    QString userId;

    /// What the key is used for.
    QStringList usage;

    /// The public key.  The object must have exactly one property, whose name
    /// is in the form `<algorithm>:<unpadded_base64_public_key>`, and whose
    /// value is the unpadded base64 public key.
    QHash<QString, QString> keys;

    /// Signatures of the key, calculated using the process described at
    /// [Signing JSON](/appendices/#signing-json). Optional for the master key.
    /// Other keys must be signed by the user\'s master key.
    QJsonObject signatures{};
};

template <>
struct JsonObjectConverter<CrossSigningKey> {
    static void dumpTo(QJsonObject& jo, const CrossSigningKey& pod)
    {
        addParam<>(jo, QStringLiteral("user_id"), pod.userId);
        addParam<>(jo, QStringLiteral("usage"), pod.usage);
        addParam<>(jo, QStringLiteral("keys"), pod.keys);
        addParam<IfNotEmpty>(jo, QStringLiteral("signatures"), pod.signatures);
    }
    static void fillFrom(const QJsonObject& jo, CrossSigningKey& pod)
    {
        fillFromJson(jo.value("user_id"_ls), pod.userId);
        fillFromJson(jo.value("usage"_ls), pod.usage);
        fillFromJson(jo.value("keys"_ls), pod.keys);
        fillFromJson(jo.value("signatures"_ls), pod.signatures);
    }
};

} // namespace Quotient
