// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"

namespace Quotient {

constexpr inline auto SasV1Method = "m.sas.v1"_L1;

// Same story as with EncryptedEvent: because KeyVerificationEvent inheritors can be sent both
// in-room and out-of-room, and RoomEvent doesn't restrict the shape of the event, rather just
// adds accessors, KeyVerificationEvent is derived from RoomEvent but when used as a to-device
// event room-specific attributes will be empty.
class QUOTIENT_API KeyVerificationEvent : public RoomEvent {
public:
    QUO_BASE_EVENT(KeyVerificationEvent, RoomEvent, "m.key.*")

    using RoomEvent::RoomEvent;

    /// An opaque identifier for the verification request. Must
    /// be unique with respect to the devices involved.
    QUO_CONTENT_GETTER(QString, transactionId)
};

/// Requests a key verification with another user's devices.
class QUOTIENT_API KeyVerificationRequestEvent : public KeyVerificationEvent {
public:
    QUO_EVENT(KeyVerificationRequestEvent, "m.key.verification.request")

    using KeyVerificationEvent::KeyVerificationEvent;
    KeyVerificationRequestEvent(const QString& transactionId,
                                const QString& fromDevice,
                                const QStringList& methods,
                                const QDateTime& timestamp)
        : KeyVerificationRequestEvent(
            basicJson(TypeId, { { "transaction_id"_L1, transactionId },
                                { "from_device"_L1, fromDevice },
                                { "methods"_L1, toJson(methods) },
                                { "timestamp"_L1, toJson(timestamp) } }))
    {}

    /// The device ID which is initiating the request.
    QUO_CONTENT_GETTER(QString, fromDevice)

    /// The verification methods supported by the sender.
    QUO_CONTENT_GETTER(QStringList, methods)

    /// The POSIX timestamp in milliseconds for when the request was
    /// made. If the request is in the future by more than 5 minutes or
    /// more than 10 minutes in the past, the message should be ignored
    /// by the receiver.
    QUO_CONTENT_GETTER(QDateTime, timestamp)
};

class QUOTIENT_API KeyVerificationReadyEvent : public KeyVerificationEvent {
public:
    QUO_EVENT(KeyVerificationReadyEvent, "m.key.verification.ready")

    using KeyVerificationEvent::KeyVerificationEvent;
    KeyVerificationReadyEvent(const QString& transactionId,
                              const QString& fromDevice,
                              const QStringList& methods)
        : KeyVerificationReadyEvent(
            basicJson(TypeId, { { "transaction_id"_L1, transactionId },
                                { "from_device"_L1, fromDevice },
                                { "methods"_L1, toJson(methods) } }))
    {}

    /// The device ID which is accepting the request.
    QUO_CONTENT_GETTER(QString, fromDevice)

    /// The verification methods supported by the sender.
    QUO_CONTENT_GETTER(QStringList, methods)
};

constexpr inline auto HmacSha256Code = "hkdf-hmac-sha256"_L1;
constexpr inline auto HmacSha256V2Code = "hkdf-hmac-sha256.v2"_L1;
constexpr std::array SupportedMacs { HmacSha256Code, HmacSha256V2Code };

/// Begins a key verification process.
class QUOTIENT_API KeyVerificationStartEvent : public KeyVerificationEvent {
public:
    QUO_EVENT(KeyVerificationStartEvent, "m.key.verification.start")

    using KeyVerificationEvent::KeyVerificationEvent;
    KeyVerificationStartEvent(const QString& transactionId,
                              const QString& fromDevice)
        : KeyVerificationStartEvent(
            basicJson(TypeId, { { "transaction_id"_L1, transactionId },
                                { "from_device"_L1, fromDevice },
                                { "method"_L1, SasV1Method },
                                { "hashes"_L1, QJsonArray{ "sha256"_L1 } },
                                { "key_agreement_protocols"_L1,
                                  QJsonArray{ "curve25519-hkdf-sha256"_L1 } },
                                { "message_authentication_codes"_L1,
                                  toJson(SupportedMacs) },
                                { "short_authentication_string"_L1,
                                  QJsonArray{ "decimal"_L1, "emoji"_L1 } } }))
    {}

    /// The device ID which is initiating the process.
    QUO_CONTENT_GETTER(QString, fromDevice)

    /// The verification method to use.
    QUO_CONTENT_GETTER(QString, method)

    /// Optional method to use to verify the other user's key with.
    QUO_CONTENT_GETTER(std::optional<QString>, nextMethod)

    // SAS.V1 methods

    /// The key agreement protocols the sending device understands.
    /// \note Only exist if method is m.sas.v1
    QStringList keyAgreementProtocols() const
    {
        Q_ASSERT(method() == SasV1Method);
        return contentPart<QStringList>("key_agreement_protocols"_L1);
    }

    /// The hash methods the sending device understands.
    /// \note Only exist if method is m.sas.v1
    QStringList hashes() const
    {
        Q_ASSERT(method() == SasV1Method);
        return contentPart<QStringList>("hashes"_L1);
    }

    /// The message authentication codes that the sending device understands.
    /// \note Only exist if method is m.sas.v1
    QStringList messageAuthenticationCodes() const
    {
        Q_ASSERT(method() == SasV1Method);
        return contentPart<QStringList>("message_authentication_codes"_L1);
    }

