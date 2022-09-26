// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "encryptedevent.h"
#include "e2ee/e2ee.h"
#include "logging.h"

using namespace Quotient;

EncryptedEvent::EncryptedEvent(const QJsonObject& ciphertexts,
                               const QString& senderKey)
    : RoomEvent({ { AlgorithmKeyL, OlmV1Curve25519AesSha2AlgoKey },
                  { CiphertextKeyL, ciphertexts },
                  { SenderKeyKeyL, senderKey } })
{}

EncryptedEvent::EncryptedEvent(const QByteArray& ciphertext,
                               const QString& senderKey,
                               const QString& deviceId, const QString& sessionId)
    : RoomEvent({
        { AlgorithmKeyL, MegolmV1AesSha2AlgoKey },
        { CiphertextKeyL, QString(ciphertext) },
        { DeviceIdKeyL, deviceId },
        { SenderKeyKeyL, senderKey },
        { SessionIdKeyL, sessionId },
    })
{}

EncryptedEvent::EncryptedEvent(const QJsonObject& obj)
    : RoomEvent(obj)
{
    qCDebug(E2EE) << "Encrypted event from" << senderId();
}

QString EncryptedEvent::algorithm() const
{
    const auto algo = contentPart<QString>(AlgorithmKeyL);
    if (!isSupportedAlgorithm(algo))
        qWarning(MAIN) << "The EncryptedEvent's algorithm" << algo
                       << "is not supported";

    return algo;
}

RoomEventPtr EncryptedEvent::createDecrypted(const QString &decrypted) const
{
    auto eventObject = QJsonDocument::fromJson(decrypted.toUtf8()).object();
    eventObject["event_id"] = id();
    eventObject["sender"] = senderId();
    eventObject["origin_server_ts"] = originTimestamp().toMSecsSinceEpoch();
    if (const auto relatesToJson = contentPart<QJsonObject>("m.relates_to"_ls);
        !relatesToJson.isEmpty()) {
        auto content = eventObject["content"].toObject();
        content["m.relates_to"] = relatesToJson;
        eventObject["content"] = content;
    }
    if (const auto redactsJson = unsignedPart<QString>("redacts"_ls);
        !redactsJson.isEmpty()) {
        auto unsign = eventObject["unsigned"].toObject();
        unsign["redacts"] = redactsJson;
        eventObject["unsigned"] = unsign;
    }
    return loadEvent<RoomEvent>(eventObject);
}

void EncryptedEvent::setRelation(const QJsonObject& relation)
{
    auto content = contentJson();
    content["m.relates_to"] = relation;
    editJson()["content"] = content;
}
