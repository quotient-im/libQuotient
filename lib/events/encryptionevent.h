// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "quotient_common.h"
#include "stateevent.h"

namespace Quotient {
class QUOTIENT_API EncryptionEventContent {
public:
    using EncryptionType
        [[deprecated("Use Quotient::EncryptionType instead")]] =
            Quotient::EncryptionType;

    // NOLINTNEXTLINE(google-explicit-constructor)
    QUO_IMPLICIT EncryptionEventContent(Quotient::EncryptionType et);
    explicit EncryptionEventContent(const QJsonObject& json);

    QJsonObject toJson() const;

    Quotient::EncryptionType encryption;
    QString algorithm {};
    int rotationPeriodMs = 604'800'000;
    int rotationPeriodMsgs = 100;
};

class QUOTIENT_API EncryptionEvent : public StateEvent<EncryptionEventContent> {
public:
    DEFINE_EVENT_TYPEID("m.room.encryption", EncryptionEvent)

    using EncryptionType
        [[deprecated("Use Quotient::EncryptionType instead")]] =
            Quotient::EncryptionType;

    explicit EncryptionEvent(const QJsonObject& obj)
        : StateEvent(typeId(), obj)
    {}
    explicit EncryptionEvent(EncryptionEventContent&& content)
        : StateEvent(typeId(), matrixTypeId(), QString(), std::move(content))
    {}

    Quotient::EncryptionType encryption() const { return content().encryption; }
    QString algorithm() const { return content().algorithm; }
    int rotationPeriodMs() const { return content().rotationPeriodMs; }
    int rotationPeriodMsgs() const { return content().rotationPeriodMsgs; }

    bool useEncryption() const { return !algorithm().isEmpty(); }
};
REGISTER_EVENT_TYPE(EncryptionEvent)
} // namespace Quotient
