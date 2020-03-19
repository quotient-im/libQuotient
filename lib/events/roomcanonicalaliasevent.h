/******************************************************************************
 * Copyright (C) 2020 QMatrixClient project
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
 
#pragma once

#include "stateevent.h"

namespace Quotient {
namespace EventContent{
    class AliasesEventContent {

    public:

        template<typename T1, typename T2>
        AliasesEventContent(T1&& canonicalAlias, T2&& altAliases)
            : canonicalAlias(std::forward<T1>(canonicalAlias))
            , altAliases(std::forward<T2>(altAliases))
        { }

        AliasesEventContent(const QJsonObject& json)
            : canonicalAlias(fromJson<QString>(json["alias"]))
            , altAliases(fromJson<QStringList>(json["alt_aliases"]))
        { }

        QJsonObject toJson() const
        {
            return { { "alias", Quotient::toJson(canonicalAlias) },
                     { "alt_aliases", Quotient::toJson(altAliases) } };
        }

        QString canonicalAlias;
        QStringList altAliases;
    };
} // namespace EventContent

class RoomCanonicalAliasEvent
    : public StateEvent<EventContent::AliasesEventContent> {
public:
    DEFINE_EVENT_TYPEID("m.room.canonical_alias", RoomCanonicalAliasEvent)

    explicit RoomCanonicalAliasEvent(const QJsonObject& obj)
        : StateEvent(typeId(), obj)
    { }

    RoomCanonicalAliasEvent(const QString& canonicalAlias, const QStringList& altAliases = {})
        : StateEvent(typeId(), matrixTypeId(), QString(),
                canonicalAlias, altAliases)
    { }

    RoomCanonicalAliasEvent(QString&& canonicalAlias, QStringList&& altAliases = {})
        : StateEvent(typeId(), matrixTypeId(), QString(),
                std::move(canonicalAlias), std::move(altAliases))
    { }

    QString alias() const { return content().canonicalAlias; }

    QStringList altAliases() const { return content().altAliases; }
};
REGISTER_EVENT_TYPE(RoomCanonicalAliasEvent)
} // namespace Quotient
