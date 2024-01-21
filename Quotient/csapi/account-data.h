/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include <Quotient/jobs/basejob.h>

namespace Quotient {

/*! \brief Set some account data for the user.
 *
 * Set some account data for the client. This config is only visible to the user
 * that set the account data. The config will be available to clients through
 * the top-level `account_data` field in the homeserver response to
 * [/sync](#get_matrixclientv3sync).
 */
class QUOTIENT_API SetAccountDataJob : public BaseJob {
public:
    /*! \brief Set some account data for the user.
     *
     * \param userId
     *   The ID of the user to set account data for. The access token must be
     *   authorized to make requests for this user ID.
     *
     * \param type
     *   The event type of the account data to set. Custom types should be
     *   namespaced to avoid clashes.
     *
     * \param content
     *   The content of the account data.
     */
    explicit SetAccountDataJob(const QString& userId, const QString& type,
                               const QJsonObject& content = {});
};

/*! \brief Get some account data for the user.
 *
 * Get some account data for the client. This config is only visible to the user
 * that set the account data.
 */
class QUOTIENT_API GetAccountDataJob : public BaseJob {
public:
    /*! \brief Get some account data for the user.
     *
     * \param userId
     *   The ID of the user to get account data for. The access token must be
     *   authorized to make requests for this user ID.
     *
     * \param type
     *   The event type of the account data to get. Custom types should be
     *   namespaced to avoid clashes.
     */
    explicit GetAccountDataJob(const QString& userId, const QString& type);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetAccountDataJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId,
                               const QString& type);

    // Result properties

    /// The account data content for the given type.
    QJsonObject data() const { return fromJson<QJsonObject>(jsonData()); }
};

/*! \brief Set some account data for the user that is specific to a room.
 *
 * Set some account data for the client on a given room. This config is only
 * visible to the user that set the account data. The config will be delivered
 * to clients in the per-room entries via [/sync](#get_matrixclientv3sync).
 */
class QUOTIENT_API SetAccountDataPerRoomJob : public BaseJob {
public:
    /*! \brief Set some account data for the user that is specific to a room.
     *
     * \param userId
     *   The ID of the user to set account data for. The access token must be
     *   authorized to make requests for this user ID.
     *
     * \param roomId
     *   The ID of the room to set account data on.
     *
     * \param type
     *   The event type of the account data to set. Custom types should be
     *   namespaced to avoid clashes.
     *
     * \param content
     *   The content of the account data.
     */
    explicit SetAccountDataPerRoomJob(const QString& userId,
                                      const QString& roomId, const QString& type,
                                      const QJsonObject& content = {});
};

/*! \brief Get some account data for the user that is specific to a room.
 *
 * Get some account data for the client on a given room. This config is only
 * visible to the user that set the account data.
 */
class QUOTIENT_API GetAccountDataPerRoomJob : public BaseJob {
public:
    /*! \brief Get some account data for the user that is specific to a room.
     *
     * \param userId
     *   The ID of the user to get account data for. The access token must be
     *   authorized to make requests for this user ID.
     *
     * \param roomId
     *   The ID of the room to get account data for.
     *
     * \param type
     *   The event type of the account data to get. Custom types should be
     *   namespaced to avoid clashes.
     */
    explicit GetAccountDataPerRoomJob(const QString& userId,
                                      const QString& roomId,
                                      const QString& type);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetAccountDataPerRoomJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId,
                               const QString& roomId, const QString& type);

    // Result properties

    /// The account data content for the given type.
    QJsonObject data() const { return fromJson<QJsonObject>(jsonData()); }
};

} // namespace Quotient
