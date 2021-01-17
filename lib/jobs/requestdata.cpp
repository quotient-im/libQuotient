// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "requestdata.h"

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

using namespace Quotient;

auto fromData(const QByteArray& data)
{
    auto source = std::make_unique<QBuffer>();
    source->setData(data);
    source->open(QIODevice::ReadOnly);
    return source;
}

template <typename JsonDataT>
inline auto fromJson(const JsonDataT& jdata)
{
    return fromData(QJsonDocument(jdata).toJson(QJsonDocument::Compact));
}

RequestData::RequestData(const QByteArray& a) : _source(fromData(a)) {}

RequestData::RequestData(const QJsonObject& jo) : _source(fromJson(jo)) {}

RequestData::RequestData(const QJsonArray& ja) : _source(fromJson(ja)) {}

RequestData::~RequestData() = default;
