// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quotient_export.h"
#include "events/roomevent.h"
#include "events/stateevent.h"

namespace Quotient {
class QUOTIENT_API SlidingSyncRoom {
public:
    QString id;
    int highlightCount;
    bool initial;
    int joinedCount;
    QString name;
    int notificationCount;
    QString prevBatch;
    RoomEvents timeline;
    StateEvents requiredState;
    explicit SlidingSyncRoom(const QJsonObject &data, const QString &id);
};

class QUOTIENT_API SlidingSyncOps {
public:
    QString op;
    std::pair<int, int> range;
    std::vector<QString> roomIds;
    static SlidingSyncOps fromJson(const QJsonObject &data);
};

class QUOTIENT_API SlidingSyncLists {
public:
    int count;
    QList<SlidingSyncOps> ops;
    static SlidingSyncLists fromJson(const QJsonObject &data);
};

using SlidingRoomsData = std::vector<SlidingSyncRoom>;

class QUOTIENT_API SlidingSyncData {
public:
    SlidingSyncData() = default;
    void parseJson(const QJsonObject& json);
    QJsonObject extensions;
    QMap<QString, SlidingSyncLists> lists;
    QString pos;
    SlidingRoomsData rooms;
    SlidingRoomsData takeRoomData();
};

} // namespace Quotient
