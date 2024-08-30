// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {
//! A signature of an `m.third_party_invite` token to prove that this user
//! owns a third-party identity which has been invited to the room.
struct QUOTIENT_API ThirdPartySigned {
    //! The Matrix ID of the user who issued the invite.
    QString sender;

    //! The Matrix ID of the invitee.
    QString mxid;

    //! The state key of the m.third_party_invite event.
    QString token;

    //! A signatures object containing a signature of the entire signed object.
    QHash<QString, QHash<QString, QString>> signatures;
};

template <>
struct JsonObjectConverter<ThirdPartySigned> {
    static void dumpTo(QJsonObject& jo, const ThirdPartySigned& pod)
    {
        addParam<>(jo, "sender"_L1, pod.sender);
        addParam<>(jo, "mxid"_L1, pod.mxid);
        addParam<>(jo, "token"_L1, pod.token);
        addParam<>(jo, "signatures"_L1, pod.signatures);
    }
    static void fillFrom(const QJsonObject& jo, ThirdPartySigned& pod)
    {
        fillFromJson(jo.value("sender"_L1), pod.sender);
        fillFromJson(jo.value("mxid"_L1), pod.mxid);
        fillFromJson(jo.value("token"_L1), pod.token);
        fillFromJson(jo.value("signatures"_L1), pod.signatures);
    }
};

} // namespace Quotient
