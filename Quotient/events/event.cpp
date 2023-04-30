// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "event.h"

#include "callevents.h"
#include <Quotient/logging.h>
#include "stateevent.h"

#include <QtCore/QJsonDocument>

using namespace Quotient;

QString EventTypeRegistry::getMatrixType(event_type_t typeId) { return typeId; }

void AbstractEventMetaType::addDerived(const AbstractEventMetaType* newType)
{
    if (const auto existing =
            std::find_if(derivedTypes.cbegin(), derivedTypes.cend(),
                         [&newType](const AbstractEventMetaType* t) {
                             return t->matrixId == newType->matrixId;
                         });
        existing != derivedTypes.cend())
    {
        if (*existing == newType)
            return;
        // Two different metatype objects claim the same Matrix type id; this
        // is not normal, so give as much information as possible to diagnose
        if ((*existing)->className == newType->className) {
            qCritical(EVENTS)
                << newType->className << "claims" << newType->matrixId
                << "repeatedly; check that it's exported across translation "
                   "units or shared objects";
            Q_ASSERT(false); // That situation is plain wrong
            return; // So maybe std::terminate() even?
        }
        qWarning(EVENTS).nospace()
            << newType->matrixId << " is already mapped to "
            << (*existing)->className << " before " << newType->className
            << "; unless the two have different isValid() conditions, the "
               "latter class will never be used";
    }
    derivedTypes.emplace_back(newType);
    qDebug(EVENTS).nospace()
        << newType->matrixId << " -> " << newType->className << "; "
        << derivedTypes.size() << " derived type(s) registered for "
        << className;
}

Event::Event(const QJsonObject& json)
    : _json(json)
{
    if (!json.contains(ContentKey)
        && !json.value(UnsignedKey).toObject().contains(RedactedCauseKey)) {
        qCWarning(EVENTS) << "Event without 'content' node";
        qCWarning(EVENTS) << formatJson << json;
    }
}

Event::~Event() = default;

QString Event::matrixType() const { return fullJson()[TypeKey].toString(); }

QByteArray Event::originalJson() const { return QJsonDocument(_json).toJson(); }

const QJsonObject Event::contentJson() const
{
    return fullJson()[ContentKey].toObject();
}

const QJsonObject Event::unsignedJson() const
{
    return fullJson()[UnsignedKey].toObject();
}

bool Event::isStateEvent() const { return is<StateEvent>(); }

bool Event::isCallEvent() const { return is<CallEvent>(); }

void Event::dumpTo(QDebug dbg) const
{
    dbg << QJsonDocument(contentJson()).toJson(QJsonDocument::Compact);
}
