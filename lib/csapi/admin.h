/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include "jobs/basejob.h"

#include <QtCore/QHash>
#include <QtCore/QVector>

namespace Quotient {

// Operations

/*! \brief Gets information about a particular user.
 *
 * Gets information about a particular user.
 *
 * This API may be restricted to only be called by the user being looked
 * up, or by a server admin. Server-local administrator privileges are not
 * specified in this document.
 */
class GetWhoIsJob : public BaseJob {
public:
    // Inner data structures

    /// Gets information about a particular user.
    ///
    /// This API may be restricted to only be called by the user being looked
    /// up, or by a server admin. Server-local administrator privileges are not
    /// specified in this document.
    struct ConnectionInfo {
        /// Most recently seen IP address of the session.
        QString ip;
        /// Unix timestamp that the session was last active.
        Omittable<qint64> lastSeen;
        /// User agent string last seen in the session.
        QString userAgent;
    };

    /// Gets information about a particular user.
    ///
    /// This API may be restricted to only be called by the user being looked
    /// up, or by a server admin. Server-local administrator privileges are not
    /// specified in this document.
    struct SessionInfo {
        /// Information particular connections in the session.
        QVector<ConnectionInfo> connections;
    };

    /// Gets information about a particular user.
    ///
    /// This API may be restricted to only be called by the user being looked
    /// up, or by a server admin. Server-local administrator privileges are not
    /// specified in this document.
    struct DeviceInfo {
        /// A user's sessions (i.e. what they did with an access token from one
        /// login).
        QVector<SessionInfo> sessions;
    };

    // Construction/destruction

    /*! \brief Gets information about a particular user.
     *
     * \param userId
     *   The user to look up.
     */
    explicit GetWhoIsJob(const QString& userId);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetWhoIsJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId);
    ~GetWhoIsJob() override;

    // Result properties

    /// The Matrix user ID of the user.
    const QString& userId() const;

    /// Each key is an identitfier for one of the user's devices.
    const QHash<QString, DeviceInfo>& devices() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace Quotient
