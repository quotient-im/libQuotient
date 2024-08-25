// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/csapi/definitions/key_backup_data.h>

#include <Quotient/converters.h>

namespace Quotient {
//! The backed up keys for a room.
struct QUOTIENT_API RoomKeyBackup {
    //! A map of session IDs to key data.
    QHash<QString, KeyBackupData> sessions;
};

template <>
struct JsonObjectConverter<RoomKeyBackup> {
    static void dumpTo(QJsonObject& jo, const RoomKeyBackup& pod)
    {
        addParam<>(jo, "sessions"_L1, pod.sessions);
    }
    static void fillFrom(const QJsonObject& jo, RoomKeyBackup& pod)
    {
        fillFromJson(jo.value("sessions"_L1), pod.sessions);
    }
};

} // namespace Quotient
