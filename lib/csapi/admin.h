/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace Quotient {

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

    // Result properties

    /// The Matrix user ID of the user.
    QString userId() const { return loadFromJson<QString>("user_id"_ls); }

    /// Each key is an identifier for one of the user's devices.
    QHash<QString, DeviceInfo> devices() const
    {
        return loadFromJson<QHash<QString, DeviceInfo>>("devices"_ls);
    }
};

template <>
struct JsonObjectConverter<GetWhoIsJob::ConnectionInfo> {
    static void fillFrom(const QJsonObject& jo,
                         GetWhoIsJob::ConnectionInfo& result)
    {
        fromJson(jo.value("ip"_ls), result.ip);
        fromJson(jo.value("last_seen"_ls), result.lastSeen);
        fromJson(jo.value("user_agent"_ls), result.userAgent);
    }
};

template <>
struct JsonObjectConverter<GetWhoIsJob::SessionInfo> {
    static void fillFrom(const QJsonObject& jo, GetWhoIsJob::SessionInfo& result)
    {
        fromJson(jo.value("connections"_ls), result.connections);
    }
};

template <>
struct JsonObjectConverter<GetWhoIsJob::DeviceInfo> {
    static void fillFrom(const QJsonObject& jo, GetWhoIsJob::DeviceInfo& result)
    {
        fromJson(jo.value("sessions"_ls), result.sessions);
    }
};

} // namespace Quotient
