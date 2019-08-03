/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace QMatrixClient
{

// Operations

/// Upgrades a room to a new room version.
/*!
 * Upgrades the given room to a particular room version.
 */
class UpgradeRoomJob : public BaseJob
{
public:
    /*! Upgrades a room to a new room version.
     * \param roomId
     *   The ID of the room to upgrade.
     * \param newVersion
     *   The new version for the room.
     */
    explicit UpgradeRoomJob(const QString& roomId, const QString& newVersion);

    ~UpgradeRoomJob() override;

    // Result properties

    /// The ID of the new room.
    const QString& replacementRoom() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace QMatrixClient
