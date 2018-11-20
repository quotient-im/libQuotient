/******************************************************************************
* Copyright (C) 2018 Kitsune Ral <kitsune-ral@users.sf.net>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include "stateevent.h"

namespace QMatrixClient {
    namespace _impl {
        template <typename BaseEventT>
        static inline auto loadEvent(const QJsonObject& json,
                                     const QString& matrixType)
        {
            if (auto e = EventFactory<BaseEventT>::make(json, matrixType))
                return e;
            return makeEvent<BaseEventT>(unknownEventTypeId(), json);
        }
    }

    /** Create an event with proper type from a JSON object
     * Use this factory template to detect the type from the JSON object
     * contents (the detected event type should derive from the template
     * parameter type) and create an event object of that type.
     */
    template <typename BaseEventT>
    inline event_ptr_tt<BaseEventT> loadEvent(const QJsonObject& fullJson)
    {
        return _impl::loadEvent<BaseEventT>(fullJson,
                                            fullJson[TypeKeyL].toString());
    }

    /** Create an event from a type string and content JSON
     * Use this factory template to resolve the C++ type from the Matrix
     * type string in \p matrixType and create an event of that type that has
     * its content part set to \p content.
     */
    template <typename BaseEventT>
    inline event_ptr_tt<BaseEventT> loadEvent(const QString& matrixType,
                                              const QJsonObject& content)
    {
        return _impl::loadEvent<BaseEventT>(basicEventJson(matrixType, content),
                                            matrixType);
    }

    template <typename EventT> struct FromJsonObject<event_ptr_tt<EventT>>
    {
        auto operator()(const QJsonObject& jo) const
        {
            return loadEvent<EventT>(jo);
        }
    };
} // namespace QMatrixClient
