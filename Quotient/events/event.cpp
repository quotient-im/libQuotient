// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "event.h"

#include "../logging_categories_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QStringBuilder>

#include <ranges>

using namespace Quotient;
using namespace std::ranges;

namespace {
std::pair<const AbstractEventMetaType *, event_type_t> findFirstOverlap(
    std::span<const AbstractEventMetaType *const> metaTypes, range auto matrixTypeIds)
{
    for (const auto *metaType : metaTypes)
        if (const auto overlappingIdIt = find_first_of(matrixTypeIds, metaType->matrixIds);
            overlappingIdIt != end(matrixTypeIds))
            return {metaType, *overlappingIdIt};
    return {};
}
}

AbstractEventMetaType::AbstractEventMetaType(const std::type_info &typeInfo, const char *className,
                                             AbstractEventMetaType *nearestBase)
    : typeInfo(typeInfo), className(className), baseType(nearestBase)
{
    auto dbg = qDebug(EVENTS).nospace();
    dbg << "New base event class " << className;
    if (nearestBase) {
        // We can't check attempts to use the same Matrix type ids here but we can check attempts
        // to use the same className at least
        if (const auto existing =
                find(nearestBase->derivedTypes(), className, &AbstractEventMetaType::className);
            existing != cend(nearestBase->derivedTypes())) {
            QUO_ALARM_X(*existing == this, "Attempt to re-register the same event class");
            QUO_ALARM_X(true, std::format("Attempt to register distinct classes with the same name "
                                          "{:s}; check that the C++ symbol is properly exported",
                                          className));
            return;
        }
        nearestBase->_derivedTypes.emplace_back(this);
        dbg << ", derived from " << nearestBase->className << "; "
            << nearestBase->_derivedTypes.size() << " type(s) derived from "
            << nearestBase->className;
    }
}

AbstractEventMetaType::AbstractEventMetaType(const std::type_info &typeInfo, const char *className,
                                             AbstractEventMetaType *nearestBase,
                                             TypeIds matrixTypeIds)
    : typeInfo(typeInfo)
    , className(className)
    , baseType(nearestBase)
    , matrixIds(std::move(matrixTypeIds))
{
    QUO_CHECK(!matrixIds.front().isEmpty());
    if (nearestBase) {
        if (const auto [priorMetaType, overlappingId] =
                findFirstOverlap(nearestBase->derivedTypes(),
                                 filter_view(matrixIds, std::not_fn(&QLatin1String::isEmpty)));
            priorMetaType) {
            if (QUO_ALARM_X(priorMetaType == this, "Attempt to re-register the same event class"))
                return; // This is kinda fine but extremely fishy

            // Two different metatype objects claim the same Matrix type id; this
            // is not normal, so give as much information as possible to diagnose
            if (QUO_ALARM_X(priorMetaType->typeInfo == typeInfo,
                            QLatin1StringView(className) % " claims '"_L1 % overlappingId
                                % "' repeatedly; check that the C++ symbol is properly exported"_L1))
                return; // That situation is very wrong (see #413) so maybe std::terminate() even?

            qWarning(EVENTS).nospace() << overlappingId << " is already mapped to "
                                       << priorMetaType->className << " before " << className
                                       << "; unless the two have different isValid() conditions, "
                                          "the latter class will never be used";
        }
        nearestBase->_derivedTypes.emplace_back(this);
    }
    auto dbg = qDebug(EVENTS).nospace();
    dbg << matrixIds.front();
    if (!matrixIds.back().isEmpty())
        dbg << ", " << matrixIds.back();
    dbg << " -> " << className << "; " << nearestBase->_derivedTypes.size()
        << " type(s) derived from " << nearestBase->className;
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

void Event::dumpTo(QDebug dbg) const
{
    dbg << QJsonDocument(contentJson()).toJson(QJsonDocument::Compact);
}
