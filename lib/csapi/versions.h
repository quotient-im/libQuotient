/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include "jobs/basejob.h"

#include <QtCore/QHash>

namespace Quotient
{

// Operations

/// Gets the versions of the specification supported by the server.
/*!
 * Gets the versions of the specification supported by the server.
 *
 * Values will take the form ``rX.Y.Z``.
 *
 * Only the latest ``Z`` value will be reported for each supported ``X.Y``
 * value. i.e. if the server implements ``r0.0.0``, ``r0.0.1``, and ``r1.2.0``,
 * it will report ``r0.0.1`` and ``r1.2.0``.
 *
 * The server may additionally advertise experimental features it supports
 * through ``unstable_features``. These features should be namespaced and
 * may optionally include version information within their name if desired.
 * Features listed here are not for optionally toggling parts of the Matrix
 * specification and should only be used to advertise support for a feature
 * which has not yet landed in the spec. For example, a feature currently
 * undergoing the proposal process may appear here and eventually be taken
 * off this list once the feature lands in the spec and the server deems it
 * reasonable to do so. Servers may wish to keep advertising features here
 * after they've been released into the spec to give clients a chance to
 * upgrade appropriately. Additionally, clients should avoid using unstable
 * features in their stable releases.
 */
class GetVersionsJob : public BaseJob
{
public:
    explicit GetVersionsJob();

    /*! Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for
     * GetVersionsJob is necessary but the job
     * itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl);

    ~GetVersionsJob() override;

    // Result properties

    /// The supported versions.
    const QStringList& versions() const;
    /// Experimental features the server supports. Features not listed here,
    /// or the lack of this property all together, indicate that a feature is
    /// not supported.
    const QHash<QString, bool>& unstableFeatures() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace Quotient
