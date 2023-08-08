/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include <Quotient/csapi/definitions/key_backup_data.h>
#include <Quotient/csapi/definitions/room_key_backup.h>

#include <Quotient/jobs/basejob.h>

namespace Quotient {

/*! \brief Create a new backup.
 *
 * Creates a new backup.
 */
class QUOTIENT_API PostRoomKeysVersionJob : public BaseJob {
public:
    /*! \brief Create a new backup.
     *
     * \param algorithm
     *   The algorithm used for storing backups.
     *
     * \param authData
     *   Algorithm-dependent data. See the documentation for the backup
     *   algorithms in [Server-side key
     * backups](/client-server-api/#server-side-key-backups) for more
     * information on the expected format of the data.
     */
    explicit PostRoomKeysVersionJob(const QString& algorithm,
                                    const QJsonObject& authData);

    // Result properties

    /// The backup version. This is an opaque string.
    QString version() const { return loadFromJson<QString>("version"_ls); }
};

/*! \brief Get information about the latest backup version.
 *
 * Get information about the latest backup version.
 */
class QUOTIENT_API GetRoomKeysVersionCurrentJob : public BaseJob {
public:
    /// Get information about the latest backup version.
    explicit GetRoomKeysVersionCurrentJob();

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetRoomKeysVersionCurrentJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl);

    // Result properties

    /// The algorithm used for storing backups.
    QString algorithm() const { return loadFromJson<QString>("algorithm"_ls); }

    /// Algorithm-dependent data. See the documentation for the backup
    /// algorithms in [Server-side key
    /// backups](/client-server-api/#server-side-key-backups) for more
    /// information on the expected format of the data.
    QJsonObject authData() const
    {
        return loadFromJson<QJsonObject>("auth_data"_ls);
    }

    /// The number of keys stored in the backup.
    int count() const { return loadFromJson<int>("count"_ls); }

    /// An opaque string representing stored keys in the backup.
    /// Clients can compare it with the `etag` value they received
    /// in the request of their last key storage request.  If not
    /// equal, another client has modified the backup.
    QString etag() const { return loadFromJson<QString>("etag"_ls); }

    /// The backup version.
    QString version() const { return loadFromJson<QString>("version"_ls); }
};

/*! \brief Get information about an existing backup.
 *
 * Get information about an existing backup.
 */
class QUOTIENT_API GetRoomKeysVersionJob : public BaseJob {
public:
    /*! \brief Get information about an existing backup.
     *
     * \param version
     *   The backup version to get, as returned in the `version` parameter
     *   of the response in
     *   [`POST
     * /_matrix/client/v3/room_keys/version`](/client-server-api/#post_matrixclientv3room_keysversion)
     *   or this endpoint.
     */
    explicit GetRoomKeysVersionJob(const QString& version);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetRoomKeysVersionJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& version);

    // Result properties

    /// The algorithm used for storing backups.
    QString algorithm() const { return loadFromJson<QString>("algorithm"_ls); }

    /// Algorithm-dependent data. See the documentation for the backup
    /// algorithms in [Server-side key
    /// backups](/client-server-api/#server-side-key-backups) for more
    /// information on the expected format of the data.
    QJsonObject authData() const
    {
        return loadFromJson<QJsonObject>("auth_data"_ls);
    }

    /// The number of keys stored in the backup.
    int count() const { return loadFromJson<int>("count"_ls); }

    /// An opaque string representing stored keys in the backup.
    /// Clients can compare it with the `etag` value they received
    /// in the request of their last key storage request.  If not
    /// equal, another client has modified the backup.
    QString etag() const { return loadFromJson<QString>("etag"_ls); }

    /// The backup version.
    QString version() const { return loadFromJson<QString>("version"_ls); }
};

/*! \brief Update information about an existing backup.
 *
 * Update information about an existing backup.  Only `auth_data` can be
 * modified.
 */
class QUOTIENT_API PutRoomKeysVersionJob : public BaseJob {
public:
    /*! \brief Update information about an existing backup.
     *
     * \param version
     *   The backup version to update, as returned in the `version`
     *   parameter in the response of
     *   [`POST
     * /_matrix/client/v3/room_keys/version`](/client-server-api/#post_matrixclientv3room_keysversion)
     *   or [`GET
     * /_matrix/client/v3/room_keys/version/{version}`](/client-server-api/#get_matrixclientv3room_keysversionversion).
     *
     * \param algorithm
     *   The algorithm used for storing backups.  Must be the same as
     *   the algorithm currently used by the backup.
     *
     * \param authData
     *   Algorithm-dependent data. See the documentation for the backup
     *   algorithms in [Server-side key
     * backups](/client-server-api/#server-side-key-backups) for more
     * information on the expected format of the data.
     */
    explicit PutRoomKeysVersionJob(const QString& version,
                                   const QString& algorithm,
                                   const QJsonObject& authData);
};

