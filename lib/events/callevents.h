// SPDX-FileCopyrightText: 2022 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"

namespace Quotient {

class QUOTIENT_API CallEvent : public RoomEvent {
public:
    QUO_BASE_EVENT(CallEvent, RoomEvent, "m.call.*")
    static bool isValid(const QJsonObject&, const QString& mType)
    {
        return mType.startsWith("m.call.");
    }

    QUO_CONTENT_GETTER(QString, callId)
    QUO_CONTENT_GETTER(int, version)

protected:
    explicit CallEvent(const QJsonObject& json);

    static QJsonObject basicJson(const QString& matrixType,
                                 const QString& callId, int version,
                                 QJsonObject contentJson = {});
};
using CallEventBase
    [[deprecated("CallEventBase is CallEvent now")]] = CallEvent;

template <typename EventT>
class EventTemplate<EventT, CallEvent> : public CallEvent {
public:
    using CallEvent::CallEvent;
    explicit EventTemplate(const QString& callId,
                           const QJsonObject& contentJson = {})
        : EventTemplate(basicJson(EventT::TypeId, callId, 0, contentJson))
    {}
};

template <typename EventT, typename ContentT>
class EventTemplate<EventT, CallEvent, ContentT>
    : public EventTemplate<EventT, CallEvent> {
public:
    using EventTemplate<EventT, CallEvent>::EventTemplate;
    template <typename... ContentParamTs>
    explicit EventTemplate(const QString& callId,
                           ContentParamTs&&... contentParams)
        : EventTemplate<EventT, CallEvent>(
            callId,
            toJson(ContentT{ std::forward<ContentParamTs>(contentParams)... }))
    {}
};

class QUOTIENT_API CallInviteEvent
    : public EventTemplate<CallInviteEvent, CallEvent> {
public:
    QUO_EVENT(CallInviteEvent, "m.call.invite")

    using EventTemplate::EventTemplate;

    explicit CallInviteEvent(const QString& callId, int lifetime,
                             const QString& sdp);

    QUO_CONTENT_GETTER(int, lifetime)
    QString sdp() const
    {
        return contentPart<QJsonObject>("offer"_ls).value("sdp"_ls).toString();
    }
};

DEFINE_SIMPLE_EVENT(CallCandidatesEvent, CallEvent, "m.call.candidates",
                    QJsonArray, candidates, "candidates")

class QUOTIENT_API CallAnswerEvent
    : public EventTemplate<CallAnswerEvent, CallEvent> {
public:
    QUO_EVENT(CallAnswerEvent, "m.call.answer")

    using EventTemplate::EventTemplate;

    explicit CallAnswerEvent(const QString& callId, const QString& sdp);

    QString sdp() const
    {
        return contentPart<QJsonObject>("answer"_ls).value("sdp"_ls).toString();
    }
};

class QUOTIENT_API CallHangupEvent
    : public EventTemplate<CallHangupEvent, CallEvent> {
public:
    QUO_EVENT(CallHangupEvent, "m.call.hangup")
    using EventTemplate::EventTemplate;
};

} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::CallEvent*)
Q_DECLARE_METATYPE(const Quotient::CallEvent*)
