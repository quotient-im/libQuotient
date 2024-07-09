// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/jobs/basejob.h>

#include <Quotient/application-service/definitions/location.h>
#include <Quotient/application-service/definitions/protocol.h>
#include <Quotient/application-service/definitions/user.h>

namespace Quotient {

//! \brief Retrieve metadata about all protocols that a homeserver supports.
//!
//! Fetches the overall metadata about protocols supported by the
//! homeserver. Includes both the available protocols and all fields
//! required for queries against each protocol.
class QUOTIENT_API GetProtocolsJob : public BaseJob {
public:
    explicit GetProtocolsJob();

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetProtocolsJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData);

    // Result properties

    //! The protocols supported by the homeserver.
    QHash<QString, ThirdPartyProtocol> protocols() const
    {
        return fromJson<QHash<QString, ThirdPartyProtocol>>(jsonData());
    }
};

inline auto collectResponse(const GetProtocolsJob* job) { return job->protocols(); }

//! \brief Retrieve metadata about a specific protocol that the homeserver supports.
//!
//! Fetches the metadata from the homeserver about a particular third-party protocol.
class QUOTIENT_API GetProtocolMetadataJob : public BaseJob {
public:
    //! \param protocol
    //!   The name of the protocol.
    explicit GetProtocolMetadataJob(const QString& protocol);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetProtocolMetadataJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QString& protocol);

    // Result properties

    //! The protocol was found and metadata returned.
    ThirdPartyProtocol data() const { return fromJson<ThirdPartyProtocol>(jsonData()); }
};

inline auto collectResponse(const GetProtocolMetadataJob* job) { return job->data(); }

//! \brief Retrieve Matrix-side portals rooms leading to a third-party location.
//!
//! Requesting this endpoint with a valid protocol name results in a list
//! of successful mapping results in a JSON array. Each result contains
//! objects to represent the Matrix room or rooms that represent a portal
//! to this third-party network. Each has the Matrix room alias string,
//! an identifier for the particular third-party network protocol, and an
//! object containing the network-specific fields that comprise this
//! identifier. It should attempt to canonicalise the identifier as much
//! as reasonably possible given the network type.
class QUOTIENT_API QueryLocationByProtocolJob : public BaseJob {
public:
    //! \param protocol
    //!   The protocol used to communicate to the third-party network.
    //!
    //! \param searchFields
    //!   One or more custom fields to help identify the third-party
    //!   location.
    explicit QueryLocationByProtocolJob(const QString& protocol, const QString& searchFields = {});

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for QueryLocationByProtocolJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QString& protocol,
                               const QString& searchFields = {});

    // Result properties

    //! At least one portal room was found.
    QVector<ThirdPartyLocation> data() const
    {
        return fromJson<QVector<ThirdPartyLocation>>(jsonData());
    }
};

inline auto collectResponse(const QueryLocationByProtocolJob* job) { return job->data(); }

//! \brief Retrieve the Matrix User ID of a corresponding third-party user.
//!
//! Retrieve a Matrix User ID linked to a user on the third-party service, given
//! a set of user parameters.
class QUOTIENT_API QueryUserByProtocolJob : public BaseJob {
public:
    //! \param protocol
    //!   The name of the protocol.
    //!
    //! \param fields
    //!   One or more custom fields that are passed to the AS to help identify the user.
    explicit QueryUserByProtocolJob(const QString& protocol,
                                    const QHash<QString, QString>& fields = {});

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for QueryUserByProtocolJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QString& protocol,
                               const QHash<QString, QString>& fields = {});

    // Result properties

    //! The Matrix User IDs found with the given parameters.
    QVector<ThirdPartyUser> data() const { return fromJson<QVector<ThirdPartyUser>>(jsonData()); }
};

inline auto collectResponse(const QueryUserByProtocolJob* job) { return job->data(); }

//! \brief Reverse-lookup third-party locations given a Matrix room alias.
//!
//! Retrieve an array of third-party network locations from a Matrix room
//! alias.
class QUOTIENT_API QueryLocationByAliasJob : public BaseJob {
public:
    //! \param alias
    //!   The Matrix room alias to look up.
    explicit QueryLocationByAliasJob(const QString& alias);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for QueryLocationByAliasJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QString& alias);

    // Result properties

    //! All found third-party locations.
    QVector<ThirdPartyLocation> data() const
    {
        return fromJson<QVector<ThirdPartyLocation>>(jsonData());
    }
};

inline auto collectResponse(const QueryLocationByAliasJob* job) { return job->data(); }

//! \brief Reverse-lookup third-party users given a Matrix User ID.
//!
//! Retrieve an array of third-party users from a Matrix User ID.
class QUOTIENT_API QueryUserByIDJob : public BaseJob {
public:
    //! \param userid
    //!   The Matrix User ID to look up.
    explicit QueryUserByIDJob(const QString& userid);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for QueryUserByIDJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QString& userid);

    // Result properties

    //! An array of third-party users.
    QVector<ThirdPartyUser> data() const { return fromJson<QVector<ThirdPartyUser>>(jsonData()); }
};

inline auto collectResponse(const QueryUserByIDJob* job) { return job->data(); }

} // namespace Quotient
