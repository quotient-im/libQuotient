// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "event.h"

namespace Quotient {

constexpr inline auto SasV1Method = "m.sas.v1"_ls;

class QUOTIENT_API KeyVerificationEvent : public Event {
public:
    QUO_BASE_EVENT(KeyVerificationEvent, Event)

    using Event::Event;

    /// An opaque identifier for the verification request. Must
    /// be unique with respect to the devices involved.
    QUO_CONTENT_GETTER(QString, transactionId)
};

/// Requests a key verification with another user's devices.
/// Typically sent as a to-device event.
class QUOTIENT_API KeyVerificationRequestEvent : public KeyVerificationEvent {
public:
    QUO_EVENT(KeyVerificationRequestEvent, "m.key.verification.request")

    using KeyVerificationEvent::KeyVerificationEvent;
    KeyVerificationRequestEvent(const QString& transactionId,
                                const QString& fromDevice,
                                const QStringList& methods,
                                const QDateTime& timestamp)
        : KeyVerificationRequestEvent(
            basicJson(TypeId, { { "transaction_id"_ls, transactionId },
                                { "from_device"_ls, fromDevice },
                                { "methods"_ls, toJson(methods) },
                                { "timestamp"_ls, toJson(timestamp) } }))
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
            basicJson(TypeId, { { "transaction_id"_ls, transactionId },
                                { "from_device"_ls, fromDevice },
                                { "methods"_ls, toJson(methods) } }))
    {}

    /// The device ID which is accepting the request.
    QUO_CONTENT_GETTER(QString, fromDevice)

    /// The verification methods supported by the sender.
    QUO_CONTENT_GETTER(QStringList, methods)
};

constexpr inline auto HmacSha256Code = "hkdf-hmac-sha256"_ls;
constexpr inline auto HmacSha256V2Code = "hkdf-hmac-sha256.v2"_ls;
constexpr std::array SupportedMacs { HmacSha256Code, HmacSha256V2Code };

/// Begins a key verification process.
class QUOTIENT_API KeyVerificationStartEvent : public KeyVerificationEvent {
public:
    QUO_EVENT(KeyVerificationStartEvent, "m.key.verification.start")

    using KeyVerificationEvent::KeyVerificationEvent;
    KeyVerificationStartEvent(const QString& transactionId,
                              const QString& fromDevice)
        : KeyVerificationStartEvent(
            basicJson(TypeId, { { "transaction_id"_ls, transactionId },
                                { "from_device"_ls, fromDevice },
                                { "method"_ls, SasV1Method },
                                { "hashes"_ls, QJsonArray{ "sha256"_ls } },
                                { "key_agreement_protocols"_ls,
                                  QJsonArray{ "curve25519-hkdf-sha256"_ls } },
                                { "message_authentication_codes"_ls,
                                  toJson(SupportedMacs) },
                                { "short_authentication_string"_ls,
                                  QJsonArray{ "decimal"_ls, "emoji"_ls } } }))
    {}

    /// The device ID which is initiating the process.
    QUO_CONTENT_GETTER(QString, fromDevice)

    /// The verification method to use.
    QUO_CONTENT_GETTER(QString, method)

    /// Optional method to use to verify the other user's key with.
    QUO_CONTENT_GETTER(Omittable<QString>, nextMethod)

    // SAS.V1 methods

    /// The key agreement protocols the sending device understands.
    /// \note Only exist if method is m.sas.v1
    QStringList keyAgreementProtocols() const
    {
        Q_ASSERT(method() == SasV1Method);
        return contentPart<QStringList>("key_agreement_protocols"_ls);
    }

    /// The hash methods the sending device understands.
    /// \note Only exist if method is m.sas.v1
    QStringList hashes() const
    {
        Q_ASSERT(method() == SasV1Method);
        return contentPart<QStringList>("hashes"_ls);
    }

