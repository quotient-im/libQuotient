// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "keyverificationevent.h"

using namespace Quotient;

KeyVerificationRequestEvent::KeyVerificationRequestEvent(const QJsonObject &obj)
    : Event(typeId(), obj)
{}

QString KeyVerificationRequestEvent::fromDevice() const
{
    return contentPart<QString>("from_device"_ls);
}

QString KeyVerificationRequestEvent::transactionId() const
{
    return contentPart<QString>("transaction_id"_ls);
}

QStringList KeyVerificationRequestEvent::methods() const
{
    return contentPart<QStringList>("methods"_ls);
}

uint64_t KeyVerificationRequestEvent::timestamp() const
{
    return contentPart<double>("timestamp"_ls);
}

KeyVerificationStartEvent::KeyVerificationStartEvent(const QJsonObject &obj)
    : Event(typeId(), obj)
{}

QString KeyVerificationStartEvent::fromDevice() const
{
    return contentPart<QString>("from_device"_ls);
}

QString KeyVerificationStartEvent::transactionId() const
{
    return contentPart<QString>("transaction_id"_ls);
}

QString KeyVerificationStartEvent::method() const
{
    return contentPart<QString>("method"_ls);
}

Omittable<QString> KeyVerificationStartEvent::nextMethod() const
{
    return contentPart<Omittable<QString>>("method_ls");
}

QStringList KeyVerificationStartEvent::keyAgreementProtocols() const
{
    Q_ASSERT(method() == QStringLiteral("m.sas.v1"));
    return contentPart<QStringList>("key_agreement_protocols"_ls);
}

QStringList KeyVerificationStartEvent::hashes() const
{
    Q_ASSERT(method() == QStringLiteral("m.sas.v1"));
        return contentPart<QStringList>("hashes"_ls);

}

QStringList KeyVerificationStartEvent::messageAuthenticationCodes() const
{
    Q_ASSERT(method() == QStringLiteral("m.sas.v1"));
    return contentPart<QStringList>("message_authentication_codes"_ls);
}

QString KeyVerificationStartEvent::shortAuthenticationString() const
{
    return contentPart<QString>("short_authentification_string"_ls);
}

KeyVerificationAcceptEvent::KeyVerificationAcceptEvent(const QJsonObject &obj)
    : Event(typeId(), obj)
{}

QString KeyVerificationAcceptEvent::transactionId() const
{
    return contentPart<QString>("transaction_id"_ls);
}

QString KeyVerificationAcceptEvent::method() const
{
    return contentPart<QString>("method"_ls);
}

QString KeyVerificationAcceptEvent::keyAgreementProtocol() const
{
    return contentPart<QString>("key_agreement_protocol"_ls);
}

QString KeyVerificationAcceptEvent::hashData() const
{
    return contentPart<QString>("hash"_ls);
}

QStringList KeyVerificationAcceptEvent::shortAuthenticationString() const
{
    return contentPart<QStringList>("short_authentification_string"_ls);
}

QString KeyVerificationAcceptEvent::commitment() const
{
    return contentPart<QString>("commitment"_ls);
}

KeyVerificationCancelEvent::KeyVerificationCancelEvent(const QJsonObject &obj)
    : Event(typeId(), obj)
{}

QString KeyVerificationCancelEvent::transactionId() const
{
    return contentPart<QString>("transaction_id"_ls);
}

QString KeyVerificationCancelEvent::reason() const
{
    return contentPart<QString>("reason"_ls);
}

QString KeyVerificationCancelEvent::code() const
{
    return contentPart<QString>("code"_ls);
}

KeyVerificationKeyEvent::KeyVerificationKeyEvent(const QJsonObject &obj)
    : Event(typeId(), obj)
{}

QString KeyVerificationKeyEvent::transactionId() const
{
    return contentPart<QString>("transaction_id"_ls);
}

QString KeyVerificationKeyEvent::key() const
{
    return contentPart<QString>("key"_ls);
}

KeyVerificationMacEvent::KeyVerificationMacEvent(const QJsonObject &obj)
    : Event(typeId(), obj)
{}

QString KeyVerificationMacEvent::transactionId() const
{
    return contentPart<QString>("transaction_id"_ls);
}

QString KeyVerificationMacEvent::keys() const
{
    return contentPart<QString>("keys"_ls);
}

QHash<QString, QString> KeyVerificationMacEvent::mac() const
{
    return contentPart<QHash<QString, QString>>("mac"_ls);
}

KeyVerificationDoneEvent::KeyVerificationDoneEvent(const QJsonObject &obj)
    : Event(typeId(), obj)
{
}

QString KeyVerificationDoneEvent::transactionId() const
{
    return contentPart<QString>("transaction_id"_ls);
}


KeyVerificationReadyEvent::KeyVerificationReadyEvent(const QJsonObject &obj)
    : Event(typeId(), obj)
{}

QString KeyVerificationReadyEvent::fromDevice() const
{
    return contentPart<QString>("from_device"_ls);
}

QString KeyVerificationReadyEvent::transactionId() const
{
    return contentPart<QString>("transaction_id"_ls);
}

QStringList KeyVerificationReadyEvent::methods() const
{
    return contentPart<QStringList>("methods"_ls);
}
