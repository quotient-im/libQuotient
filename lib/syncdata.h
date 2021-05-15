// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "quotient_common.h"

#include "events/stateevent.h"

namespace Quotient {
/// Room summary, as defined in MSC688
/**
 * Every member of this structure is an Omittable; as per the MSC, only
 * changed values are sent from the server so if nothing is in the payload
 * the respective member will be omitted. In particular, `heroes.omitted()`
 * means that nothing has come from the server; heroes.value().isEmpty()
 * means a peculiar case of a room with the only member - the current user.
 */
struct RoomSummary {
    Omittable<int> joinedMemberCount;
    Omittable<int> invitedMemberCount;
    Omittable<QStringList> heroes; //< mxids of users to take part in the room
                                   // name

    bool isEmpty() const;
    /// Merge the contents of another RoomSummary object into this one
    /// \return true, if the current object has changed; false otherwise
    bool merge(const RoomSummary& other);
};
QDebug operator<<(QDebug dbg, const RoomSummary& rs);


template <>
struct JsonObjectConverter<RoomSummary> {
    static void dumpTo(QJsonObject& jo, const RoomSummary& rs);
    static void fillFrom(const QJsonObject& jo, RoomSummary& rs);
};

/// Information on e2e device updates. Note: only present on an
/// incremental sync.
struct DevicesList {
    /// List of users who have updated their device identity keys, or who
    /// now share an encrypted room with the client since the previous
    /// sync response.
    QStringList changed;

    /// List of users with whom we do not share any encrypted rooms
    /// anymore since the previous sync response.
    QStringList left;
};

QDebug operator<<(QDebug dhg, const DevicesList &devicesList);

template <>
struct JsonObjectConverter<DevicesList> {
    static void dumpTo(QJsonObject &jo, const DevicesList &dev);
    static void fillFrom(const QJsonObject& jo, DevicesList& rs);
};

class SyncRoomData {
public:
    QString roomId;
    JoinState joinState;
    RoomSummary summary;
    StateEvents state;
    RoomEvents timeline;
    Events ephemeral;
    Events accountData;

    bool timelineLimited;
    QString timelinePrevBatch;
    Omittable<int> unreadCount;
    Omittable<int> highlightCount;
    Omittable<int> notificationCount;

    SyncRoomData(const QString& roomId, JoinState joinState_,
                 const QJsonObject& room_);
    SyncRoomData(SyncRoomData&&) = default;
    SyncRoomData& operator=(SyncRoomData&&) = default;

    static const QString UnreadCountKey;
};

// QVector cannot work with non-copyable objects, std::vector can.
using SyncDataList = std::vector<SyncRoomData>;

class SyncData {
public:
    SyncData() = default;
    explicit SyncData(const QString& cacheFileName);
    /** Parse sync response into room events
     * \param json response from /sync or a room state cache
     * \return the list of rooms with missing cache files; always
     *         empty when parsing response from /sync
     */
    void parseJson(const QJsonObject& json, const QString& baseDir = {});

    Events&& takePresenceData();
    Events&& takeAccountData();
    Events&& takeToDeviceEvents();
    const QHash<QString, int>& deviceOneTimeKeysCount() const
    {
        return deviceOneTimeKeysCount_;
    }
    SyncDataList&& takeRoomData();
    DevicesList&& takeDevicesList();

    QString nextBatch() const { return nextBatch_; }

    QStringList unresolvedRooms() const { return unresolvedRoomIds; }

    static std::pair<int, int> cacheVersion() { return { 11, 0 }; }
    static QString fileNameForRoom(QString roomId);

private:
    QString nextBatch_;
    Events presenceData;
    Events accountData;
    Events toDeviceEvents;
    SyncDataList roomData;
    QStringList unresolvedRoomIds;
    QHash<QString, int> deviceOneTimeKeysCount_;
    DevicesList devicesList;

    static QJsonObject loadJson(const QString& fileName);
};
} // namespace Quotient
