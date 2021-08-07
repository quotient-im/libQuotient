/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Update this user's presence state.
 *
 * This API sets the given user's presence state. When setting the status,
 * the activity time is updated to reflect that activity; the client does
 * not need to specify the `last_active_ago` field. You cannot set the
 * presence state of another user.
 */
class SetPresenceJob : public BaseJob {
public:
    /*! \brief Update this user's presence state.
     *
     * \param userId
     *   The user whose presence state to update.
     *
     * \param presence
     *   The new presence state.
     *
     * \param statusMsg
     *   The status message to attach to this state.
     */
    explicit SetPresenceJob(const QString& userId, const QString& presence,
                            const QString& statusMsg = {});
};

/*! \brief Get this user's presence state.
 *
 * Get the given user's presence state.
 */
class GetPresenceJob : public BaseJob {
public:
    /*! \brief Get this user's presence state.
     *
     * \param userId
     *   The user whose presence state to get.
     */
    explicit GetPresenceJob(const QString& userId);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetPresenceJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId);

    // Result properties

    /// This user's presence.
    QString presence() const { return loadFromJson<QString>("presence"_ls); }

    /// The length of time in milliseconds since an action was performed
    /// by this user.
    Omittable<int> lastActiveAgo() const
    {
        return loadFromJson<Omittable<int>>("last_active_ago"_ls);
    }

    /// The state message for this user if one was set.
    QString statusMsg() const { return loadFromJson<QString>("status_msg"_ls); }

    /// Whether the user is currently active
    Omittable<bool> currentlyActive() const
    {
        return loadFromJson<Omittable<bool>>("currently_active"_ls);
    }
};

} // namespace Quotient