    /// The SAS methods the sending device (and the sending device's
    /// user) understands.
    /// \note Only exist if method is m.sas.v1
    QString shortAuthenticationString() const
    {
        Q_ASSERT(method() == SasV1Method);
        return contentPart<QString>("short_authentication_string"_L1);
    }
};

/// Accepts a previously sent m.key.verification.start message.
class QUOTIENT_API KeyVerificationAcceptEvent : public KeyVerificationEvent {
public:
    QUO_EVENT(KeyVerificationAcceptEvent, "m.key.verification.accept")

    using KeyVerificationEvent::KeyVerificationEvent;
    KeyVerificationAcceptEvent(const QString& transactionId,
                               const QString& commitment)
        : KeyVerificationAcceptEvent(basicJson(
            TypeId, { { "transaction_id"_L1, transactionId },
                      { "method"_L1, SasV1Method },
                      { "key_agreement_protocol"_L1, "curve25519-hkdf-sha256"_L1 },
                      { "hash"_L1, "sha256"_L1 },
                      { "message_authentication_code"_L1, HmacSha256V2Code },
                      { "short_authentication_string"_L1,
                        QJsonArray{ "decimal"_L1, "emoji"_L1, } },
                      { "commitment"_L1, commitment } }))
    {}

    /// The verification method to use. Must be 'm.sas.v1'.
    QUO_CONTENT_GETTER(QString, method)

    /// The key agreement protocol the device is choosing to use, out of
    /// the options in the m.key.verification.start message.
    QUO_CONTENT_GETTER(QString, keyAgreementProtocol)

    /// The hash method the device is choosing to use, out of the
    /// options in the m.key.verification.start message.
    QUO_CONTENT_GETTER_X(QString, hashData, "hash"_L1)

    /// The message authentication code the device is choosing to use, out
    /// of the options in the m.key.verification.start message.
    QUO_CONTENT_GETTER(QString, messageAuthenticationCode)

    /// The SAS methods both devices involved in the verification process understand.
    QUO_CONTENT_GETTER(QStringList, shortAuthenticationString)

    /// The hash (encoded as unpadded base64) of the concatenation of the
    /// device's ephemeral public key (encoded as unpadded base64) and the
    /// canonical JSON representation of the m.key.verification.start message.
    QUO_CONTENT_GETTER(QString, commitment)
};

class QUOTIENT_API KeyVerificationCancelEvent : public KeyVerificationEvent {
public:
    QUO_EVENT(KeyVerificationCancelEvent, "m.key.verification.cancel")

    using KeyVerificationEvent::KeyVerificationEvent;
    KeyVerificationCancelEvent(const QString& transactionId,
                               const QString& reason)
        : KeyVerificationCancelEvent(
            basicJson(TypeId, {
                                  { "transaction_id"_L1, transactionId },
                                  { "reason"_L1, reason },
                                  { "code"_L1, reason } // Not a typo
                              }))
    {}

    /// A human readable description of the code. The client should only
    /// rely on this string if it does not understand the code.
    QUO_CONTENT_GETTER(QString, reason)

    /// The error code for why the process/request was cancelled by the user.
    QUO_CONTENT_GETTER(QString, code)
};

/// Sends the ephemeral public key for a device to the partner device.
class QUOTIENT_API KeyVerificationKeyEvent : public KeyVerificationEvent {
public:
    QUO_EVENT(KeyVerificationKeyEvent, "m.key.verification.key")

    using KeyVerificationEvent::KeyVerificationEvent;
    KeyVerificationKeyEvent(const QString& transactionId, const QString& key)
        : KeyVerificationKeyEvent(
            basicJson(TypeId, { { "transaction_id"_L1, transactionId },
                                { "key"_L1, key } }))
    {}

    /// The device's ephemeral public key, encoded as unpadded base64.
    QUO_CONTENT_GETTER(QString, key)
};

/// Sends the MAC of a device's key to the partner device.
class QUOTIENT_API KeyVerificationMacEvent : public KeyVerificationEvent {
public:
    QUO_EVENT(KeyVerificationMacEvent, "m.key.verification.mac")

    using KeyVerificationEvent::KeyVerificationEvent;
    KeyVerificationMacEvent(const QString& transactionId, const QString& keys,
                            const QJsonObject& mac)
        : KeyVerificationMacEvent(
            basicJson(TypeId, { { "transaction_id"_L1, transactionId },
                                { "keys"_L1, keys },
                                { "mac"_L1, mac } }))
    {}

    /// The device's ephemeral public key, encoded as unpadded base64.
    QUO_CONTENT_GETTER(QString, keys)

    QHash<QString, QString> mac() const
    {
        return contentPart<QHash<QString, QString>>("mac"_L1);
    }
};

class QUOTIENT_API KeyVerificationDoneEvent : public KeyVerificationEvent {
public:
    QUO_EVENT(KeyVerificationDoneEvent, "m.key.verification.done")

    using KeyVerificationEvent::KeyVerificationEvent;
    explicit KeyVerificationDoneEvent(const QString& transactionId)
        : KeyVerificationDoneEvent(
            basicJson(TypeId, { { "transaction_id"_L1, transactionId } }))
    {}
};
} // namespace Quotient
