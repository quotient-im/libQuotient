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

#include "event.h"

namespace QMatrixClient
{
    static constexpr const char* FavouriteTag = "m.favourite";
    static constexpr const char* LowPriorityTag = "m.lowpriority";

    struct TagRecord
    {
        explicit TagRecord(const QJsonObject& json = {});

        QString order;
    };

    class TagEvent : public Event
    {
        public:
            explicit TagEvent(const QJsonObject& obj);

            /** Get the list of tag names */
            QStringList tagNames() const;

            /** Get the list of tags along with information on each */
            QHash<QString, TagRecord> tags() const;

            static constexpr const char * TypeId = "m.tag";

        protected:
            QJsonObject tagsObject() const;
    };
}
