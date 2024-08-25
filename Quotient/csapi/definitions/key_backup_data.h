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
        addParam<>(jo, "first_message_index"_L1, pod.firstMessageIndex);
        addParam<>(jo, "forwarded_count"_L1, pod.forwardedCount);
        addParam<>(jo, "is_verified"_L1, pod.isVerified);
        addParam<>(jo, "session_data"_L1, pod.sessionData);
    }
    static void fillFrom(const QJsonObject& jo, KeyBackupData& pod)
    {
        fillFromJson(jo.value("first_message_index"_L1), pod.firstMessageIndex);
        fillFromJson(jo.value("forwarded_count"_L1), pod.forwardedCount);
        fillFromJson(jo.value("is_verified"_L1), pod.isVerified);
        fillFromJson(jo.value("session_data"_L1), pod.sessionData);
    }
};

} // namespace Quotient
