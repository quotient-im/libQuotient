// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "converters.h"

#include <QVariant>
#include "crypto/e2ee.h"

using namespace Quotient;

QJsonValue JsonConverter<QVariant>::dump(const QVariant& v)
{
    if (v.canConvert<SignedOneTimeKey>()) {
        return toJson(v.value<SignedOneTimeKey>());
    }
    return QJsonValue::fromVariant(v);
}

QVariant JsonConverter<QVariant>::load(const QJsonValue& jv)
{
    if (jv.isObject()) {
        QJsonObject obj = jv.toObject();
        if (obj.contains("key") && obj.contains("signatures")) {
            SignedOneTimeKey signedOneTimeKeys;
            signedOneTimeKeys.key = obj["key"].toString();
        }
    }
    return jv.toVariant();
}

QJsonObject JsonConverter<QVariantHash>::dump(const QVariantHash& vh)
{
    return QJsonObject::fromVariantHash(vh);
}

QVariantHash JsonConverter<QVariantHash>::load(const QJsonValue& jv)
{
    return jv.toObject().toVariantHash();
}
