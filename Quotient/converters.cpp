// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "converters.h"
#include "logging.h"

#include <QtCore/QVariant>

void Quotient::_impl::warnUnknownEnumValue(const QString& stringValue,
                                           const char* enumTypeName)
{
    qWarning(EVENTS).noquote()
        << "Unknown" << enumTypeName << "value:" << stringValue;
}

void Quotient::_impl::reportEnumOutOfBounds(uint32_t v, const char* enumTypeName)
{
    qCritical(MAIN).noquote()
        << "Value" << v << "is out of bounds for enumeration" << enumTypeName;
}

QJsonValue Quotient::JsonConverter<QVariant>::dump(const QVariant& v)
{
    return QJsonValue::fromVariant(v);
}

QVariant Quotient::JsonConverter<QVariant>::load(const QJsonValue& jv)
{
    return jv.toVariant();
}

QJsonObject Quotient::toJson(const QVariantHash& vh)
{
    return QJsonObject::fromVariantHash(vh);
}

template<>
QVariantHash Quotient::fromJson(const QJsonValue& jv)
{
    return jv.toObject().toVariantHash();
}
