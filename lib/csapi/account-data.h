/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Set some account_data for the user.
 *
 * Set some account_data for the client. This config is only visible to the user
 * that set the account_data. The config will be synced to clients in the
 * top-level `account_data`.
 */
class QUOTIENT_API SetAccountDataJob : public BaseJob {
public:
    /*! \brief Set some account_data for the user.
     *
     * \param userId
     *   The ID of the user to set account_data for. The access token must be
     *   authorized to make requests for this user ID.
     *
     * \param type
     *   The event type of the account_data to set. Custom types should be
     *   namespaced to avoid clashes.
     *
     * \param content
     *   The content of the account_data
     */
    explicit SetAccountDataJob(const QString& userId, const QString& type,
                               const QJsonObject& content = {});
};

/*! \brief Get some account_data for the user.
 *
 * Get some account_data for the client. This config is only visible to the user
 * that set the account_data.
 */
class QUOTIENT_API GetAccountDataJob : public BaseJob {
public:
    /*! \brief Get some account_data for the user.
     *
     * \param userId
     *   The ID of the user to get account_data for. The access token must be
     *   authorized to make requests for this user ID.
     *
     * \param type
     *   The event type of the account_data to get. Custom types should be
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
};

/*! \brief Set some account_data for the user.
 *
 * Set some account_data for the client on a given room. This config is only
 * visible to the user that set the account_data. The config will be synced to
 * clients in the per-room `account_data`.
 */
class QUOTIENT_API SetAccountDataPerRoomJob : public BaseJob {
public:
    /*! \brief Set some account_data for the user.
     *
     * \param userId
     *   The ID of the user to set account_data for. The access token must be
     *   authorized to make requests for this user ID.
     *
     * \param roomId
     *   The ID of the room to set account_data on.
     *
     * \param type
     *   The event type of the account_data to set. Custom types should be
     *   namespaced to avoid clashes.
     *
     * \param content
     *   The content of the account_data
     */
    explicit SetAccountDataPerRoomJob(const QString& userId,
                                      const QString& roomId, const QString& type,
                                      const QJsonObject& content = {});
};

/*! \brief Get some account_data for the user.
 *
 * Get some account_data for the client on a given room. This config is only
 * visible to the user that set the account_data.
 */
class QUOTIENT_API GetAccountDataPerRoomJob : public BaseJob {
public:
    /*! \brief Get some account_data for the user.
     *
     * \param userId
     *   The ID of the user to set account_data for. The access token must be
     *   authorized to make requests for this user ID.
     *
     * \param roomId
     *   The ID of the room to get account_data for.
     *
     * \param type
     *   The event type of the account_data to get. Custom types should be
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
};

} // namespace Quotient
