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

#include "tagevent.h"

using namespace QMatrixClient;

TagRecord::TagRecord(const QJsonObject& json)
    : order(json.value("order").toString())
{ }

TagEvent::TagEvent(const QJsonObject& obj)
    : Event(Type::Tag, obj)
{
    Q_ASSERT(obj["type"].toString() == TypeId);
}

QStringList TagEvent::tagNames() const
{
    return tagsObject().keys();
}

QHash<QString, TagRecord> TagEvent::tags() const
{
    QHash<QString, TagRecord> result;
    auto allTags { tagsObject() };
    for (auto it = allTags.begin(); it != allTags.end(); ++ it)
        result.insert(it.key(), TagRecord(it.value().toObject()));
    return result;
}

QJsonObject TagEvent::tagsObject() const
{
    return contentJson().value("tags").toObject();
}
