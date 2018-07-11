/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
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

#include "event.h"

#include "logging.h"

#include <QtCore/QJsonDocument>

using namespace QMatrixClient;

event_type_t EventTypeRegistry::initializeTypeId(event_mtype_t matrixTypeId)
{
    const auto id = get().eventTypes.size();
    get().eventTypes.push_back(matrixTypeId);
    if (strncmp(matrixTypeId, "", 1) == 0)
        qDebug(EVENTS) << "Initialized unknown event type with id" << id;
    else
        qDebug(EVENTS) << "Initialized event type" << matrixTypeId
                       << "with id" << id;
    return id;
}

QString EventTypeRegistry::getMatrixType(event_type_t typeId)
{
    return typeId < get().eventTypes.size() ? get().eventTypes[typeId] : "";
}

Event::Event(Type type, const QJsonObject& json)
    : _type(type), _json(json)
{
    if (!json.contains(ContentKeyL) &&
            !json.value(UnsignedKeyL).toObject().contains(RedactedCauseKeyL))
    {
        qCWarning(EVENTS) << "Event without 'content' node";
        qCWarning(EVENTS) << formatJson << json;
    }
}

Event::Event(Type type, event_mtype_t matrixType, const QJsonObject& contentJson)
    : Event(type, basicEventJson(matrixType, contentJson))
{ }

Event::~Event() = default;

QString Event::matrixType() const
{
    return fullJson()[TypeKeyL].toString();
}

QByteArray Event::originalJson() const
{
    return QJsonDocument(_json).toJson();
}

const QJsonObject Event::contentJson() const
{
    return fullJson()[ContentKeyL].toObject();
}

const QJsonObject Event::unsignedJson() const
{
    return fullJson()[UnsignedKeyL].toObject();
}