/*! \brief Delete an existing key backup.
 *
 * Delete an existing key backup. Both the information about the backup,
 * as well as all key data related to the backup will be deleted.
 */
class QUOTIENT_API DeleteRoomKeysVersionJob : public BaseJob {
public:
    /*! \brief Delete an existing key backup.
     *
     * \param version
     *   The backup version to delete, as returned in the `version`
     *   parameter in the response of
     *   [`POST
     * /_matrix/client/v3/room_keys/version`](/client-server-api/#post_matrixclientv3room_keysversion)
     *   or [`GET
     * /_matrix/client/v3/room_keys/version/{version}`](/client-server-api/#get_matrixclientv3room_keysversionversion).
     */
    explicit DeleteRoomKeysVersionJob(const QString& version);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for DeleteRoomKeysVersionJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& version);
};

/*! \brief Store a key in the backup.
 *
 * Store a key in the backup.
 */
class QUOTIENT_API PutRoomKeyBySessionIdJob : public BaseJob {
public:
    /*! \brief Store a key in the backup.
     *
     * \param roomId
     *   The ID of the room that the key is for.
     *
     * \param sessionId
     *   The ID of the megolm session that the key is for.
     *
     * \param version
     *   The backup in which to store the key. Must be the current backup.
     *
     * \param data
     *   The key data.
     */
    explicit PutRoomKeyBySessionIdJob(const QString& roomId,
                                      const QString& sessionId,
                                      const QString& version,
                                      const KeyBackupData& data);

    // Result properties

    /// The new etag value representing stored keys in the backup.
    /// See `GET /room_keys/version/{version}` for more details.
    QString etag() const { return loadFromJson<QString>("etag"_ls); }

    /// The number of keys stored in the backup
    int count() const { return loadFromJson<int>("count"_ls); }
};

/*! \brief Retrieve a key from the backup.
 *
 * Retrieve a key from the backup.
 */
class QUOTIENT_API GetRoomKeyBySessionIdJob : public BaseJob {
public:
    /*! \brief Retrieve a key from the backup.
     *
     * \param roomId
     *   The ID of the room that the requested key is for.
     *
     * \param sessionId
     *   The ID of the megolm session whose key is requested.
     *
     * \param version
     *   The backup from which to retrieve the key.
     */
    explicit GetRoomKeyBySessionIdJob(const QString& roomId,
                                      const QString& sessionId,
                                      const QString& version);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetRoomKeyBySessionIdJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId,
                               const QString& sessionId, const QString& version);

    // Result properties

    /// The key data
    KeyBackupData data() const { return fromJson<KeyBackupData>(jsonData()); }
};

/*! \brief Delete a key from the backup.
 *
 * Delete a key from the backup.
 */
class QUOTIENT_API DeleteRoomKeyBySessionIdJob : public BaseJob {
public:
    /*! \brief Delete a key from the backup.
     *
     * \param roomId
     *   The ID of the room that the specified key is for.
     *
     * \param sessionId
     *   The ID of the megolm session whose key is to be deleted.
     *
     * \param version
     *   The backup from which to delete the key
     */
    explicit DeleteRoomKeyBySessionIdJob(const QString& roomId,
                                         const QString& sessionId,
                                         const QString& version);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for DeleteRoomKeyBySessionIdJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId,
                               const QString& sessionId, const QString& version);

    // Result properties

    /// The new etag value representing stored keys in the backup.
    /// See `GET /room_keys/version/{version}` for more details.
    QString etag() const { return loadFromJson<QString>("etag"_ls); }

    /// The number of keys stored in the backup
    int count() const { return loadFromJson<int>("count"_ls); }
};

/*! \brief Store several keys in the backup for a given room.
 *
 * Store several keys in the backup for a given room.
 */
class QUOTIENT_API PutRoomKeysByRoomIdJob : public BaseJob {
public:
    /*! \brief Store several keys in the backup for a given room.
     *
     * \param roomId
     *   The ID of the room that the keys are for.
     *
     * \param version
     *   The backup in which to store the keys. Must be the current backup.
     *
     * \param backupData
     *   The backup data
     */
    explicit PutRoomKeysByRoomIdJob(const QString& roomId,
                                    const QString& version,
                                    const RoomKeyBackup& backupData);

