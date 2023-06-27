// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "event.h"

#include <Quotient/logging.h>
#include "stateevent.h"

#include <QtCore/QJsonDocument>

using namespace Quotient;

Event::Event(const QJsonObject& json)
    : _json(json)
{
    if (!json.contains(ContentKey)
        && !json.value(UnsignedKey).toObject().contains(RedactedCauseKey)) {
        qCWarning(EVENTS) << "Event without 'content' node";
        qCWarning(EVENTS) << formatJson << json;
    }
}

QString Event::matrixType() const { return fullJson()[TypeKey].toString(); }

const QJsonObject Event::contentJson() const
{
    return fullJson()[ContentKey].toObject();
}

const QJsonObject Event::unsignedJson() const
{
    return fullJson()[UnsignedKey].toObject();
}

bool Event::isStateEvent() const { return is<StateEvent>(); }

void Event::dumpTo(QDebug dbg) const
{
    dbg << QJsonDocument(contentJson()).toJson(QJsonDocument::Compact);
}
