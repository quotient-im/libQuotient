/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

namespace Quotient {
/// Used by clients to discover homeserver information.
struct HomeserverInformation {
    /// The base URL for the homeserver for client-server connections.
    QUrl baseUrl;
};

template <>
struct JsonObjectConverter<HomeserverInformation> {
    static void dumpTo(QJsonObject& jo, const HomeserverInformation& pod)
    {
        addParam<>(jo, QStringLiteral("base_url"), pod.baseUrl);
    }
    static void fillFrom(const QJsonObject& jo, HomeserverInformation& pod)
    {
        fromJson(jo.value("base_url"_ls), pod.baseUrl);
    }
};

} // namespace Quotient
