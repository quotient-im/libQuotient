// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "event.h"

#include "../logging_categories_p.h"
#include "../ranges_extras.h"

#include <QtCore/QJsonDocument>

#if Quotient_VERSION_MAJOR == 0 && Quotient_VERSION_MINOR <= 9
#include "stateevent.h" // For deprecated isStateEvent(); remove, once Event::isStateEvent() is gone
#endif

using namespace Quotient;

void AbstractEventMetaType::addDerived(const AbstractEventMetaType* newType)
{
    if (const auto existing =
            findIndirect(_derivedTypes, newType->matrixId, &AbstractEventMetaType::matrixId);
        existing != _derivedTypes.cend()) {
        if (*existing == newType)
            return;
        // Two different metatype objects claim the same Matrix type id; this
        // is not normal, so give as much information as possible to diagnose
        if ((*existing)->className == newType->className) {
            qCritical(EVENTS) << newType->className << "claims" << newType->matrixId
                              << "repeatedly; check that it's exported across translation "
                                 "units or shared objects";
            Q_ASSERT(false); // That situation is plain wrong
            return; // So maybe std::terminate() even?
        }
        qWarning(EVENTS).nospace() << newType->matrixId << " is already mapped to "
                                   << (*existing)->className << " before " << newType->className
                                   << "; unless the two have different isValid() conditions, the "
                                      "latter class will never be used";
    }
    _derivedTypes.emplace_back(newType);
    qDebug(EVENTS).nospace() << newType->matrixId << " -> " << newType->className << "; "
                             << _derivedTypes.size() << " derived type(s) registered for "
                             << className;
}

Event::Event(const QJsonObject& json) : _json(json) {}

Event::~Event() = default;

QString Event::matrixType() const { return fullJson()[TypeKey].toString(); }

const QJsonObject Event::contentJson() const
{
    return fullJson()[ContentKey].toObject();
}

const QJsonObject Event::unsignedJson() const
{
    return fullJson()[UnsignedKey].toObject();
}

#if Quotient_VERSION_MAJOR == 0 && Quotient_VERSION_MINOR <= 9
bool Event::isStateEvent() const { return is<StateEvent>(); }
#endif

void Event::dumpTo(QDebug dbg) const
{
    dbg << QJsonDocument(contentJson()).toJson(QJsonDocument::Compact);
}
