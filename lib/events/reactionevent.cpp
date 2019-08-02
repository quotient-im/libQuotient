/******************************************************************************
 * Copyright (C) 2019 Kitsune Ral <kitsune-ral@users.sf.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "reactionevent.h"

using namespace QMatrixClient;

void QMatrixClient::JsonObjectConverter<EventRelation>::dumpTo(
    QJsonObject& jo, const EventRelation& pod)
{
    if (pod.type.isEmpty()) {
        qCWarning(MAIN) << "Empty relation type; won't dump to JSON";
        return;
    }
    jo.insert(QStringLiteral("rel_type"), pod.type);
    jo.insert(EventIdKey, pod.eventId);
    if (pod.type == EventRelation::Annotation())
        jo.insert(QStringLiteral("key"), pod.key);
}

void QMatrixClient::JsonObjectConverter<EventRelation>::fillFrom(
    const QJsonObject& jo, EventRelation& pod)
{
    // The experimental logic for generic relationships (MSC1849)
    fromJson(jo["rel_type"_ls], pod.type);
    fromJson(jo[EventIdKeyL], pod.eventId);
    if (pod.type == EventRelation::Annotation())
        fromJson(jo["key"_ls], pod.key);
}
