/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../jobs/basejob.h"

namespace Quotient {

/*! \brief Upgrades a room to a new room version.
 *
 * Upgrades the given room to a particular room version.
 */
class QUOTIENT_API UpgradeRoomJob : public BaseJob {
public:
    /*! \brief Upgrades a room to a new room version.
     *
     * \param roomId
     *   The ID of the room to upgrade.
     *
     * \param newVersion
     *   The new version for the room.
     */
    explicit UpgradeRoomJob(const QString& roomId, const QString& newVersion);

    // Result properties

    /// The ID of the new room.
    QString replacementRoom() const
    {
        return loadFromJson<QString>("replacement_room"_ls);
    }
};

} // namespace Quotient
