// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "event.h"

namespace Quotient {

/// Requests a key verification with another user's devices.
/// Typically sent as a to-device event.
class KeyVerificationRequestEvent : public Event {
    Q_GADGET
public:
    DEFINE_EVENT_TYPEID("m.key.verification.request", KeyVerificationRequestEvent)

    explicit KeyVerificationRequestEvent(const QJsonObject& obj);

    /// The device ID which is initiating the request.
    QString fromDevice() const;

    /// An opaque identifier for the verification request. Must
    /// be unique with respect to the devices involved.
    QString transactionId() const;

    /// The verification methods supported by the sender.
    QStringList methods() const;

    /// The POSIX timestamp in milliseconds for when the request was
    /// made. If the request is in the future by more than 5 minutes or
    /// more than 10 minutes in the past, the message should be ignored
    /// by the receiver.
    uint64_t timestamp() const;
};
REGISTER_EVENT_TYPE(KeyVerificationRequestEvent)

/// Begins a key verification process.
class KeyVerificationStartEvent : public Event {
    Q_GADGET
public:
    DEFINE_EVENT_TYPEID("m.key.verification.start", KeyVerificationStartEvent)

    explicit KeyVerificationStartEvent(const QJsonObject &obj);

    /// The device ID which is initiating the process.
    QString fromDevice() const;

    /// An opaque identifier for the verification request. Must
    /// be unique with respect to the devices involved.
    QString transactionId() const;

    /// The verification method to use.
    QString method() const;

    /// Optional method to use to verify the other user's key with.
    Omittable<QString> nextMethod() const;

    // SAS.V1 methods

    /// The key agreement protocols the sending device understands.
    /// \note Only exist if method is m.sas.v1
    QStringList keyAgreementProtocols() const;

    /// The hash methods the sending device understands.
    /// \note Only exist if method is m.sas.v1
    QStringList hashes() const;

    /// The message authentication codes that the sending device understands.
    /// \note Only exist if method is m.sas.v1
    QStringList messageAuthenticationCodes() const;

    /// The SAS methods the sending device (and the sending device's
    /// user) understands.
    /// \note Only exist if method is m.sas.v1
    QString shortAuthenticationString() const;
};
REGISTER_EVENT_TYPE(KeyVerificationStartEvent)

/// Accepts a previously sent m.key.verification.start message.
/// Typically sent as a to-device event.
class KeyVerificationAcceptEvent : public Event {
    Q_GADGET
public:
    DEFINE_EVENT_TYPEID("m.key.verification.accept", KeyVerificationAcceptEvent)

    explicit KeyVerificationAcceptEvent(const QJsonObject& obj);

    /// An opaque identifier for the verification process.
    QString transactionId() const;

    /// The verification method to use. Must be 'm.sas.v1'.
    QString method() const;

    /// The key agreement protocol the device is choosing to use, out of
    /// the options in the m.key.verification.start message.
    QString keyAgreementProtocol() const;

    /// The hash method the device is choosing to use, out of the
    /// options in the m.key.verification.start message.
    QString hashData() const;

    /// The message authentication code the device is choosing to use, out
    /// of the options in the m.key.verification.start message.
    QString messageAuthenticationCode() const;

    /// The SAS methods both devices involved in the verification process understand.
    QStringList shortAuthenticationString() const;

    /// The hash (encoded as unpadded base64) of the concatenation of the
    /// device's ephemeral public key (encoded as unpadded base64) and the
    /// canonical JSON representation of the m.key.verification.start message.
    QString commitement() const;
};
REGISTER_EVENT_TYPE(KeyVerificationAcceptEvent)

class KeyVerificationCancelEvent : public Event {
    Q_GADGET
public:
    DEFINE_EVENT_TYPEID("m.key.verification.cancel", KeyVerificationCancelEvent)

    explicit KeyVerificationCancelEvent(const QJsonObject &obj);

    /// An opaque identifier for the verification process.
    QString transactionId() const;

    /// A human readable description of the code. The client should only
    /// rely on this string if it does not understand the code.
    QString reason() const;

    /// The error code for why the process/request was cancelled by the user.
    QString code() const;
};
REGISTER_EVENT_TYPE(KeyVerificationCancelEvent)

/// Sends the ephemeral public key for a device to the partner device.
/// Typically sent as a to-device event.
class KeyVerificationKeyEvent : public Event {
    Q_GADGET
public:
    DEFINE_EVENT_TYPEID("m.key.verification.key", KeyVerificationKeyEvent)

    explicit KeyVerificationKeyEvent(const QJsonObject &obj);

    /// An opaque identifier for the verification process. 
    QString transactionId() const;

    /// The device's ephemeral public key, encoded as unpadded base64.
    QString key() const;
};
REGISTER_EVENT_TYPE(KeyVerificationKeyEvent)

/// Sends the MAC of a device's key to the partner device.
class KeyVerificationMacEvent : public Event {
    Q_GADGET
public:
    DEFINE_EVENT_TYPEID("m.key.verification.mac", KeyVerificationMacEvent)

    explicit KeyVerificationMacEvent(const QJsonObject &obj);

    /// An opaque identifier for the verification process. 
    QString transactionId() const;

    /// The device's ephemeral public key, encoded as unpadded base64.
    QString keys() const;

    QHash<QString, QString> mac() const;
};
REGISTER_EVENT_TYPE(KeyVerificationMacEvent)
} // namespace Quotient
