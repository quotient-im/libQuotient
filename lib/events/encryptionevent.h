/******************************************************************************
 * Copyright (C) 2017 Kitsune Ral <kitsune-ral@users.sf.net>
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

#include "eventcontent.h"
#include "stateevent.h"

namespace Quotient {
class EncryptionEventContent : public EventContent::Base {
public:
    enum EncryptionType : size_t { MegolmV1AesSha2 = 0, Undefined };

    explicit EncryptionEventContent(EncryptionType et = Undefined)
        : encryption(et)
    {}
    explicit EncryptionEventContent(const QJsonObject& json);

    EncryptionType encryption;
    QString algorithm;
    int rotationPeriodMs;
    int rotationPeriodMsgs;

protected:
    void fillJson(QJsonObject* o) const override;
};

using EncryptionType = EncryptionEventContent::EncryptionType;

class EncryptionEvent : public StateEvent<EncryptionEventContent> {
    Q_GADGET
public:
    DEFINE_EVENT_TYPEID("m.room.encryption", EncryptionEvent)

    using EncryptionType = EncryptionEventContent::EncryptionType;

    explicit EncryptionEvent(const QJsonObject& obj = {}) // TODO: apropriate
                                                          // default value
        : StateEvent(typeId(), obj)
    {}
    template <typename... ArgTs>
    EncryptionEvent(ArgTs&&... contentArgs)
        : StateEvent(typeId(), matrixTypeId(), QString(),
                     std::forward<ArgTs>(contentArgs)...)
    {}

    EncryptionType encryption() const { return content().encryption; }

    QString algorithm() const { return content().algorithm; }
    int rotationPeriodMs() const { return content().rotationPeriodMs; }
    int rotationPeriodMsgs() const { return content().rotationPeriodMsgs; }

private:
    Q_ENUM(EncryptionType)
};

REGISTER_EVENT_TYPE(EncryptionEvent)
} // namespace Quotient
