// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "encryptionevent.h"
#include "logging.h"

#include "e2ee/e2ee.h"

using namespace Quotient;

static constexpr std::array encryptionStrings { MegolmV1AesSha2AlgoKey };

template <>
EncryptionType Quotient::fromJson(const QJsonValue& jv)
{
    const auto& encryptionString = jv.toString();
    for (auto it = encryptionStrings.begin(); it != encryptionStrings.end();
         ++it)
        if (encryptionString == *it)
            return EncryptionType(it - encryptionStrings.begin());

    if (!encryptionString.isEmpty())
        qCWarning(EVENTS) << "Unknown EncryptionType: " << encryptionString;
    return EncryptionType::Undefined;
}

EncryptionEventContent::EncryptionEventContent(const QJsonObject& json)
    : encryption(fromJson<Quotient::EncryptionType>(json[AlgorithmKeyL]))
    , algorithm(sanitized(json[AlgorithmKeyL].toString()))
{
    // NB: fillFromJson only fills the variable if the JSON key exists
    fillFromJson<int>(json[RotationPeriodMsKeyL], rotationPeriodMs);
    fillFromJson<int>(json[RotationPeriodMsgsKeyL], rotationPeriodMsgs);
}

EncryptionEventContent::EncryptionEventContent(Quotient::EncryptionType et)
    : encryption(et)
{
    if(encryption != Quotient::EncryptionType::Undefined) {
        algorithm = encryptionStrings[static_cast<size_t>(encryption)];
    }
}

QJsonObject EncryptionEventContent::toJson() const
{
    QJsonObject o;
    if (encryption != Quotient::EncryptionType::Undefined)
        o.insert(AlgorithmKey, algorithm);
    o.insert(RotationPeriodMsKey, rotationPeriodMs);
    o.insert(RotationPeriodMsgsKey, rotationPeriodMsgs);
    return o;
}
