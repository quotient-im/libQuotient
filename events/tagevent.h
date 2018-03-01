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
            TagEvent();
            explicit TagEvent(const QJsonObject& obj);

            /** Get the list of tag names */
            QStringList tagNames() const;

            /** Get the list of tags along with information on each */
            QHash<QString, TagRecord> tags() const;

            /** Check if the event lists no tags */
            bool empty() const;

            /** Check whether the tags list contains the specified name */
            bool contains(const QString& name) const;

            /** Get the record for the given tag name */
            TagRecord recordForTag(const QString& name) const;

            /** Get the whole tags content as a JSON object
             * It's NOT recommended to use this method directly from client code.
             * Use other convenience methods provided by the class.
             */
            QJsonObject tagsObject() const;

            static constexpr const char * TypeId = "m.tag";
    };

    using TagEventPtr = event_ptr_tt<TagEvent>;

    inline QJsonValue toJson(const TagEventPtr& tagEvent)
    {
        return QJsonObject {{ "type", "m.tag" },
            // TODO: Replace tagsObject() with a genuine list of tags
            // (or make the needed JSON upon TagEvent creation)
            { "content", QJsonObject {{ "tags", tagEvent->tagsObject() }} }};
    }
}
