/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "converters.h"

namespace QMatrixClient
{
    // Operations

    /// Update this user's presence state.
    ///
    /// This API sets the given user's presence state. When setting the status,
    /// the activity time is updated to reflect that activity; the client does
    /// not need to specify the ``last_active_ago`` field. You cannot set the
    /// presence state of another user.
    class SetPresenceJob : public BaseJob
    {
        public:
            /*! Update this user's presence state.
             * \param userId
             *   The user whose presence state to update.
             * \param presence
             *   The new presence state.
             * \param statusMsg
             *   The status message to attach to this state.
             */
            explicit SetPresenceJob(const QString& userId, const QString& presence, const QString& statusMsg = {});
    };

    /// Get this user's presence state.
    ///
    /// Get the given user's presence state.
    class GetPresenceJob : public BaseJob
    {
        public:
            /*! Get this user's presence state.
             * \param userId
             *   The user whose presence state to get.
             */
            explicit GetPresenceJob(const QString& userId);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetPresenceJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId);

            ~GetPresenceJob() override;

            // Result properties

            /// This user's presence.
            const QString& presence() const;
            /// The length of time in milliseconds since an action was performed
            /// by this user.
            Omittable<int> lastActiveAgo() const;
            /// The state message for this user if one was set.
            const QString& statusMsg() const;
            /// Whether the user is currently active
            Omittable<bool> currentlyActive() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
