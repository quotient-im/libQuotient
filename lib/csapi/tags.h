/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include "jobs/basejob.h"

#include <QtCore/QHash>
#include <QtCore/QVariant>

namespace Quotient
{

// Operations

/// List the tags for a room.
/*!
 * List the tags set by a user on a room.
 */
class GetRoomTagsJob : public BaseJob
{
public:
    // Inner data structures

    /// List the tags set by a user on a room.
    struct Tag
    {
        /// A number in a range ``[0,1]`` describing a relativeposition of the
        /// room under the given tag.
        Omittable<float> order;

        /// List the tags set by a user on a room.
        QVariantHash additionalProperties;
    };

    // Construction/destruction

    /*! List the tags for a room.
     * \param userId
     *   The id of the user to get tags for. The access token must be
     *   authorized to make requests for this user ID.
     * \param roomId
     *   The ID of the room to get tags for.
     */
    explicit GetRoomTagsJob(const QString& userId, const QString& roomId);

    /*! Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for
     * GetRoomTagsJob is necessary but the job
     * itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId,
                               const QString& roomId);

    ~GetRoomTagsJob() override;

    // Result properties

    /// List the tags set by a user on a room.
    const QHash<QString, Tag>& tags() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

/// Add a tag to a room.
/*!
 * Add a tag to the room.
 */
class SetRoomTagJob : public BaseJob
{
public:
    /*! Add a tag to a room.
     * \param userId
     *   The id of the user to add a tag for. The access token must be
     *   authorized to make requests for this user ID.
     * \param roomId
     *   The ID of the room to add a tag to.
     * \param tag
     *   The tag to add.
     * \param order
     *   A number in a range ``[0,1]`` describing a relative
     *   position of the room under the given tag.
     */
    explicit SetRoomTagJob(const QString& userId, const QString& roomId,
                           const QString& tag, Omittable<float> order = none);
};

/// Remove a tag from the room.
/*!
 * Remove a tag from the room.
 */
class DeleteRoomTagJob : public BaseJob
{
public:
    /*! Remove a tag from the room.
     * \param userId
     *   The id of the user to remove a tag for. The access token must be
     *   authorized to make requests for this user ID.
     * \param roomId
     *   The ID of the room to remove a tag from.
     * \param tag
     *   The tag to remove.
     */
    explicit DeleteRoomTagJob(const QString& userId, const QString& roomId,
                              const QString& tag);

    /*! Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for
     * DeleteRoomTagJob is necessary but the job
     * itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId,
                               const QString& roomId, const QString& tag);
};

} // namespace Quotient
