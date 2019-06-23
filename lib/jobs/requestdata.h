/******************************************************************************
 * Copyright (C) 2018 Kitsune Ral <kitsune-ral@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#pragma once

#include <memory>

class QByteArray;
class QJsonObject;
class QJsonArray;
class QJsonDocument;
class QIODevice;

namespace QMatrixClient
{
/**
 * A simple wrapper that represents the request body.
 * Provides a unified interface to dump an unstructured byte stream
 * as well as JSON (and possibly other structures in the future) to
 * a QByteArray consumed by QNetworkAccessManager request methods.
 */
class RequestData
{
public:
    RequestData() = default;
    RequestData(const QByteArray& a);
    RequestData(const QJsonObject& jo);
    RequestData(const QJsonArray& ja);
    RequestData(QIODevice* source)
        : _source(std::unique_ptr<QIODevice>(source))
    {}
    RequestData(const RequestData&) = delete;
    RequestData& operator=(const RequestData&) = delete;
    RequestData(RequestData&&) = default;
    RequestData& operator=(RequestData&&) = default;
    ~RequestData();

    QIODevice* source() const { return _source.get(); }

private:
    std::unique_ptr<QIODevice> _source;
};
} // namespace QMatrixClient
