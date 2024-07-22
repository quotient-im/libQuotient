// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Quotient/quotient_common.h>
#include "stateevent.h"

namespace Quotient {
class QUOTIENT_API EncryptionEventContent {
public:
    Q_IMPLICIT EncryptionEventContent(Quotient::EncryptionType et);
    explicit EncryptionEventContent(const QJsonObject& json);

    QJsonObject toJson() const;

    Quotient::EncryptionType encryption;
    QString algorithm {};
    int rotationPeriodMs = 604'800'000;
    int rotationPeriodMsgs = 100;
};

class QUOTIENT_API EncryptionEvent
    : public KeylessStateEventBase<EncryptionEvent, EncryptionEventContent> {
public:
    QUO_EVENT(EncryptionEvent, "m.room.encryption")

    using KeylessStateEventBase::KeylessStateEventBase;

    Quotient::EncryptionType encryption() const { return content().encryption; }
    QString algorithm() const { return content().algorithm; }
    int rotationPeriodMs() const { return content().rotationPeriodMs; }
    int rotationPeriodMsgs() const { return content().rotationPeriodMsgs; }

    bool useEncryption() const { return !algorithm().isEmpty(); }
};
} // namespace Quotient
