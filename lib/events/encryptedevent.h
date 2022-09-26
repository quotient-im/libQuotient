// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"

namespace Quotient {

constexpr auto CiphertextKeyL = "ciphertext"_ls;
constexpr auto SenderKeyKeyL = "sender_key"_ls;
constexpr auto DeviceIdKeyL = "device_id"_ls;
constexpr auto SessionIdKeyL = "session_id"_ls;

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
class QUOTIENT_API EncryptedEvent : public RoomEvent {
public:
    QUO_EVENT(EncryptedEvent, "m.room.encrypted")

    /* In case with Olm, the encrypted content of the event is
     * a map from the recipient Curve25519 identity key to ciphertext
     * information */
    explicit EncryptedEvent(const QJsonObject& ciphertexts,
                            const QString& senderKey);
    /* In case with Megolm, device_id and session_id are required */
    explicit EncryptedEvent(const QByteArray& ciphertext,
                            const QString& senderKey, const QString& deviceId,
                            const QString& sessionId);
    explicit EncryptedEvent(const QJsonObject& obj);

    QString algorithm() const;
    QByteArray ciphertext() const
    {
        return contentPart<QString>(CiphertextKeyL).toLatin1();
    }
    QJsonObject ciphertext(const QString& identityKey) const
    {
        return contentPart<QJsonObject>(CiphertextKeyL)
            .value(identityKey)
            .toObject();
    }
    QString senderKey() const { return contentPart<QString>(SenderKeyKeyL); }

    /* device_id and session_id are required with Megolm */
    QString deviceId() const { return contentPart<QString>(DeviceIdKeyL); }
    QString sessionId() const { return contentPart<QString>(SessionIdKeyL); }
    RoomEventPtr createDecrypted(const QString &decrypted) const;

    void setRelation(const QJsonObject& relation);
};
} // namespace Quotient
