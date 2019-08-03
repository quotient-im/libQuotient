/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace QMatrixClient
{

// Operations

/// Stop the requesting user participating in a particular room.
/*!
 * This API stops a user participating in a particular room.
 *
 * If the user was already in the room, they will no longer be able to see
 * new events in the room. If the room requires an invite to join, they
 * will need to be re-invited before they can re-join.
 *
 * If the user was invited to the room, but had not joined, this call
 * serves to reject the invite.
 *
 * The user will still be allowed to retrieve history from the room which
 * they were previously allowed to see.
 */
class LeaveRoomJob : public BaseJob
{
public:
    /*! Stop the requesting user participating in a particular room.
     * \param roomId
     *   The room identifier to leave.
     */
    explicit LeaveRoomJob(const QString& roomId);

    /*! Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for
     * LeaveRoomJob is necessary but the job
     * itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId);
};

/// Stop the requesting user remembering about a particular room.
/*!
 * This API stops a user remembering about a particular room.
 *
 * In general, history is a first class citizen in Matrix. After this API
 * is called, however, a user will no longer be able to retrieve history
 * for this room. If all users on a homeserver forget a room, the room is
 * eligible for deletion from that homeserver.
 *
 * If the user is currently joined to the room, they must leave the room
 * before calling this API.
 */
class ForgetRoomJob : public BaseJob
{
public:
    /*! Stop the requesting user remembering about a particular room.
     * \param roomId
     *   The room identifier to forget.
     */
    explicit ForgetRoomJob(const QString& roomId);

    /*! Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for
     * ForgetRoomJob is necessary but the job
     * itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId);
};

} // namespace QMatrixClient
