/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include "csapi/definitions/device_keys.h"

#include "jobs/basejob.h"

#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QVariant>

namespace Quotient
{

// Operations

/// Upload end-to-end encryption keys.
/*!
 * Publishes end-to-end encryption keys for the device.
 */
class UploadKeysJob : public BaseJob
{
public:
    /*! Upload end-to-end encryption keys.
     * \param deviceKeys
     *   Identity keys for the device. May be absent if no new
     *   identity keys are required.
     * \param oneTimeKeys
     *   One-time public keys for "pre-key" messages.  The names of
     *   the properties should be in the format
     *   ``<algorithm>:<key_id>``. The format of the key is determined
     *   by the key algorithm.
     *
     *   May be absent if no new one-time keys are required.
     */
    explicit UploadKeysJob(const Omittable<DeviceKeys>& deviceKeys = none,
                           const QHash<QString, QVariant>& oneTimeKeys = {});

    ~UploadKeysJob() override;

    // Result properties

    /// For each key algorithm, the number of unclaimed one-time keys
    /// of that type currently held on the server for this device.
    const QHash<QString, int>& oneTimeKeyCounts() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

/// Download device identity keys.
/*!
 * Returns the current devices and identity keys for the given users.
 */
class QueryKeysJob : public BaseJob
{
public:
    // Inner data structures

    /// Additional data added to the device key informationby intermediate
    /// servers, and not covered by thesignatures.
    struct UnsignedDeviceInfo
    {
        /// The display name which the user set on the device.
        QString deviceDisplayName;
    };

    /// Returns the current devices and identity keys for the given users.
    struct DeviceInformation : DeviceKeys
    {
        /// Additional data added to the device key informationby intermediate
        /// servers, and not covered by thesignatures.
        Omittable<UnsignedDeviceInfo> unsignedData;
    };

    // Construction/destruction

    /*! Download device identity keys.
     * \param deviceKeys
     *   The keys to be downloaded. A map from user ID, to a list of
     *   device IDs, or to an empty list to indicate all devices for the
     *   corresponding user.
     * \param timeout
     *   The time (in milliseconds) to wait when downloading keys from
     *   remote servers. 10 seconds is the recommended default.
     * \param token
     *   If the client is fetching keys as a result of a device update received
     *   in a sync request, this should be the 'since' token of that sync
     * request, or any later sync token. This allows the server to ensure its
     * response contains the keys advertised by the notification in that sync.
     */
    explicit QueryKeysJob(const QHash<QString, QStringList>& deviceKeys,
                          Omittable<int> timeout = none,
                          const QString& token = {});

    ~QueryKeysJob() override;

    // Result properties

    /// If any remote homeservers could not be reached, they are
    /// recorded here. The names of the properties are the names of
    /// the unreachable servers.
    ///
    /// If the homeserver could be reached, but the user or device
    /// was unknown, no failure is recorded. Instead, the corresponding
    /// user or device is missing from the ``device_keys`` result.
    const QHash<QString, QJsonObject>& failures() const;
    /// Information on the queried devices. A map from user ID, to a
    /// map from device ID to device information.  For each device,
    /// the information returned will be the same as uploaded via
    /// ``/keys/upload``, with the addition of an ``unsigned``
    /// property.
    const QHash<QString, QHash<QString, DeviceInformation>>& deviceKeys() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

/// Claim one-time encryption keys.
/*!
 * Claims one-time keys for use in pre-key messages.
 */
class ClaimKeysJob : public BaseJob
{
public:
    /*! Claim one-time encryption keys.
     * \param oneTimeKeys
     *   The keys to be claimed. A map from user ID, to a map from
     *   device ID to algorithm name.
     * \param timeout
     *   The time (in milliseconds) to wait when downloading keys from
     *   remote servers. 10 seconds is the recommended default.
     */
    explicit ClaimKeysJob(
        const QHash<QString, QHash<QString, QString>>& oneTimeKeys,
        Omittable<int> timeout = none);

    ~ClaimKeysJob() override;

    // Result properties

    /// If any remote homeservers could not be reached, they are
    /// recorded here. The names of the properties are the names of
    /// the unreachable servers.
    ///
    /// If the homeserver could be reached, but the user or device
    /// was unknown, no failure is recorded. Instead, the corresponding
    /// user or device is missing from the ``one_time_keys`` result.
    const QHash<QString, QJsonObject>& failures() const;
    /// One-time keys for the queried devices. A map from user ID, to a
    /// map from devices to a map from ``<algorithm>:<key_id>`` to the key object.
    const QHash<QString, QHash<QString, QVariant>>& oneTimeKeys() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

/// Query users with recent device key updates.
/*!
 * Gets a list of users who have updated their device identity keys since a
 * previous sync token.
 *
 * The server should include in the results any users who:
 *
 * * currently share a room with the calling user (ie, both users have
 *   membership state ``join``); *and*
 * * added new device identity keys or removed an existing device with
 *   identity keys, between ``from`` and ``to``.
 */
class GetKeysChangesJob : public BaseJob
{
public:
    /*! Query users with recent device key updates.
     * \param from
     *   The desired start point of the list. Should be the ``next_batch`` field
     *   from a response to an earlier call to |/sync|. Users who have not
     *   uploaded new device identity keys since this point, nor deleted
     *   existing devices with identity keys since then, will be excluded
     *   from the results.
     * \param to
     *   The desired end point of the list. Should be the ``next_batch``
     *   field from a recent call to |/sync| - typically the most recent
     *   such call. This may be used by the server as a hint to check its
     *   caches are up to date.
     */
    explicit GetKeysChangesJob(const QString& from, const QString& to);

    /*! Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for
     * GetKeysChangesJob is necessary but the job
     * itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& from,
                               const QString& to);

    ~GetKeysChangesJob() override;

    // Result properties

    /// The Matrix User IDs of all users who updated their device
    /// identity keys.
    const QStringList& changed() const;
    /// The Matrix User IDs of all users who may have left all
    /// the end-to-end encrypted rooms they previously shared
    /// with the user.
    const QStringList& left() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace Quotient
