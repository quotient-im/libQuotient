// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "encryptedevent.h"
#include "logging.h"

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

QString EncryptedEvent::algorithm() const
{
    const auto algo = content<QString>(AlgorithmKeyL);
    if (!SupportedAlgorithms.contains(algo)) {
        qCWarning(MAIN) << "The EncryptedEvent's algorithm" << algo
                       << "is not supported";
    }
    return algo;
}
