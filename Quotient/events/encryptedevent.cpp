// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "encryptedevent.h"
#include <Quotient/e2ee/e2ee_common.h>

using namespace Quotient;

EncryptedEvent::EncryptedEvent(const QJsonObject& ciphertexts,
                               const QString& senderKey)
    : RoomEvent(basicJson(TypeId, { { AlgorithmKeyL, OlmV1Curve25519AesSha2AlgoKey },
                                    { CiphertextKey, ciphertexts },
                                    { SenderKeyKey, senderKey } }))
{}

EncryptedEvent::EncryptedEvent(const QByteArray& ciphertext,
                               const QString& senderKey,
                               const QString& deviceId, const QString& sessionId)
    : RoomEvent(basicJson(TypeId, { { AlgorithmKeyL, MegolmV1AesSha2AlgoKey },
                                    { CiphertextKey, QString::fromLatin1(ciphertext) },
                                    { DeviceIdKey, deviceId },
                                    { SenderKeyKey, senderKey },
                                    { SessionIdKey, sessionId } }))
{}

EncryptedEvent::EncryptedEvent(const QJsonObject& obj) : RoomEvent(obj) {}

QString EncryptedEvent::algorithm() const
{
    return contentPart<QString>(AlgorithmKeyL);
}

RoomEventPtr EncryptedEvent::createDecrypted(const QString &decrypted) const
{
    auto eventObject = QJsonDocument::fromJson(decrypted.toUtf8()).object();
    eventObject["event_id"_ls] = id();
    eventObject["sender"_ls] = senderId();
    eventObject["origin_server_ts"_ls] = originTimestamp().toMSecsSinceEpoch();
    if (const auto relatesToJson = contentPart<QJsonObject>(RelatesToKey);
        !relatesToJson.isEmpty()) {
        replaceSubvalue(eventObject, ContentKey, RelatesToKey, relatesToJson);
    }
    if (const auto redactsJson = unsignedPart<QString>("redacts"_ls);
        !redactsJson.isEmpty()) {
        replaceSubvalue(eventObject, UnsignedKey, "redacts"_ls, redactsJson);
    }
    return loadEvent<RoomEvent>(eventObject);
}

void EncryptedEvent::setRelation(const QJsonObject& relation)
{
    replaceSubvalue(editJson(), ContentKey, RelatesToKey, relation);
}
