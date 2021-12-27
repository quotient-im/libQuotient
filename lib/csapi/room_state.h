/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Send a state event to the given room.
 *
 * State events can be sent using this endpoint.  These events will be
 * overwritten if `<room id>`, `<event type>` and `<state key>` all
 * match.
 *
 * Requests to this endpoint **cannot use transaction IDs**
 * like other `PUT` paths because they cannot be differentiated from the
 * `state_key`. Furthermore, `POST` is unsupported on state paths.
 *
 * The body of the request should be the content object of the event; the
 * fields in this object will vary depending on the type of event. See
 * [Room Events](/client-server-api/#room-events) for the `m.` event
 * specification.
 *
 * If the event type being sent is `m.room.canonical_alias` servers
 * SHOULD ensure that any new aliases being listed in the event are valid
 * per their grammar/syntax and that they point to the room ID where the
 * state event is to be sent. Servers do not validate aliases which are
 * being removed or are already present in the state event.
 */
class SetRoomStateWithKeyJob : public BaseJob {
public:
    /*! \brief Send a state event to the given room.
     *
     * \param roomId
     *   The room to set the state in
     *
     * \param eventType
     *   The type of event to send.
     *
     * \param stateKey
     *   The state_key for the state to send. Defaults to the empty string. When
     *   an empty string, the trailing slash on this endpoint is optional.
     *
     * \param body
     *   State events can be sent using this endpoint.  These events will be
     *   overwritten if `<room id>`, `<event type>` and `<state key>` all
     *   match.
     *
     *   Requests to this endpoint **cannot use transaction IDs**
     *   like other `PUT` paths because they cannot be differentiated from the
     *   `state_key`. Furthermore, `POST` is unsupported on state paths.
     *
     *   The body of the request should be the content object of the event; the
     *   fields in this object will vary depending on the type of event. See
     *   [Room Events](/client-server-api/#room-events) for the `m.` event
     * specification.
     *
     *   If the event type being sent is `m.room.canonical_alias` servers
     *   SHOULD ensure that any new aliases being listed in the event are valid
     *   per their grammar/syntax and that they point to the room ID where the
     *   state event is to be sent. Servers do not validate aliases which are
     *   being removed or are already present in the state event.
     */
    explicit SetRoomStateWithKeyJob(const QString& roomId,
                                    const QString& eventType,
                                    const QString& stateKey,
                                    const QJsonObject& body = {});

    // Result properties

    /// A unique identifier for the event.
    QString eventId() const { return loadFromJson<QString>("event_id"_ls); }
};

} // namespace Quotient
