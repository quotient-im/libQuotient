// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "converters.h"

#include <QVariant>
#include "e2ee/e2ee.h"

QJsonValue Quotient::JsonConverter<QVariant>::dump(const QVariant& v)
{
    if (v.canConvert<SignedOneTimeKey>()) {
        return toJson(v.value<SignedOneTimeKey>());
    }
    return QJsonValue::fromVariant(v);
}

QVariant Quotient::JsonConverter<QVariant>::load(const QJsonValue& jv)
{
    if (jv.isObject()) {
        const QJsonObject obj = jv.toObject();
        if (obj.contains(QLatin1String("key")) && obj.contains(QLatin1String("signatures"))) {
            SignedOneTimeKey signedOneTimeKeys;
            signedOneTimeKeys.key = obj[QLatin1String("key")].toString();
        }
    }
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
