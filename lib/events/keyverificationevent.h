// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "event.h"

namespace Quotient {

/// Requests a key verification with another user's devices.
/// Typically sent as a to-device event.
class QUOTIENT_API KeyVerificationRequestEvent : public Event {
public:
    DEFINE_EVENT_TYPEID("m.key.verification.request", KeyVerificationRequestEvent)

    explicit KeyVerificationRequestEvent(const QJsonObject& obj);

    /// The device ID which is initiating the request.
    QUO_CONTENT_GETTER(QString, fromDevice)

    /// An opaque identifier for the verification request. Must
    /// be unique with respect to the devices involved.
    QUO_CONTENT_GETTER(QString, transactionId)

    /// The verification methods supported by the sender.
    QUO_CONTENT_GETTER(QStringList, methods)

    /// The POSIX timestamp in milliseconds for when the request was
    /// made. If the request is in the future by more than 5 minutes or
    /// more than 10 minutes in the past, the message should be ignored
    /// by the receiver.
    QUO_CONTENT_GETTER(uint64_t, timestamp)
};
REGISTER_EVENT_TYPE(KeyVerificationRequestEvent)

/// Begins a key verification process.
class QUOTIENT_API KeyVerificationStartEvent : public Event {
public:
    DEFINE_EVENT_TYPEID("m.key.verification.start", KeyVerificationStartEvent)

    explicit KeyVerificationStartEvent(const QJsonObject &obj);

    /// The device ID which is initiating the process.
    QUO_CONTENT_GETTER(QString, fromDevice)

    /// An opaque identifier for the verification request. Must
    /// be unique with respect to the devices involved.
    QUO_CONTENT_GETTER(QString, transactionId)

    /// The verification method to use.
    QUO_CONTENT_GETTER(QString, method)

    /// Optional method to use to verify the other user's key with.
    QUO_CONTENT_GETTER(Omittable<QString>, nextMethod)

    // SAS.V1 methods

    /// The key agreement protocols the sending device understands.
    /// \note Only exist if method is m.sas.v1
    QStringList keyAgreementProtocols() const
    {
        Q_ASSERT(method() == QStringLiteral("m.sas.v1"));
        return contentPart<QStringList>("key_agreement_protocols"_ls);
    }

    /// The hash methods the sending device understands.
    /// \note Only exist if method is m.sas.v1
    QStringList hashes() const
    {
        Q_ASSERT(method() == QStringLiteral("m.sas.v1"));
        return contentPart<QStringList>("hashes"_ls);
    }

    /// The message authentication codes that the sending device understands.
    /// \note Only exist if method is m.sas.v1
    QStringList messageAuthenticationCodes() const
    {
        Q_ASSERT(method() == QStringLiteral("m.sas.v1"));
        return contentPart<QStringList>("message_authentication_codes"_ls);
    }

    /// The SAS methods the sending device (and the sending device's
    /// user) understands.
    /// \note Only exist if method is m.sas.v1
    QString shortAuthenticationString() const
    {
        Q_ASSERT(method() == QStringLiteral("m.sas.v1"));
        return contentPart<QString>("short_authentification_string"_ls);
    }
};
REGISTER_EVENT_TYPE(KeyVerificationStartEvent)

/// Accepts a previously sent m.key.verification.start message.
/// Typically sent as a to-device event.
class QUOTIENT_API KeyVerificationAcceptEvent : public Event {
public:
    DEFINE_EVENT_TYPEID("m.key.verification.accept", KeyVerificationAcceptEvent)

    explicit KeyVerificationAcceptEvent(const QJsonObject& obj);

    /// An opaque identifier for the verification process.
    QUO_CONTENT_GETTER(QString, transactionId)

    /// The verification method to use. Must be 'm.sas.v1'.
    QUO_CONTENT_GETTER(QString, method)

    /// The key agreement protocol the device is choosing to use, out of
    /// the options in the m.key.verification.start message.
    QUO_CONTENT_GETTER(QString, keyAgreementProtocol)

    /// The hash method the device is choosing to use, out of the
    /// options in the m.key.verification.start message.
    QString hashData() const
    {
        return contentPart<QString>("hash"_ls);
    }

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
REGISTER_EVENT_TYPE(KeyVerificationAcceptEvent)

class QUOTIENT_API KeyVerificationCancelEvent : public Event {
public:
    DEFINE_EVENT_TYPEID("m.key.verification.cancel", KeyVerificationCancelEvent)

    explicit KeyVerificationCancelEvent(const QJsonObject &obj);

    /// An opaque identifier for the verification process.
    QUO_CONTENT_GETTER(QString, transactionId)

    /// A human readable description of the code. The client should only
    /// rely on this string if it does not understand the code.
    QUO_CONTENT_GETTER(QString, reason)

    /// The error code for why the process/request was cancelled by the user.
    QUO_CONTENT_GETTER(QString, code)
};
REGISTER_EVENT_TYPE(KeyVerificationCancelEvent)

/// Sends the ephemeral public key for a device to the partner device.
/// Typically sent as a to-device event.
class KeyVerificationKeyEvent : public Event {
public:
    DEFINE_EVENT_TYPEID("m.key.verification.key", KeyVerificationKeyEvent)

    explicit KeyVerificationKeyEvent(const QJsonObject &obj);

    /// An opaque identifier for the verification process.
    QUO_CONTENT_GETTER(QString, transactionId)

    /// The device's ephemeral public key, encoded as unpadded base64.
    QUO_CONTENT_GETTER(QString, key)
};
REGISTER_EVENT_TYPE(KeyVerificationKeyEvent)

/// Sends the MAC of a device's key to the partner device.
class QUOTIENT_API KeyVerificationMacEvent : public Event {
public:
    DEFINE_EVENT_TYPEID("m.key.verification.mac", KeyVerificationMacEvent)

    explicit KeyVerificationMacEvent(const QJsonObject &obj);

    /// An opaque identifier for the verification process.
    QUO_CONTENT_GETTER(QString, transactionId)

    /// The device's ephemeral public key, encoded as unpadded base64.
    QUO_CONTENT_GETTER(QString, keys)

    QHash<QString, QString> mac() const
    {
        return contentPart<QHash<QString, QString>>("mac"_ls);
    }
};
REGISTER_EVENT_TYPE(KeyVerificationMacEvent)
} // namespace Quotient
