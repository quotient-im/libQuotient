/******************************************************************************
 * SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "event.h"

#include "logging.h"

#include <QtCore/QJsonDocument>

using namespace Quotient;

event_type_t EventTypeRegistry::initializeTypeId(event_mtype_t matrixTypeId)
{
    const auto id = get().eventTypes.size();
    get().eventTypes.push_back(matrixTypeId);
    if (strncmp(matrixTypeId, "", 1) == 0)
        qDebug(EVENTS) << "Initialized unknown event type with id" << id;
    else
        qDebug(EVENTS) << "Initialized event type" << matrixTypeId << "with id"
                       << id;
    return id;
}

QString EventTypeRegistry::getMatrixType(event_type_t typeId)
{
    return typeId < get().eventTypes.size() ? get().eventTypes[typeId]
                                            : QString();
}

Event::Event(Type type, const QJsonObject& json) : _type(type), _json(json)
{
    if (!json.contains(ContentKeyL)
        && !json.value(UnsignedKeyL).toObject().contains(RedactedCauseKeyL)) {
        qCWarning(EVENTS) << "Event without 'content' node";
        qCWarning(EVENTS) << formatJson << json;
    }
}

Event::Event(Type type, event_mtype_t matrixType, const QJsonObject& contentJson)
    : Event(type, basicEventJson(matrixType, contentJson))
{}

Event::~Event() = default;

QString Event::matrixType() const { return fullJson()[TypeKeyL].toString(); }

QByteArray Event::originalJson() const { return QJsonDocument(_json).toJson(); }

const QJsonObject Event::contentJson() const
{
    return fullJson()[ContentKeyL].toObject();
}

const QJsonObject Event::unsignedJson() const
{
    return fullJson()[UnsignedKeyL].toObject();
}

void Event::dumpTo(QDebug dbg) const
{
    dbg << QJsonDocument(contentJson()).toJson(QJsonDocument::Compact);
}
