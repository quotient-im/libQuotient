// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "stateevent.h"

namespace Quotient {

/*! Create an event with proper type from a JSON object
 *
 * Use this factory template to detect the type from the JSON object
 * contents (the detected event type should derive from the template
 * parameter type) and create an event object of that type.
 */
template <typename BaseEventT>
inline event_ptr_tt<BaseEventT> loadEvent(const QJsonObject& fullJson)
{
    return doLoadEvent<BaseEventT>(fullJson, fullJson[TypeKeyL].toString());
}

/*! Create an event from a type string and content JSON
 *
 * Use this factory template to resolve the C++ type from the Matrix
 * type string in \p matrixType and create an event of that type that has
 * its content part set to \p content.
 */
template <typename BaseEventT>
inline event_ptr_tt<BaseEventT> loadEvent(const QString& matrixType,
                                          const QJsonObject& content)
{
    return doLoadEvent<BaseEventT>(Event::basicJson(matrixType, content),
                                   matrixType);
}

/*! Create a state event from a type string, content JSON and state key
 *
 * Use this factory to resolve the C++ type from the Matrix type string
 * in \p matrixType and create a state event of that type with content part
 * set to \p content and state key set to \p stateKey (empty by default).
 */
inline StateEventPtr loadStateEvent(const QString& matrixType,
                                    const QJsonObject& content,
                                    const QString& stateKey = {})
{
    return doLoadEvent<StateEventBase>(
        StateEventBase::basicJson(matrixType, content, stateKey), matrixType);
}

template <typename EventT>
struct JsonConverter<event_ptr_tt<EventT>> {
    static auto load(const QJsonValue& jv)
    {
        return loadEvent<EventT>(jv.toObject());
    }
    static auto load(const QJsonDocument& jd)
    {
        return loadEvent<EventT>(jd.object());
    }
};
} // namespace Quotient
