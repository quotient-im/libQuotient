// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Gets the versions of the specification supported by the server.
//!
//! Gets the versions of the specification supported by the server.
//!
//! Values will take the form `vX.Y` or `rX.Y.Z` in historical cases. See
//! [the Specification Versioning](../#specification-versions) for more
//! information.
//!
//! The server may additionally advertise experimental features it supports
//! through `unstable_features`. These features should be namespaced and
//! may optionally include version information within their name if desired.
//! Features listed here are not for optionally toggling parts of the Matrix
//! specification and should only be used to advertise support for a feature
//! which has not yet landed in the spec. For example, a feature currently
//! undergoing the proposal process may appear here and eventually be taken
//! off this list once the feature lands in the spec and the server deems it
//! reasonable to do so. Servers may wish to keep advertising features here
//! after they've been released into the spec to give clients a chance to
//! upgrade appropriately. Additionally, clients should avoid using unstable
//! features in their stable releases.
class QUOTIENT_API GetVersionsJob : public BaseJob {
public:
    explicit GetVersionsJob();

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetVersionsJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl);

    // Result properties

    //! The supported versions.
    QStringList versions() const { return loadFromJson<QStringList>("versions"_ls); }

    //! Experimental features the server supports. Features not listed here,
    //! or the lack of this property all together, indicate that a feature is
    //! not supported.
    QHash<QString, bool> unstableFeatures() const
    {
        return loadFromJson<QHash<QString, bool>>("unstable_features"_ls);
    }
};

} // namespace Quotient
