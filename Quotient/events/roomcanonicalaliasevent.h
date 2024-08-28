// SPDX-FileCopyrightText: 2020 Ram Nad <ramnad1999@gmail.com>
// SPDX-FileCopyrightText: 2020 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later
 
#pragma once

#include "stateevent.h"

namespace Quotient {
namespace EventContent {
    struct AliasesEventContent {
        QString canonicalAlias;
        QStringList altAliases;
    };
} // namespace EventContent

template<>
inline EventContent::AliasesEventContent fromJson(const QJsonObject& jo)
{
    return EventContent::AliasesEventContent {
        fromJson<QString>(jo["alias"_L1]),
        fromJson<QStringList>(jo["alt_aliases"_L1])
    };
}
template<>
inline auto toJson(const EventContent::AliasesEventContent& c)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, "alias"_L1, c.canonicalAlias);
    addParam<IfNotEmpty>(jo, "alt_aliases"_L1, c.altAliases);
    return jo;
}

class QUOTIENT_API RoomCanonicalAliasEvent
    : public KeylessStateEventBase<RoomCanonicalAliasEvent,
                                   EventContent::AliasesEventContent> {
public:
    QUO_EVENT(RoomCanonicalAliasEvent, "m.room.canonical_alias")
    using KeylessStateEventBase::KeylessStateEventBase;

    QString alias() const { return content().canonicalAlias; }
    QStringList altAliases() const { return content().altAliases; }
};
} // namespace Quotient
