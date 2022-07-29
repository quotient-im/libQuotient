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

//! \brief Create an event from a type string and content JSON
//!
//! Use this template to resolve the C++ type from the Matrix type string in
//! \p matrixType and create an event of that type by passing all parameters
//! to BaseEventT::basicJson().
template <typename BaseEventT, typename... BasicJsonParamTs>
inline event_ptr_tt<BaseEventT> loadEvent(
    const QString& matrixType, const BasicJsonParamTs&... basicJsonParams)
{
    return doLoadEvent<BaseEventT>(
        BaseEventT::basicJson(matrixType, basicJsonParams...), matrixType);
}

template <typename EventT>
struct JsonConverter<event_ptr_tt<EventT>>
    : JsonObjectUnpacker<event_ptr_tt<EventT>> {
    using JsonObjectUnpacker<event_ptr_tt<EventT>>::load;
    static auto load(const QJsonObject& jo)
    {
        return loadEvent<EventT>(jo);
    }
};

} // namespace Quotient
