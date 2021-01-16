// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "converters.h"

#include <QtCore/QVariant>

using namespace Quotient;

QJsonValue JsonConverter<QVariant>::dump(const QVariant& v)
{
    return QJsonValue::fromVariant(v);
}

QVariant JsonConverter<QVariant>::load(const QJsonValue& jv)
{
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
