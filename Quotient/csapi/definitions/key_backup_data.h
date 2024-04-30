// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {
//! The key data
struct QUOTIENT_API KeyBackupData {
    //! The index of the first message in the session that the key can decrypt.
    int firstMessageIndex;

    //! The number of times this key has been forwarded via key-sharing between devices.
    int forwardedCount;

    //! Whether the device backing up the key verified the device that the key
    //! is from.
    bool isVerified;

    //! Algorithm-dependent data.  See the documentation for the backup
    //! algorithms in [Server-side key backups](/client-server-api/#server-side-key-backups) for
    //! more information on the expected format of the data.
    QJsonObject sessionData;
};

template <>
struct JsonObjectConverter<KeyBackupData> {
    static void dumpTo(QJsonObject& jo, const KeyBackupData& pod)
    {
        addParam<>(jo, QStringLiteral("first_message_index"), pod.firstMessageIndex);
        addParam<>(jo, QStringLiteral("forwarded_count"), pod.forwardedCount);
        addParam<>(jo, QStringLiteral("is_verified"), pod.isVerified);
        addParam<>(jo, QStringLiteral("session_data"), pod.sessionData);
    }
    static void fillFrom(const QJsonObject& jo, KeyBackupData& pod)
    {
        fillFromJson(jo.value("first_message_index"_ls), pod.firstMessageIndex);
        fillFromJson(jo.value("forwarded_count"_ls), pod.forwardedCount);
        fillFromJson(jo.value("is_verified"_ls), pod.isVerified);
        fillFromJson(jo.value("session_data"_ls), pod.sessionData);
    }
};

} // namespace Quotient
