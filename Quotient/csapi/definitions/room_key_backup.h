// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/csapi/definitions/key_backup_data.h>

#include <Quotient/converters.h>

namespace Quotient {
//! The backed up keys for a room.
struct RoomKeyBackup {
    //! A map of session IDs to key data.
    QHash<QString, KeyBackupData> sessions;
};

template <>
struct JsonObjectConverter<RoomKeyBackup> {
    static void dumpTo(QJsonObject& jo, const RoomKeyBackup& pod)
    {
        addParam<>(jo, QStringLiteral("sessions"), pod.sessions);
    }
    static void fillFrom(const QJsonObject& jo, RoomKeyBackup& pod)
    {
        fillFromJson(jo.value("sessions"_ls), pod.sessions);
    }
};

} // namespace Quotient
