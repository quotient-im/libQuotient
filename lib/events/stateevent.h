// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"

namespace Quotient {

class QUOTIENT_API StateEvent : public RoomEvent {
public:
    QUO_BASE_EVENT(StateEvent, RoomEvent, "json.contains('state_key')")

    static bool isValid(const QJsonObject& fullJson)
    {
        return fullJson.contains(StateKeyKeyL);
    }

    //! \brief Static setting of whether a given even type uses state keys
    //!
    //! Most event types don't use a state key; overriding this to `true`
    //! for a given type changes the calls across Quotient to include state key
    //! in their signatures; otherwise, state key is still accessible but
    //! constructors and calls in, e.g., RoomStateView don't include it.
    static constexpr auto needsStateKey = false;

    explicit StateEvent(Type type, const QString& stateKey = {},
                        const QJsonObject& contentJson = {});

    //! Make a minimal correct Matrix state event JSON
    static QJsonObject basicJson(const QString& matrixTypeId,
                                 const QString& stateKey = {},
                                 const QJsonObject& contentJson = {})
    {
        return { { TypeKey, matrixTypeId },
                 { StateKeyKey, stateKey },
                 { ContentKey, contentJson } };
    }

    QString replacedState() const;
    virtual bool repeatsState() const;

protected:
    explicit StateEvent(const QJsonObject& json);
    void dumpTo(QDebug dbg) const override;
};
using StateEventBase
    [[deprecated("StateEventBase is StateEvent now")]] = StateEvent;
using StateEventPtr = event_ptr_tt<StateEvent>;
using StateEvents = EventsArray<StateEvent>;

[[deprecated("Use StateEvent::basicJson() instead")]]
inline QJsonObject basicStateEventJson(const QString& matrixTypeId,
                                       const QJsonObject& content,
                                       const QString& stateKey = {})
{
    return StateEvent::basicJson(matrixTypeId, stateKey, content);
}

/**
 * A combination of event type and state key uniquely identifies a piece
 * of state in Matrix.
 * \sa
 * https://matrix.org/docs/spec/client_server/unstable.html#types-of-room-events
 */
using StateEventKey = std::pair<QString, QString>;

template <typename EventT, typename ContentT>
class EventTemplate<EventT, StateEvent, ContentT>
    : public StateEvent {
public:
    using content_type = ContentT;

    struct Prev {
        explicit Prev() = default;
        explicit Prev(const QJsonObject& unsignedJson)
            : senderId(fromJson<QString>(unsignedJson["prev_sender"_ls]))
            , content(
                  fromJson<Omittable<ContentT>>(unsignedJson[PrevContentKeyL]))
        {}

        QString senderId;
        Omittable<ContentT> content;
    };

    explicit EventTemplate(const QJsonObject& fullJson)
        : StateEvent(fullJson)
        , _content(fromJson<ContentT>(Event::contentJson()))
        , _prev(unsignedJson())
    {}
    template <typename... ContentParamTs>
    explicit EventTemplate(const QString& stateKey,
                           ContentParamTs&&... contentParams)
        : StateEvent(EventT::TypeId, stateKey)
        , _content { std::forward<ContentParamTs>(contentParams)... }
    {
        editJson().insert(ContentKey, toJson(_content));
    }

    const ContentT& content() const { return _content; }

    template <typename VisitorT>
    void editContent(VisitorT&& visitor)
    {
        visitor(_content);
        editJson()[ContentKeyL] = toJson(_content);
    }
    const Omittable<ContentT>& prevContent() const { return _prev.content; }
    QString prevSenderId() const { return _prev.senderId; }

private:
    ContentT _content;
    Prev _prev;
};

template <typename EventT, typename ContentT>
class KeyedStateEventBase
    : public EventTemplate<EventT, StateEvent, ContentT> {
public:
    static constexpr auto needsStateKey = true;

    using EventTemplate<EventT, StateEvent, ContentT>::EventTemplate;
};

template <typename EvT>
concept Keyed_State_Event = EvT::needsStateKey;

template <typename EventT, typename ContentT>
class KeylessStateEventBase
    : public EventTemplate<EventT, StateEvent, ContentT> {
private:
    using base_type = EventTemplate<EventT, StateEvent, ContentT>;

public:
    template <typename... ContentParamTs>
    explicit KeylessStateEventBase(ContentParamTs&&... contentParams)
        : base_type(QString(), std::forward<ContentParamTs>(contentParams)...)
    {}

protected:
    explicit KeylessStateEventBase(const QJsonObject& fullJson)
        : base_type(fullJson)
    {}
};

template <typename EvT>
concept Keyless_State_Event = !EvT::needsStateKey;

} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::StateEvent*)
Q_DECLARE_METATYPE(const Quotient::StateEvent*)
