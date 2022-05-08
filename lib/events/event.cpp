// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "event.h"

#include "logging.h"

#include <QtCore/QJsonDocument>

using namespace Quotient;

QString EventTypeRegistry::getMatrixType(event_type_t typeId) { return typeId; }

void _impl::EventFactoryBase::logAddingMethod(event_type_t TypeId,
                                              size_t newSize)
{
    qDebug(EVENTS) << "Adding factory method for" << TypeId << "events;"
                   << newSize << "methods will be in the" << name
                   << "chain";
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
    : Event(type, basicJson(matrixType, contentJson))
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
