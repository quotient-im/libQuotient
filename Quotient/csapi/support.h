// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Gets homeserver contacts and support details.
//!
//! Gets server admin contact and support page of the domain.
//!
//! Like the [well-known discovery URI](/client-server-api/#well-known-uri),
//! this should be accessed with the hostname of the homeserver by making a
//! GET request to `https://hostname/.well-known/matrix/support`.
//!
//! Note that this endpoint is not necessarily handled by the homeserver.
//! It may be served by another webserver, used for discovering support
//! information for the homeserver.
class QUOTIENT_API GetWellknownSupportJob : public BaseJob {
public:
    // Inner data structures

    //! A way to contact the server administrator.
    struct QUOTIENT_API Contact {
        //! An informal description of what the contact methods
        //! are used for.
        //!
        //! `m.role.admin` is a catch-all role for any queries
        //! and `m.role.security` is intended for sensitive
        //! requests.
        //!
        //! Unspecified roles are permitted through the use of
        //! [Namespaced Identifiers](/appendices/#common-namespaced-identifier-grammar).
        QString role;

        //! A [Matrix User ID](/appendices/#user-identifiers)
        //! representing the administrator.
        //!
        //! It could be an account registered on a different
        //! homeserver so the administrator can be contacted
        //! when the homeserver is down.
        //!
        //! At least one of `matrix_id` or `email_address` is
        //! required.
        QString matrixId{};

        //! An email address to reach the administrator.
        //!
        //! At least one of `matrix_id` or `email_address` is
        //! required.
        QString emailAddress{};
    };

    // Construction/destruction

    explicit GetWellknownSupportJob();

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetWellknownSupportJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData);

    // Result properties

    //! Ways to contact the server administrator.
    //!
    //! At least one of `contacts` or `support_page` is required.
    //! If only `contacts` is set, it must contain at least one
    //! item.
    QVector<Contact> contacts() const { return loadFromJson<QVector<Contact>>("contacts"_L1); }

    //! The URL of a page to give users help specific to the
    //! homeserver, like extra login/registration steps.
    //!
    //! At least one of `contacts` or `support_page` is required.
    QString supportPage() const { return loadFromJson<QString>("support_page"_L1); }

    struct Response {
        //! Ways to contact the server administrator.
        //!
        //! At least one of `contacts` or `support_page` is required.
        //! If only `contacts` is set, it must contain at least one
        //! item.
        QVector<Contact> contacts{};

        //! The URL of a page to give users help specific to the
        //! homeserver, like extra login/registration steps.
        //!
        //! At least one of `contacts` or `support_page` is required.
        QString supportPage{};
    };
};

template <std::derived_from<GetWellknownSupportJob> JobT>
constexpr inline auto doCollectResponse<JobT> =
    [](JobT* j) -> GetWellknownSupportJob::Response { return { j->contacts(), j->supportPage() }; };

template <>
struct QUOTIENT_API JsonObjectConverter<GetWellknownSupportJob::Contact> {
    static void fillFrom(const QJsonObject& jo, GetWellknownSupportJob::Contact& result)
    {
        fillFromJson(jo.value("role"_L1), result.role);
        fillFromJson(jo.value("matrix_id"_L1), result.matrixId);
        fillFromJson(jo.value("email_address"_L1), result.emailAddress);
    }
};

} // namespace Quotient
