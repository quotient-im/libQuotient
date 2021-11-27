// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "encryptedevent.h"
#include "roommessageevent.h"

using namespace Quotient;

EncryptedEvent::EncryptedEvent(const QJsonObject& ciphertext,
                               const QString& senderKey)
    : RoomEvent(typeId(), matrixTypeId(),
                { { AlgorithmKeyL, OlmV1Curve25519AesSha2AlgoKey },
                  { CiphertextKeyL, ciphertext },
                  { SenderKeyKeyL, senderKey } })
{}

EncryptedEvent::EncryptedEvent(QByteArray ciphertext, const QString& senderKey,
                               const QString& deviceId, const QString& sessionId)
    : RoomEvent(typeId(), matrixTypeId(),
                {
                    { AlgorithmKeyL, MegolmV1AesSha2AlgoKey },
                    { CiphertextKeyL, QString(ciphertext) },
                    { DeviceIdKeyL, deviceId },
                    { SenderKeyKeyL, senderKey },
                    { SessionIdKeyL, sessionId },
                })
{}

EncryptedEvent::EncryptedEvent(const QJsonObject& obj)
    : RoomEvent(typeId(), obj)
{
    qCDebug(E2EE) << "Encrypted event from" << senderId();
}

RoomEventPtr EncryptedEvent::createDecrypted(const QString &decrypted) const
{
    auto eventObject = QJsonDocument::fromJson(decrypted.toUtf8()).object();
    eventObject["event_id"] = id();
    eventObject["sender"] = senderId();
    eventObject["origin_server_ts"] = originTimestamp().toMSecsSinceEpoch();
    if(contentJson().contains("m.relates_to")) {
        auto relates = contentJson()["m.relates_to"].toObject();
        auto content = eventObject["content"].toObject();
        content["m.relates_to"] = relates;
        eventObject["content"] = content;
    }
    if(unsignedJson().contains("redacts")) {
        auto redacts = unsignedJson()["redacts"].toString();
        auto unsign = eventObject["unsigned"].toObject();
        unsign["redacts"] = redacts;
        eventObject["unsigned"] = unsign;
    }
    return makeEvent<RoomMessageEvent>(eventObject);
}
