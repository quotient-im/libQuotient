// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Update this user's presence state.
//!
//! This API sets the given user's presence state. When setting the status,
//! the activity time is updated to reflect that activity; the client does
//! not need to specify the `last_active_ago` field. You cannot set the
//! presence state of another user.
class QUOTIENT_API SetPresenceJob : public BaseJob {
public:
    //! \param userId
    //!   The user whose presence state to update.
    //!
    //! \param presence
    //!   The new presence state.
    //!
    //! \param statusMsg
    //!   The status message to attach to this state.
    explicit SetPresenceJob(const QString& userId, const QString& presence,
                            const QString& statusMsg = {});
};

//! \brief Get this user's presence state.
//!
//! Get the given user's presence state.
class QUOTIENT_API GetPresenceJob : public BaseJob {
public:
    //! \param userId
    //!   The user whose presence state to get.
    explicit GetPresenceJob(const QString& userId);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetPresenceJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId);

    // Result properties

    //! This user's presence.
    QString presence() const { return loadFromJson<QString>("presence"_ls); }

    //! The length of time in milliseconds since an action was performed
    //! by this user.
    std::optional<int> lastActiveAgo() const
    {
        return loadFromJson<std::optional<int>>("last_active_ago"_ls);
    }

    //! The state message for this user if one was set.
    QString statusMsg() const { return loadFromJson<QString>("status_msg"_ls); }

    //! Whether the user is currently active
    std::optional<bool> currentlyActive() const
    {
        return loadFromJson<std::optional<bool>>("currently_active"_ls);
    }

    struct Response {
        //! This user's presence.
        QString presence{};

        //! The length of time in milliseconds since an action was performed
        //! by this user.
        std::optional<int> lastActiveAgo{};

        //! The state message for this user if one was set.
        QString statusMsg{};

        //! Whether the user is currently active
        std::optional<bool> currentlyActive{};
    };
};

template <std::derived_from<GetPresenceJob> JobT>
constexpr inline auto doCollectResponse<JobT> = [](JobT* j) -> GetPresenceJob::Response {
    return { j->presence(), j->lastActiveAgo(), j->statusMsg(), j->currentlyActive() };
};

} // namespace Quotient
