// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"

namespace Quotient {

/// Make a minimal correct Matrix state event JSON
inline QJsonObject basicStateEventJson(const QString& matrixTypeId,
                                       const QJsonObject& content,
                                       const QString& stateKey = {})
{
    return { { TypeKey, matrixTypeId },
             { StateKeyKey, stateKey },
             { ContentKey, content } };
}

class StateEventBase : public RoomEvent {
    Q_GADGET
    Q_PROPERTY(QString stateKey READ stateKey CONSTANT)
public:
    using factory_t = EventFactory<StateEventBase>;

    StateEventBase(Type type, const QJsonObject& json) : RoomEvent(type, json)
    {}
    StateEventBase(Type type, event_mtype_t matrixType,
                   const QString& stateKey = {},
                   const QJsonObject& contentJson = {});
    ~StateEventBase() override = default;

    bool isStateEvent() const override { return true; }
    QString replacedState() const;
    void dumpTo(QDebug dbg) const override;

    virtual bool repeatsState() const;
};
using StateEventPtr = event_ptr_tt<StateEventBase>;
using StateEvents = EventsArray<StateEventBase>;

template <>
inline bool is<StateEventBase>(const Event& e)
{
    return e.isStateEvent();
}

/**
 * A combination of event type and state key uniquely identifies a piece
 * of state in Matrix.
 * \sa
 * https://matrix.org/docs/spec/client_server/unstable.html#types-of-room-events
 */
using StateEventKey = QPair<QString, QString>;

template <typename ContentT>
struct Prev {
    template <typename... ContentParamTs>
    explicit Prev(const QJsonObject& unsignedJson,
                  ContentParamTs&&... contentParams)
        : senderId(unsignedJson.value("prev_sender"_ls).toString())
        , content(unsignedJson.value(PrevContentKeyL).toObject(),
                  std::forward<ContentParamTs>(contentParams)...)
    {}

    QString senderId;
    ContentT content;
};

template <typename ContentT>
class StateEvent : public StateEventBase {
public:
    using content_type = ContentT;

    template <typename... ContentParamTs>
    explicit StateEvent(Type type, const QJsonObject& fullJson,
                        ContentParamTs&&... contentParams)
        : StateEventBase(type, fullJson)
        , _content(contentJson(), std::forward<ContentParamTs>(contentParams)...)
    {
        const auto& unsignedData = unsignedJson();
        if (unsignedData.contains(PrevContentKeyL))
            _prev = std::make_unique<Prev<ContentT>>(
                unsignedData, std::forward<ContentParamTs>(contentParams)...);
    }
    template <typename... ContentParamTs>
    explicit StateEvent(Type type, event_mtype_t matrixType,
                        const QString& stateKey,
                        ContentParamTs&&... contentParams)
        : StateEventBase(type, matrixType, stateKey)
        , _content(std::forward<ContentParamTs>(contentParams)...)
    {
        editJson().insert(ContentKey, _content.toJson());
    }

    const ContentT& content() const { return _content; }
    template <typename VisitorT>
    void editContent(VisitorT&& visitor)
    {
        visitor(_content);
        editJson()[ContentKeyL] = _content.toJson();
    }
    const ContentT* prevContent() const
    {
        return _prev ? &_prev->content : nullptr;
    }
    QString prevSenderId() const { return _prev ? _prev->senderId : QString(); }

private:
    ContentT _content;
    std::unique_ptr<Prev<ContentT>> _prev;
};
} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::StateEventBase*)
Q_DECLARE_METATYPE(const Quotient::StateEventBase*)
