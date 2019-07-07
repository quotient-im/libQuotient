//
// Created by rusakov on 26/09/2017.
// Contributed by andreev on 27/06/2019.
//

#include "encryptionevent.h"

#include "converters.h"
#include "logging.h"

#include <array>

static const std::array<QString, 1> encryptionStrings = { {
    QStringLiteral("m.megolm.v1.aes-sha2")
} };

namespace QMatrixClient {
    template <>
    struct JsonConverter<EncryptionType>
    {
        static EncryptionType load(const QJsonValue& jv)
        {
            const auto& encryptionString = jv.toString();
            for (auto it = encryptionStrings.begin();
                    it != encryptionStrings.end(); ++it)
                if (encryptionString == *it)
                    return EncryptionType(it - encryptionStrings.begin());

            qCWarning(EVENTS) << "Unknown EncryptionType: " << encryptionString;
            return EncryptionType::Undefined;
        }
    };
}

using namespace QMatrixClient;

EncryptionEventContent::EncryptionEventContent(const QJsonObject& json)
    : encryption(fromJson<EncryptionType>(json["algorithm"_ls]))
    , algorithm(sanitized(json["algorithm"_ls].toString()))
    , rotationPeriodMs(json["rotation_period_ms"_ls].toInt(604800000))
    , rotationPeriodMsgs(json["rotation_period_msgs"_ls].toInt(100))
{ }

void EncryptionEventContent::fillJson(QJsonObject* o) const
{
    Q_ASSERT(o);
    Q_ASSERT_X(encryption != EncryptionType::Undefined, __FUNCTION__,
             "The key 'algorithm' must be explicit in EncryptionEventContent");
    if (encryption != EncryptionType::Undefined)
        o->insert(QStringLiteral("algorithm"), algorithm);
    o->insert(QStringLiteral("rotation_period_ms"), rotationPeriodMs);
    o->insert(QStringLiteral("rotation_period_msgs"), rotationPeriodMsgs);
}
