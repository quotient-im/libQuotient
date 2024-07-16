// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/csapi/definitions/tag.h>

#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief List the tags for a room.
//!
//! List the tags set by a user on a room.
class QUOTIENT_API GetRoomTagsJob : public BaseJob {
public:
    //! \param userId
    //!   The id of the user to get tags for. The access token must be
    //!   authorized to make requests for this user ID.
    //!
    //! \param roomId
    //!   The ID of the room to get tags for.
    explicit GetRoomTagsJob(const QString& userId, const QString& roomId);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetRoomTagsJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId, const QString& roomId);

    // Result properties

    QHash<QString, Tag> tags() const { return loadFromJson<QHash<QString, Tag>>("tags"_ls); }
};

inline auto collectResponse(const GetRoomTagsJob* job) { return job->tags(); }

//! \brief Add a tag to a room.
//!
//! Add a tag to the room.
class QUOTIENT_API SetRoomTagJob : public BaseJob {
public:
    //! \param userId
    //!   The id of the user to add a tag for. The access token must be
    //!   authorized to make requests for this user ID.
    //!
    //! \param roomId
    //!   The ID of the room to add a tag to.
    //!
    //! \param tag
    //!   The tag to add.
    //!
    //! \param data
    //!   Extra data for the tag, e.g. ordering.
    explicit SetRoomTagJob(const QString& userId, const QString& roomId, const QString& tag,
                           const Tag& data);
};

//! \brief Remove a tag from the room.
//!
//! Remove a tag from the room.
class QUOTIENT_API DeleteRoomTagJob : public BaseJob {
public:
    //! \param userId
    //!   The id of the user to remove a tag for. The access token must be
    //!   authorized to make requests for this user ID.
    //!
    //! \param roomId
    //!   The ID of the room to remove a tag from.
    //!
    //! \param tag
    //!   The tag to remove.
    explicit DeleteRoomTagJob(const QString& userId, const QString& roomId, const QString& tag);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for DeleteRoomTagJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId, const QString& roomId,
                               const QString& tag);
};

} // namespace Quotient
