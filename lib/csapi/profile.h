/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace QMatrixClient {
    // Operations

    /// Set the user's display name.
    ///
    /// This API sets the given user's display name. You must have permission to
    /// set this user's display name, e.g. you need to have their
    /// ``access_token``.
    class SetDisplayNameJob : public BaseJob
    {
        public:
        /*! Set the user's display name.
         * \param userId
         *   The user whose display name to set.
         * \param displayname
         *   The new display name for this user.
         */
        explicit SetDisplayNameJob(const QString& userId,
                                   const QString& displayname = {});
    };

    /// Get the user's display name.
    ///
    /// Get the user's display name. This API may be used to fetch the user's
    /// own displayname or to query the name of other users; either locally or
    /// on remote homeservers.
    class GetDisplayNameJob : public BaseJob
    {
        public:
        /*! Get the user's display name.
         * \param userId
         *   The user whose display name to get.
         */
        explicit GetDisplayNameJob(const QString& userId);

        /*! Construct a URL without creating a full-fledged job object
         *
         * This function can be used when a URL for
         * GetDisplayNameJob is necessary but the job
         * itself isn't.
         */
        static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId);

        ~GetDisplayNameJob() override;

        // Result properties

        /// The user's display name if they have set one, otherwise not present.
        const QString& displayname() const;

        protected:
        Status parseJson(const QJsonDocument& data) override;

        private:
        class Private;
        QScopedPointer<Private> d;
    };

    /// Set the user's avatar URL.
    ///
    /// This API sets the given user's avatar URL. You must have permission to
    /// set this user's avatar URL, e.g. you need to have their
    /// ``access_token``.
    class SetAvatarUrlJob : public BaseJob
    {
        public:
        /*! Set the user's avatar URL.
         * \param userId
         *   The user whose avatar URL to set.
         * \param avatarUrl
         *   The new avatar URL for this user.
         */
        explicit SetAvatarUrlJob(const QString& userId,
                                 const QString& avatarUrl = {});
    };

    /// Get the user's avatar URL.
    ///
    /// Get the user's avatar URL. This API may be used to fetch the user's
    /// own avatar URL or to query the URL of other users; either locally or
    /// on remote homeservers.
    class GetAvatarUrlJob : public BaseJob
    {
        public:
        /*! Get the user's avatar URL.
         * \param userId
         *   The user whose avatar URL to get.
         */
        explicit GetAvatarUrlJob(const QString& userId);

        /*! Construct a URL without creating a full-fledged job object
         *
         * This function can be used when a URL for
         * GetAvatarUrlJob is necessary but the job
         * itself isn't.
         */
        static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId);

        ~GetAvatarUrlJob() override;

        // Result properties

        /// The user's avatar URL if they have set one, otherwise not present.
        const QString& avatarUrl() const;

        protected:
        Status parseJson(const QJsonDocument& data) override;

        private:
        class Private;
        QScopedPointer<Private> d;
    };

    /// Get this user's profile information.
    ///
    /// Get the combined profile information for this user. This API may be used
    /// to fetch the user's own profile information or other users; either
    /// locally or on remote homeservers. This API may return keys which are not
    /// limited to ``displayname`` or ``avatar_url``.
    class GetUserProfileJob : public BaseJob
    {
        public:
        /*! Get this user's profile information.
         * \param userId
         *   The user whose profile information to get.
         */
        explicit GetUserProfileJob(const QString& userId);

        /*! Construct a URL without creating a full-fledged job object
         *
         * This function can be used when a URL for
         * GetUserProfileJob is necessary but the job
         * itself isn't.
         */
        static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId);

        ~GetUserProfileJob() override;

        // Result properties

        /// The user's avatar URL if they have set one, otherwise not present.
        const QString& avatarUrl() const;
        /// The user's display name if they have set one, otherwise not present.
        const QString& displayname() const;

        protected:
        Status parseJson(const QJsonDocument& data) override;

        private:
        class Private;
        QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
