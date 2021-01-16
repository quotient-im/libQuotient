// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "encryptedevent.h"

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
