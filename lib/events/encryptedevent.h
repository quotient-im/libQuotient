// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "crypto/e2ee.h"
#include "roomevent.h"

namespace Quotient {
class Room;
/*
 * While the specification states:
 *
 * "This event type is used when sending encrypted events.
 * It can be used either within a room
 * (in which case it will have all of the Room Event fields),
 * or as a to-device event."
 * "The encrypted payload can contain any message event."
 * https://matrix.org/docs/spec/client_server/latest#id493
 *
 * -- for most of the cases the message event is the room message event.
 * And even for the to-device events the context is for the room.
 *
 * So, to simplify integration to the timeline, EncryptedEvent is a RoomEvent
 * inheritor. Strictly speaking though, it's not always a RoomEvent, but an Event
 * in general. It's possible, because RoomEvent interface is similar to Event's
 * one and doesn't add new restrictions, just provides additional features.
 */
class EncryptedEvent : public RoomEvent {
    Q_GADGET
public:
    DEFINE_EVENT_TYPEID("m.room.encrypted", EncryptedEvent)

    /* In case with Olm, the encrypted content of the event is
     * a map from the recipient Curve25519 identity key to ciphertext
     * information */
    explicit EncryptedEvent(const QJsonObject& ciphertext,
                            const QString& senderKey);
    /* In case with Megolm, device_id and session_id are required */
    explicit EncryptedEvent(QByteArray ciphertext, const QString& senderKey,
                            const QString& deviceId, const QString& sessionId);
    explicit EncryptedEvent(const QJsonObject& obj);

    QString algorithm() const
    {
        QString algo = content<QString>(AlgorithmKeyL);
        if (!SupportedAlgorithms.contains(algo)) {
            qWarning(MAIN) << "The EncryptedEvent's algorithm" << algo
                           << "is not supported";
        }
        return algo;
    }
    QByteArray ciphertext() const
    {
        return content<QString>(CiphertextKeyL).toLatin1();
    }
    QJsonObject ciphertext(const QString& identityKey) const
    {
        return content<QJsonObject>(CiphertextKeyL).value(identityKey).toObject();
    }
    QString senderKey() const { return content<QString>(SenderKeyKeyL); }

    /* device_id and session_id are required with Megolm */
    QString deviceId() const { return content<QString>(DeviceIdKeyL); }
    QString sessionId() const { return content<QString>(SessionIdKeyL); }
};
REGISTER_EVENT_TYPE(EncryptedEvent)

} // namespace Quotient
