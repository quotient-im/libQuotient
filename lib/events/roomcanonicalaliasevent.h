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
    template <typename T1, typename T2>
    class SimpleDualContent {
    public:
        using first_value_type = T1;
        using second_value_type = T2;

        template <typename TT1, typename TT2>
        SimpleDualContent(QString Key1Name, TT1&& value1, QString Key2Name,
                          TT2&& value2)
            : value1(std::forward<TT1>(value1))
            , value2(std::forward<TT2>(value2))
            , key1(std::move(Key1Name))
            , key2(std::move(Key2Name))
        { }

        SimpleDualContent(const QJsonObject& json, QString key1Name,
                          QString key2Name)
            : value1(fromJson<T1>(json[key1Name]))
            , value2(fromJson<T2>(json[key2Name]))
            , key1(std::move(key1Name))
            , key2(std::move(key2Name))
        { }

        QJsonObject toJson() const
        {
            return { { key1, Quotient::toJson(value1) },
                     { key2, Quotient::toJson(value2) } };
        }

    public:
        T1 value1;
        T2 value2;

    protected:
        QString key1;
        QString key2;
    };
} // namespace EventContent

class RoomCanonicalAliasEvent
    : public StateEvent<EventContent::SimpleDualContent<QString, QStringList>> {
public:
    DEFINE_EVENT_TYPEID("m.room.canonical_alias", RoomCanonicalAliasEvent)

    explicit RoomCanonicalAliasEvent(const QJsonObject& obj)
        : StateEvent(typeId(), obj, QStringLiteral("alias"),
                     QStringLiteral("alt_aliases"))
    { }

    RoomCanonicalAliasEvent(const QString& server, const QString& alias,
                            const QStringList& alt_aliases)
        : StateEvent(typeId(), matrixTypeId(), server, QStringLiteral("alias"),
                     alias, QStringLiteral("alt_aliases"), alt_aliases)
    { }

    // For compatibility used at Room::setCanonicalAlias
    explicit RoomCanonicalAliasEvent(const QString& value1)
        : RoomCanonicalAliasEvent(value1, QStringList())
    { }

    // Because, MSC2432 specifies, that alt_aliases may be present
    // without aliases as well
    explicit RoomCanonicalAliasEvent(const QStringList& value2)
        : RoomCanonicalAliasEvent(QString(), value2)
    { }

    template <typename T1, typename T2>
    RoomCanonicalAliasEvent(T1&& value1, T2&& value2)
        : StateEvent(typeId(), matrixTypeId(), QString(),
                     QStringLiteral("alias"), std::forward<T1>(value1),
                     QStringLiteral("alt_aliases"), std::forward<T2>(value2))
    { }

    QString alias() const { return content().value1; }

    QStringList alt_aliases() const { return content().value2; }
};
REGISTER_EVENT_TYPE(RoomCanonicalAliasEvent)
} // namespace Quotient
