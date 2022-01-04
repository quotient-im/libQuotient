// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "eventcontent.h"
#include "stateevent.h"
#include "quotient_common.h"

namespace Quotient {
class QUOTIENT_API EncryptionEventContent : public EventContent::Base {
public:
    enum EncryptionType : size_t { MegolmV1AesSha2 = 0, Undefined };

    QUO_IMPLICIT EncryptionEventContent(EncryptionType et);
    [[deprecated("This constructor will require explicit EncryptionType soon")]] //
    explicit EncryptionEventContent()
        : EncryptionEventContent(Undefined)
    {}
    explicit EncryptionEventContent(const QJsonObject& json);

    EncryptionType encryption;
    QString algorithm;
    int rotationPeriodMs;
    int rotationPeriodMsgs;

protected:
    void fillJson(QJsonObject& o) const override;
};

using EncryptionType = EncryptionEventContent::EncryptionType;

class QUOTIENT_API EncryptionEvent : public StateEvent<EncryptionEventContent> {
    Q_GADGET
public:
    DEFINE_EVENT_TYPEID("m.room.encryption", EncryptionEvent)

    using EncryptionType = EncryptionEventContent::EncryptionType;
    Q_ENUM(EncryptionType)

    explicit EncryptionEvent(const QJsonObject& obj)
        : StateEvent(typeId(), obj)
    {}
    [[deprecated("This constructor will require an explicit parameter soon")]] //
//    explicit EncryptionEvent()
//        : EncryptionEvent(QJsonObject())
//    {}
    explicit EncryptionEvent(EncryptionEventContent&& content)
        : StateEvent(typeId(), matrixTypeId(), QString(), std::move(content))
    {}

    EncryptionType encryption() const { return content().encryption; }

    QString algorithm() const { return content().algorithm; }
    int rotationPeriodMs() const { return content().rotationPeriodMs; }
    int rotationPeriodMsgs() const { return content().rotationPeriodMsgs; }
};
REGISTER_EVENT_TYPE(EncryptionEvent)
} // namespace Quotient