    /// The message authentication codes that the sending device understands.
    /// \note Only exist if method is m.sas.v1
    QStringList messageAuthenticationCodes() const
    {
        Q_ASSERT(method() == SasV1Method);
        return contentPart<QStringList>("message_authentication_codes"_ls);
    }

    /// The SAS methods the sending device (and the sending device's
    /// user) understands.
    /// \note Only exist if method is m.sas.v1
    QString shortAuthenticationString() const
    {
        Q_ASSERT(method() == SasV1Method);
        return contentPart<QString>("short_authentication_string"_ls);
    }
};

/// Accepts a previously sent m.key.verification.start message.
/// Typically sent as a to-device event.
class QUOTIENT_API KeyVerificationAcceptEvent : public KeyVerificationEvent {
public:
    QUO_EVENT(KeyVerificationAcceptEvent, "m.key.verification.accept")

    using KeyVerificationEvent::KeyVerificationEvent;
    KeyVerificationAcceptEvent(const QString& transactionId,
                               const QString& commitment)
        : KeyVerificationAcceptEvent(basicJson(
            TypeId, { { "transaction_id"_ls, transactionId },
                      { "method"_ls, SasV1Method },
                      { "key_agreement_protocol"_ls, "curve25519-hkdf-sha256"_ls },
                      { "hash"_ls, "sha256"_ls },
                      { "message_authentication_code"_ls, HmacSha256V2Code },
                      { "short_authentication_string"_ls,
                        QJsonArray{ "decimal"_ls, "emoji"_ls, } },
                      { "commitment"_ls, commitment } }))
    {}

    /// The verification method to use. Must be 'm.sas.v1'.
    QUO_CONTENT_GETTER(QString, method)

    /// The key agreement protocol the device is choosing to use, out of
    /// the options in the m.key.verification.start message.
    QUO_CONTENT_GETTER(QString, keyAgreementProtocol)

    /// The hash method the device is choosing to use, out of the
    /// options in the m.key.verification.start message.
    QUO_CONTENT_GETTER_X(QString, hashData, "hash"_ls)

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
                                  { "transaction_id"_ls, transactionId },
                                  { "reason"_ls, reason },
                                  { "code"_ls, reason } // Not a typo
                              }))
    {}

    /// A human readable description of the code. The client should only
    /// rely on this string if it does not understand the code.
    QUO_CONTENT_GETTER(QString, reason)

    /// The error code for why the process/request was cancelled by the user.
    QUO_CONTENT_GETTER(QString, code)
};

/// Sends the ephemeral public key for a device to the partner device.
/// Typically sent as a to-device event.
class QUOTIENT_API KeyVerificationKeyEvent : public KeyVerificationEvent {
public:
    QUO_EVENT(KeyVerificationKeyEvent, "m.key.verification.key")

    using KeyVerificationEvent::KeyVerificationEvent;
    KeyVerificationKeyEvent(const QString& transactionId, const QString& key)
        : KeyVerificationKeyEvent(
            basicJson(TypeId, { { "transaction_id"_ls, transactionId },
                                { "key"_ls, key } }))
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
            basicJson(TypeId, { { "transaction_id"_ls, transactionId },
                                { "keys"_ls, keys },
                                { "mac"_ls, mac } }))
    {}

    /// The device's ephemeral public key, encoded as unpadded base64.
    QUO_CONTENT_GETTER(QString, keys)

    QHash<QString, QString> mac() const
    {
        return contentPart<QHash<QString, QString>>("mac"_ls);
    }
};

class QUOTIENT_API KeyVerificationDoneEvent : public KeyVerificationEvent {
public:
    QUO_EVENT(KeyVerificationDoneEvent, "m.key.verification.done")

    using KeyVerificationEvent::KeyVerificationEvent;
    explicit KeyVerificationDoneEvent(const QString& transactionId)
        : KeyVerificationDoneEvent(
            basicJson(TypeId, { { "transaction_id"_ls, transactionId } }))
    {}
};
} // namespace Quotient