    // Result properties

    /// The new etag value representing stored keys in the backup.
    /// See `GET /room_keys/version/{version}` for more details.
    QString etag() const { return loadFromJson<QString>("etag"_ls); }

    /// The number of keys stored in the backup
    int count() const { return loadFromJson<int>("count"_ls); }
};

/*! \brief Retrieve the keys from the backup for a given room.
 *
 * Retrieve the keys from the backup for a given room.
 */
class QUOTIENT_API GetRoomKeysByRoomIdJob : public BaseJob {
public:
    /*! \brief Retrieve the keys from the backup for a given room.
     *
     * \param roomId
     *   The ID of the room that the requested key is for.
     *
     * \param version
     *   The backup from which to retrieve the key.
     */
    explicit GetRoomKeysByRoomIdJob(const QString& roomId,
                                    const QString& version);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetRoomKeysByRoomIdJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId,
                               const QString& version);

    // Result properties

    /// The key data.  If no keys are found, then an object with an empty
    /// `sessions` property will be returned (`{"sessions": {}}`).
    RoomKeyBackup data() const { return fromJson<RoomKeyBackup>(jsonData()); }
};

/*! \brief Delete the keys from the backup for a given room.
 *
 * Delete the keys from the backup for a given room.
 */
class QUOTIENT_API DeleteRoomKeysByRoomIdJob : public BaseJob {
public:
    /*! \brief Delete the keys from the backup for a given room.
     *
     * \param roomId
     *   The ID of the room that the specified key is for.
     *
     * \param version
     *   The backup from which to delete the key.
     */
    explicit DeleteRoomKeysByRoomIdJob(const QString& roomId,
                                       const QString& version);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for DeleteRoomKeysByRoomIdJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId,
                               const QString& version);

    // Result properties

    /// The new etag value representing stored keys in the backup.
    /// See `GET /room_keys/version/{version}` for more details.
    QString etag() const { return loadFromJson<QString>("etag"_ls); }

    /// The number of keys stored in the backup
    int count() const { return loadFromJson<int>("count"_ls); }
};

/*! \brief Store several keys in the backup.
 *
 * Store several keys in the backup.
 */
class QUOTIENT_API PutRoomKeysJob : public BaseJob {
public:
    /*! \brief Store several keys in the backup.
     *
     * \param version
     *   The backup in which to store the keys. Must be the current backup.
     *
     * \param rooms
     *   A map of room IDs to room key backup data.
     */
    explicit PutRoomKeysJob(const QString& version,
                            const QHash<QString, RoomKeyBackup>& rooms);

    // Result properties

    /// The new etag value representing stored keys in the backup.
    /// See `GET /room_keys/version/{version}` for more details.
    QString etag() const { return loadFromJson<QString>("etag"_ls); }

    /// The number of keys stored in the backup
    int count() const { return loadFromJson<int>("count"_ls); }
};

/*! \brief Retrieve the keys from the backup.
 *
 * Retrieve the keys from the backup.
 */
class QUOTIENT_API GetRoomKeysJob : public BaseJob {
public:
    /*! \brief Retrieve the keys from the backup.
     *
     * \param version
     *   The backup from which to retrieve the keys.
     */
    explicit GetRoomKeysJob(const QString& version);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetRoomKeysJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& version);

    // Result properties

    /// A map of room IDs to room key backup data.
    QHash<QString, RoomKeyBackup> rooms() const
    {
        return loadFromJson<QHash<QString, RoomKeyBackup>>("rooms"_ls);
    }
};

/*! \brief Delete the keys from the backup.
 *
 * Delete the keys from the backup.
 */
class QUOTIENT_API DeleteRoomKeysJob : public BaseJob {
public:
    /*! \brief Delete the keys from the backup.
     *
     * \param version
     *   The backup from which to delete the key
     */
    explicit DeleteRoomKeysJob(const QString& version);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for DeleteRoomKeysJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& version);

    // Result properties

    /// The new etag value representing stored keys in the backup.
    /// See `GET /room_keys/version/{version}` for more details.
    QString etag() const { return loadFromJson<QString>("etag"_ls); }

    /// The number of keys stored in the backup
    int count() const { return loadFromJson<int>("count"_ls); }
};

} // namespace Quotient
