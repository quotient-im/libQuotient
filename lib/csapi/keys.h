/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "csapi/definitions/cross_signing_key.h"
#include "csapi/definitions/device_keys.h"

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Upload end-to-end encryption keys.
 *
 * Publishes end-to-end encryption keys for the device.
 */
class UploadKeysJob : public BaseJob {
public:
    /*! \brief Upload end-to-end encryption keys.
     *
     * \param deviceKeys
     *   Identity keys for the device. May be absent if no new
     *   identity keys are required.
     *
     * \param oneTimeKeys
     *   One-time public keys for "pre-key" messages.  The names of
     *   the properties should be in the format
     *   `<algorithm>:<key_id>`. The format of the key is determined
     *   by the [key algorithm](/client-server-api/#key-algorithms).
     *
     *   May be absent if no new one-time keys are required.
     */
    explicit UploadKeysJob(const Omittable<DeviceKeys>& deviceKeys = none,
                           const QHash<QString, QVariant>& oneTimeKeys = {});

    // Result properties

    /// For each key algorithm, the number of unclaimed one-time keys
    /// of that type currently held on the server for this device.
    QHash<QString, int> oneTimeKeyCounts() const
    {
        return loadFromJson<QHash<QString, int>>("one_time_key_counts"_ls);
    }
};

/*! \brief Download device identity keys.
 *
 * Returns the current devices and identity keys for the given users.
 */
class QueryKeysJob : public BaseJob {
public:
    // Inner data structures

    /// Additional data added to the device key information
    /// by intermediate servers, and not covered by the
    /// signatures.
    struct UnsignedDeviceInfo {
        /// The display name which the user set on the device.
        QString deviceDisplayName;
    };

    /// Returns the current devices and identity keys for the given users.
    struct DeviceInformation : DeviceKeys {
        /// Additional data added to the device key information
        /// by intermediate servers, and not covered by the
        /// signatures.
        Omittable<UnsignedDeviceInfo> unsignedData;
    };

    // Construction/destruction

    /*! \brief Download device identity keys.
     *
     * \param deviceKeys
     *   The keys to be downloaded. A map from user ID, to a list of
     *   device IDs, or to an empty list to indicate all devices for the
     *   corresponding user.
     *
     * \param timeout
     *   The time (in milliseconds) to wait when downloading keys from
     *   remote servers. 10 seconds is the recommended default.
     *
     * \param token
     *   If the client is fetching keys as a result of a device update received
     *   in a sync request, this should be the 'since' token of that sync
     * request, or any later sync token. This allows the server to ensure its
     * response contains the keys advertised by the notification in that sync.
     */
    explicit QueryKeysJob(const QHash<QString, QStringList>& deviceKeys,
                          Omittable<int> timeout = none,
                          const QString& token = {});

    // Result properties

    /// If any remote homeservers could not be reached, they are
    /// recorded here. The names of the properties are the names of
    /// the unreachable servers.
    ///
    /// If the homeserver could be reached, but the user or device
    /// was unknown, no failure is recorded. Instead, the corresponding
    /// user or device is missing from the `device_keys` result.
    QHash<QString, QJsonObject> failures() const
    {
        return loadFromJson<QHash<QString, QJsonObject>>("failures"_ls);
    }

    /// Information on the queried devices. A map from user ID, to a
    /// map from device ID to device information.  For each device,
    /// the information returned will be the same as uploaded via
    /// `/keys/upload`, with the addition of an `unsigned`
    /// property.
    QHash<QString, QHash<QString, DeviceInformation>> deviceKeys() const
    {
        return loadFromJson<QHash<QString, QHash<QString, DeviceInformation>>>(
            "device_keys"_ls);
    }

    /// Information on the master cross-signing keys of the queried users.
    /// A map from user ID, to master key information.  For each key, the
    /// information returned will be the same as uploaded via
    /// `/keys/device_signing/upload`, along with the signatures
    /// uploaded via `/keys/signatures/upload` that the requesting user
    /// is allowed to see.
    QHash<QString, CrossSigningKey> masterKeys() const
    {
        return loadFromJson<QHash<QString, CrossSigningKey>>("master_keys"_ls);
    }

    /// Information on the self-signing keys of the queried users. A map
    /// from user ID, to self-signing key information.  For each key, the
    /// information returned will be the same as uploaded via
    /// `/keys/device_signing/upload`.
    QHash<QString, CrossSigningKey> selfSigningKeys() const
    {
        return loadFromJson<QHash<QString, CrossSigningKey>>(
            "self_signing_keys"_ls);
    }

