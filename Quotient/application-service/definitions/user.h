// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {

struct QUOTIENT_API ThirdPartyUser {
    //! A Matrix User ID representing a third-party user.
    QString userid;

    //! The protocol ID that the third-party location is a part of.
    QString protocol;

    //! Information used to identify this third-party location.
    QJsonObject fields;
};

template <>
struct JsonObjectConverter<ThirdPartyUser> {
    static void dumpTo(QJsonObject& jo, const ThirdPartyUser& pod)
    {
        addParam<>(jo, "userid"_L1, pod.userid);
        addParam<>(jo, "protocol"_L1, pod.protocol);
        addParam<>(jo, "fields"_L1, pod.fields);
    }
    static void fillFrom(const QJsonObject& jo, ThirdPartyUser& pod)
    {
        fillFromJson(jo.value("userid"_L1), pod.userid);
        fillFromJson(jo.value("protocol"_L1), pod.protocol);
        fillFromJson(jo.value("fields"_L1), pod.fields);
    }
};

} // namespace Quotient
