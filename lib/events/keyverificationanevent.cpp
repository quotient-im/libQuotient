// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "keyverificationanevent.h"

using namespace Quotient;

KeyVerificationRequestEvent::KeyVerificationRequestEvent(const QJsonObject &obj)
    : Event(typeId(), obj)
{}

QString KeyVerificationRequestEvent::fromDevice() const
{
    return contentJson()["from_device"_ls].toString();
}

QString KeyVerificationRequestEvent::transactionId() const
{
    return contentJson()["transaction_id"_ls].toString();
}

QStringList KeyVerificationRequestEvent::methods() const
{
    QStringList methods;
    for (const auto &method : contentJson()["methods"].toArray()) {
        methods.append(method.toString());
    }
    return methods;
}

uint64_t KeyVerificationRequestEvent::timestamp() const
{
    return contentJson()["timestamp"_ls].toDouble();
}

KeyVerificationStartEvent::KeyVerificationStartEvent(const QJsonObject &obj)
    : Event(typeId(), obj)
{}

QString KeyVerificationStartEvent::fromDevice() const
{
    return contentJson()["from_device"_ls].toString();
}

QString KeyVerificationStartEvent::transactionId() const
{
    return contentJson()["transaction_id"_ls].toString();
}

QString KeyVerificationStartEvent::method() const
{
    return contentJson()["method"_ls].toString();
}

Omittable<QString> KeyVerificationStartEvent::nextMethod() const
{
    auto next = contentJson()["method"_ls];
    if (next.isUndefined()) {
        return std::nullopt;
    }
    return next.toString();
}

QStringList KeyVerificationStartEvent::keyAgreementProtocols() const
{
    Q_ASSERT(method() == QStringLiteral("m.sas.v1"));
    QStringList protocols;
    for (const auto &proto : contentJson()["key_agreement_protocols"_ls].toArray()) {
        protocols.append(proto.toString());
    }
    return protocols;
}

QStringList KeyVerificationStartEvent::hashes() const
{
    Q_ASSERT(method() == QStringLiteral("m.sas.v1"));
    QStringList hashes;
    for (const auto &hashItem : contentJson()["hashes"_ls].toArray()) {
        hashes.append(hashItem.toString());
    }
    return hashes;
}

QStringList KeyVerificationStartEvent::messageAuthenticationCodes() const
{
    Q_ASSERT(method() == QStringLiteral("m.sas.v1"));

    QStringList codes;
    for (const auto &code : contentJson()["message_authentication_codes"_ls].toArray()) {
        codes.append(code.toString());
    }
    return codes;
}

QString KeyVerificationStartEvent::shortAuthenticationString() const
{
    return contentJson()["short_authentification_string"_ls].toString();
}

KeyVerificationAcceptEvent::KeyVerificationAcceptEvent(const QJsonObject &obj)
    : Event(typeId(), obj)
{}

QString KeyVerificationAcceptEvent::transactionId() const
{
    return contentJson()["transaction_id"_ls].toString();
}

QString KeyVerificationAcceptEvent::method() const
{
    return contentJson()["method"_ls].toString();
}

QString KeyVerificationAcceptEvent::keyAgreementProtocol() const
{
    return contentJson()["key_agreement_protocol"_ls].toString();
}

QString KeyVerificationAcceptEvent::hashData() const
{
    return contentJson()["hash"_ls].toString();
}

QStringList KeyVerificationAcceptEvent::shortAuthenticationString() const
{
    QStringList strings;
    for (const auto &authenticationString : contentJson()["short_authentification_string"].toArray()) {
        strings.append(authenticationString.toString());
    }
    return strings;
}

QString KeyVerificationAcceptEvent::commitement() const
{
    return contentJson()["commitement"].toString();
}

KeyVerificationCancelEvent::KeyVerificationCancelEvent(const QJsonObject &obj)
    : Event(typeId(), obj)
{}

QString KeyVerificationCancelEvent::transactionId() const
{
    return contentJson()["transaction_id"_ls].toString();
}

QString KeyVerificationCancelEvent::reason() const
{
    return contentJson()["reason"_ls].toString();
}

QString KeyVerificationCancelEvent::code() const
{
    return contentJson()["code"_ls].toString();
}

KeyVerificationKeyEvent::KeyVerificationKeyEvent(const QJsonObject &obj)
    : Event(typeId(), obj)
{}

QString KeyVerificationKeyEvent::transactionId() const
{
    return contentJson()["transaction_id"_ls].toString();
}

QString KeyVerificationKeyEvent::key() const
{
    return contentJson()["key"_ls].toString();
}

KeyVerificationMacEvent::KeyVerificationMacEvent(const QJsonObject &obj)
    : Event(typeId(), obj)
{}

QString KeyVerificationMacEvent::transactionId() const
{
    return contentJson()["transaction_id"].toString();
}

QString KeyVerificationMacEvent::keys() const
{
    return contentJson()["keys"].toString();
}

QHash<QString, QString> KeyVerificationMacEvent::mac() const
{
    QHash<QString, QString> macs;
    const auto macObj = contentJson()["mac"_ls].toObject();
    for (auto mac = macObj.constBegin(); mac != macObj.constEnd(); mac++) {
        macs.insert(mac.key(), mac.value().toString());
    }
    return macs;
}
