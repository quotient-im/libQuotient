// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {
//! Device identity keys
struct QUOTIENT_API DeviceKeys {
    //! The ID of the user the device belongs to. Must match the user ID used
    //! when logging in.
    QString userId;

    //! The ID of the device these keys belong to. Must match the device ID used
    //! when logging in.
    QString deviceId;

    //! The encryption algorithms supported by this device.
    QStringList algorithms;

    //! Public identity keys. The names of the properties should be in the
    //! format `<algorithm>:<device_id>`. The keys themselves should be
    //! encoded as specified by the key algorithm.
    QHash<QString, QString> keys;

    //! Signatures for the device key object. A map from user ID, to a map from
    //! `<algorithm>:<device_id>` to the signature.
    //!
    //! The signature is calculated using the process described at [Signing
    //! JSON](/appendices/#signing-json).
    QHash<QString, QHash<QString, QString>> signatures;
};

template <>
struct JsonObjectConverter<DeviceKeys> {
    static void dumpTo(QJsonObject& jo, const DeviceKeys& pod)
    {
        addParam<>(jo, QStringLiteral("user_id"), pod.userId);
        addParam<>(jo, QStringLiteral("device_id"), pod.deviceId);
        addParam<>(jo, QStringLiteral("algorithms"), pod.algorithms);
        addParam<>(jo, QStringLiteral("keys"), pod.keys);
        addParam<>(jo, QStringLiteral("signatures"), pod.signatures);
    }
    static void fillFrom(const QJsonObject& jo, DeviceKeys& pod)
    {
        fillFromJson(jo.value("user_id"_ls), pod.userId);
        fillFromJson(jo.value("device_id"_ls), pod.deviceId);
        fillFromJson(jo.value("algorithms"_ls), pod.algorithms);
        fillFromJson(jo.value("keys"_ls), pod.keys);
        fillFromJson(jo.value("signatures"_ls), pod.signatures);
    }
};

} // namespace Quotient