    /// Information on the user-signing key of the user making the
    /// request, if they queried their own device information. A map
    /// from user ID, to user-signing key information.  The
    /// information returned will be the same as uploaded via
    /// `/keys/device_signing/upload`.
    QHash<QString, CrossSigningKey> userSigningKeys() const
    {
        return loadFromJson<QHash<QString, CrossSigningKey>>(
            "user_signing_keys"_ls);
    }
};

template <>
struct JsonObjectConverter<QueryKeysJob::UnsignedDeviceInfo> {
    static void fillFrom(const QJsonObject& jo,
                         QueryKeysJob::UnsignedDeviceInfo& result)
    {
        fromJson(jo.value("device_display_name"_ls), result.deviceDisplayName);
    }
};

template <>
struct JsonObjectConverter<QueryKeysJob::DeviceInformation> {
    static void fillFrom(const QJsonObject& jo,
                         QueryKeysJob::DeviceInformation& result)
    {
        fillFromJson<DeviceKeys>(jo, result);
        fromJson(jo.value("unsigned"_ls), result.unsignedData);
    }
    static void dumpTo(QJsonObject& jo, const QueryKeysJob::DeviceInformation& deviceInformation)
    {
        jo = toJson<DeviceKeys>(deviceInformation);
        //addParam<>(jo, "unsigned"_ls, deviceInformation.unsignedData);
    }
};

/*! \brief Claim one-time encryption keys.
 *
 * Claims one-time keys for use in pre-key messages.
 */
class ClaimKeysJob : public BaseJob {
public:
    /*! \brief Claim one-time encryption keys.
     *
     * \param oneTimeKeys
     *   The keys to be claimed. A map from user ID, to a map from
     *   device ID to algorithm name.
     *
     * \param timeout
     *   The time (in milliseconds) to wait when downloading keys from
     *   remote servers. 10 seconds is the recommended default.
     */
    explicit ClaimKeysJob(
        const QHash<QString, QHash<QString, QString>>& oneTimeKeys,
        Omittable<int> timeout = none);

    // Result properties

    /// If any remote homeservers could not be reached, they are
    /// recorded here. The names of the properties are the names of
    /// the unreachable servers.
    ///
    /// If the homeserver could be reached, but the user or device
    /// was unknown, no failure is recorded. Instead, the corresponding
    /// user or device is missing from the `one_time_keys` result.
    QHash<QString, QJsonObject> failures() const
    {
        return loadFromJson<QHash<QString, QJsonObject>>("failures"_ls);
    }

    /// One-time keys for the queried devices. A map from user ID, to a
    /// map from devices to a map from `<algorithm>:<key_id>` to the key object.
    ///
    /// See the [key algorithms](/client-server-api/#key-algorithms) section for
    /// information on the Key Object format.
    QHash<QString, QHash<QString, QVariant>> oneTimeKeys() const
    {
        return loadFromJson<QHash<QString, QHash<QString, QVariant>>>(
            "one_time_keys"_ls);
    }
};

/*! \brief Query users with recent device key updates.
 *
 * Gets a list of users who have updated their device identity keys since a
 * previous sync token.
 *
 * The server should include in the results any users who:
 *
 * * currently share a room with the calling user (ie, both users have
 *   membership state `join`); *and*
 * * added new device identity keys or removed an existing device with
 *   identity keys, between `from` and `to`.
 */
class GetKeysChangesJob : public BaseJob {
public:
    /*! \brief Query users with recent device key updates.
     *
     * \param from
     *   The desired start point of the list. Should be the `next_batch` field
     *   from a response to an earlier call to
     * [`/sync`](/client-server-api/#get_matrixclientr0sync). Users who have not
     *   uploaded new device identity keys since this point, nor deleted
     *   existing devices with identity keys since then, will be excluded
     *   from the results.
     *
     * \param to
     *   The desired end point of the list. Should be the `next_batch`
     *   field from a recent call to
     * [`/sync`](/client-server-api/#get_matrixclientr0sync) - typically the
     * most recent such call. This may be used by the server as a hint to check
     * its caches are up to date.
     */
    explicit GetKeysChangesJob(const QString& from, const QString& to);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetKeysChangesJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& from,
                               const QString& to);

    // Result properties

    /// The Matrix User IDs of all users who updated their device
    /// identity keys.
    QStringList changed() const
    {
        return loadFromJson<QStringList>("changed"_ls);
    }

    /// The Matrix User IDs of all users who may have left all
    /// the end-to-end encrypted rooms they previously shared
    /// with the user.
    QStringList left() const { return loadFromJson<QStringList>("left"_ls); }
};

} // namespace Quotient
