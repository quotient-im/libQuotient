// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/csapi/definitions/sync_filter.h>

#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Upload a new filter.
//!
//! Uploads a new filter definition to the homeserver.
//! Returns a filter ID that may be used in future requests to
//! restrict which events are returned to the client.
class QUOTIENT_API DefineFilterJob : public BaseJob {
public:
    //! \param userId
    //!   The id of the user uploading the filter. The access token must be authorized to make
    //!   requests for this user id.
    //!
    //! \param filter
    //!   The filter to upload.
    explicit DefineFilterJob(const QString& userId, const Filter& filter);

    // Result properties

    //! The ID of the filter that was created. Cannot start
    //! with a `{` as this character is used to determine
    //! if the filter provided is inline JSON or a previously
    //! declared filter by homeservers on some APIs.
    QString filterId() const { return loadFromJson<QString>("filter_id"_ls); }
};

inline auto collectResponse(const DefineFilterJob* job) { return job->filterId(); }

//! \brief Download a filter
class QUOTIENT_API GetFilterJob : public BaseJob {
public:
    //! \param userId
    //!   The user ID to download a filter for.
    //!
    //! \param filterId
    //!   The filter ID to download.
    explicit GetFilterJob(const QString& userId, const QString& filterId);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetFilterJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QString& userId,
                               const QString& filterId);

    // Result properties

    //! The filter definition.
    Filter filter() const { return fromJson<Filter>(jsonData()); }
};

inline auto collectResponse(const GetFilterJob* job) { return job->filter(); }

} // namespace Quotient
