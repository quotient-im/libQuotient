/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <QtCore/QJsonObject>

namespace Quotient
{

// Operations

/// Send a state event to the given room.
/*!
 * State events can be sent using this endpoint.  These events will be
 * overwritten if ``<room id>``, ``<event type>`` and ``<state key>`` all
 * match.
 *
 * Requests to this endpoint **cannot use transaction IDs**
 * like other ``PUT`` paths because they cannot be differentiated from the
 * ``state_key``. Furthermore, ``POST`` is unsupported on state paths.
 *
 * The body of the request should be the content object of the event; the
 * fields in this object will vary depending on the type of event. See
 * `Room Events`_ for the ``m.`` event specification.
 */
class SetRoomStateWithKeyJob : public BaseJob
{
public:
    /*! Send a state event to the given room.
     * \param roomId
     *   The room to set the state in
     * \param eventType
     *   The type of event to send.
     * \param stateKey
     *   The state_key for the state to send. Defaults to the empty string.
     * \param body
     *   State events can be sent using this endpoint.  These events will be
     *   overwritten if ``<room id>``, ``<event type>`` and ``<state key>`` all
     *   match.
     *
     *   Requests to this endpoint **cannot use transaction IDs**
     *   like other ``PUT`` paths because they cannot be differentiated from the
     *   ``state_key``. Furthermore, ``POST`` is unsupported on state paths.
     *
     *   The body of the request should be the content object of the event; the
     *   fields in this object will vary depending on the type of event. See
     *   `Room Events`_ for the ``m.`` event specification.
     */
    explicit SetRoomStateWithKeyJob(const QString& roomId,
                                    const QString& eventType,
                                    const QString& stateKey,
                                    const QJsonObject& body = {});

    ~SetRoomStateWithKeyJob() override;

    // Result properties

    /// A unique identifier for the event.
    const QString& eventId() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

/// Send a state event to the given room.
/*!
 * State events can be sent using this endpoint. This endpoint is
 * equivalent to calling `/rooms/{roomId}/state/{eventType}/{stateKey}`
 * with an empty `stateKey`. Previous state events with matching
 * `<roomId>` and `<eventType>`, and empty `<stateKey>`, will be overwritten.
 *
 * Requests to this endpoint **cannot use transaction IDs**
 * like other ``PUT`` paths because they cannot be differentiated from the
 * ``state_key``. Furthermore, ``POST`` is unsupported on state paths.
 *
 * The body of the request should be the content object of the event; the
 * fields in this object will vary depending on the type of event. See
 * `Room Events`_ for the ``m.`` event specification.
 */
class SetRoomStateJob : public BaseJob
{
public:
    /*! Send a state event to the given room.
     * \param roomId
     *   The room to set the state in
     * \param eventType
     *   The type of event to send.
     * \param body
     *   State events can be sent using this endpoint. This endpoint is
     *   equivalent to calling `/rooms/{roomId}/state/{eventType}/{stateKey}`
     *   with an empty `stateKey`. Previous state events with matching
     *   `<roomId>` and `<eventType>`, and empty `<stateKey>`, will be
     * overwritten.
     *
     *   Requests to this endpoint **cannot use transaction IDs**
     *   like other ``PUT`` paths because they cannot be differentiated from the
     *   ``state_key``. Furthermore, ``POST`` is unsupported on state paths.
     *
     *   The body of the request should be the content object of the event; the
     *   fields in this object will vary depending on the type of event. See
     *   `Room Events`_ for the ``m.`` event specification.
     */
    explicit SetRoomStateJob(const QString& roomId, const QString& eventType,
                             const QJsonObject& body = {});

    ~SetRoomStateJob() override;

    // Result properties

    /// A unique identifier for the event.
    const QString& eventId() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace Quotient
