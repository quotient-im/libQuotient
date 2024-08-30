// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {
//! Cross signing key
struct QUOTIENT_API CrossSigningKey {
    //! The ID of the user the key belongs to.
    QString userId;

    //! What the key is used for.
    QStringList usage;

    //! The public key.  The object must have exactly one property, whose name is
    //! in the form `<algorithm>:<unpadded_base64_public_key>`, and whose value
    //! is the unpadded base64 public key.
    QHash<QString, QString> keys;

    //! Signatures of the key, calculated using the process described at [Signing
    //! JSON](/appendices/#signing-json). Optional for the master key. Other keys must be signed by
    //! the user\'s master key.
    QJsonObject signatures{};
};

template <>
struct JsonObjectConverter<CrossSigningKey> {
    static void dumpTo(QJsonObject& jo, const CrossSigningKey& pod)
    {
        addParam<>(jo, "user_id"_L1, pod.userId);
        addParam<>(jo, "usage"_L1, pod.usage);
        addParam<>(jo, "keys"_L1, pod.keys);
        addParam<IfNotEmpty>(jo, "signatures"_L1, pod.signatures);
    }
    static void fillFrom(const QJsonObject& jo, CrossSigningKey& pod)
    {
        fillFromJson(jo.value("user_id"_L1), pod.userId);
        fillFromJson(jo.value("usage"_L1), pod.usage);
        fillFromJson(jo.value("keys"_L1), pod.keys);
        fillFromJson(jo.value("signatures"_L1), pod.signatures);
    }
};

} // namespace Quotient
