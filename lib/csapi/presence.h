/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "events/eventloader.h"
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
            bool currentlyActive() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Add or remove users from this presence list.
    /// 
    /// Adds or removes users from this presence list.
    class ModifyPresenceListJob : public BaseJob
    {
        public:
            /*! Add or remove users from this presence list.
             * \param userId 
             *   The user whose presence list is being modified.
             * \param invite 
             *   A list of user IDs to add to the list.
             * \param drop 
             *   A list of user IDs to remove from the list.
             */
            explicit ModifyPresenceListJob(const QString& userId, const QStringList& invite = {}, const QStringList& drop = {});
    };

    /// Get presence events for this presence list.
    /// 
    /// Retrieve a list of presence events for every user on this list.
    class GetPresenceForListJob : public BaseJob
    {
        public:
            /*! Get presence events for this presence list.
             * \param userId 
             *   The user whose presence list should be retrieved.
             */
            explicit GetPresenceForListJob(const QString& userId);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetPresenceForListJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId);

            ~GetPresenceForListJob() override;

            // Result properties

            /// A list of presence events for this list.
            Events&& data();

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
