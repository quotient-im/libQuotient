// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

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
    Q_ENUM(EncryptionType)

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
};

REGISTER_EVENT_TYPE(EncryptionEvent)
} // namespace Quotient
