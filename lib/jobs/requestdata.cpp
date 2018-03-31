#include "requestdata.h"

#include <QtCore/QByteArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QBuffer>

using namespace QMatrixClient;

auto fromData(const QByteArray& data)
{
    auto source = std::make_unique<QBuffer>();
    source->open(QIODevice::WriteOnly);
    source->write(data);
    source->close();
    return source;
}

template <typename JsonDataT>
inline auto fromJson(const JsonDataT& jdata)
{
    return fromData(QJsonDocument(jdata).toJson(QJsonDocument::Compact));
}

RequestData::RequestData(const QByteArray& a)
    : _source(fromData(a))
{ }

RequestData::RequestData(const QJsonObject& jo)
    : _source(fromJson(jo))
{ }

RequestData::RequestData(const QJsonArray& ja)
    : _source(fromJson(ja))
{ }

RequestData::~RequestData() = default;
