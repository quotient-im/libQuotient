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

#include "roomevent.h"

namespace QMatrixClient {
    class StateEventBase: public RoomEvent
    {
        public:
            using factory_t = EventFactory<StateEventBase>;

            using RoomEvent::RoomEvent;
            ~StateEventBase() override = default;

            bool isStateEvent() const override { return true; }
            void dumpTo(QDebug dbg) const override;

            virtual bool repeatsState() const;
    };
    using StateEventPtr = event_ptr_tt<StateEventBase>;
    using StateEvents = EventsArray<StateEventBase>;

    template <typename ContentT>
    struct Prev
    {
        template <typename... ContentParamTs>
        explicit Prev(const QJsonObject& unsignedJson,
                      ContentParamTs&&... contentParams)
            : senderId(unsignedJson.value("prev_sender"_ls).toString())
            , content(unsignedJson.value(PrevContentKeyL).toObject(),
                       std::forward<ContentParamTs>(contentParams)...)
        { }

        QString senderId;
        ContentT content;
    };

    template <typename ContentT>
    class StateEvent: public StateEventBase
    {
        public:
            using content_type = ContentT;

            template <typename... ContentParamTs>
            explicit StateEvent(Type type, const QJsonObject& fullJson,
                                ContentParamTs&&... contentParams)
                : StateEventBase(type, fullJson)
                , _content(contentJson(),
                           std::forward<ContentParamTs>(contentParams)...)
            {
                const auto& unsignedData = unsignedJson();
                if (unsignedData.contains(PrevContentKeyL))
                    _prev = std::make_unique<Prev<ContentT>>(unsignedData,
                                std::forward<ContentParamTs>(contentParams)...);
            }
            template <typename... ContentParamTs>
            explicit StateEvent(Type type, event_mtype_t matrixType,
                                ContentParamTs&&... contentParams)
                : StateEventBase(type, matrixType)
                , _content(std::forward<ContentParamTs>(contentParams)...)
            {
                editJson().insert(ContentKey, _content.toJson());
            }

            const ContentT& content() const { return _content; }
            [[deprecated("Use prevContent instead")]]
            const ContentT* prev_content() const { return prevContent(); }
            const ContentT* prevContent() const
            { return _prev ? &_prev->content : nullptr; }
            QString prevSenderId() const
                { return _prev ? _prev->senderId : QString(); }

        protected:
            ContentT _content;
            std::unique_ptr<Prev<ContentT>> _prev;
    };
} // namespace QMatrixClient
