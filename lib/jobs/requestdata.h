// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtCore/QByteArray>

#include <memory>

class QJsonObject;
class QJsonArray;
class QJsonDocument;
class QIODevice;

namespace Quotient {
/**
 * A simple wrapper that represents the request body.
 * Provides a unified interface to dump an unstructured byte stream
 * as well as JSON (and possibly other structures in the future) to
 * a QByteArray consumed by QNetworkAccessManager request methods.
 */
class RequestData {
public:
    RequestData(const QByteArray& a = {});
    RequestData(const QJsonObject& jo);
    RequestData(const QJsonArray& ja);
    RequestData(QIODevice* source);
    RequestData(RequestData&&) = default;
    RequestData& operator=(RequestData&&) = default;
    ~RequestData();

    QIODevice* source() const { return _source.get(); }

private:
    std::unique_ptr<QIODevice> _source;
};
} // namespace Quotient
/// \deprecated Use namespace Quotient instead
namespace QMatrixClient = Quotient;
