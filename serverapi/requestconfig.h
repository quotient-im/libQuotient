/******************************************************************************
 * Copyright (C) 2016 Kitsune Ral <kitsune-ral@users.sf.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include <QtCore/QUrlQuery>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QByteArray>
#include <QtCore/QMimeType>

namespace QMatrixClient
{
    namespace ServerApi
    {
        class ApiPath
        {
            public:
                ApiPath(QString shortPath,
                        QString scope = "client",
                        QString version = "r0");
                ApiPath(const char * shortPath)
                    : ApiPath(QString(shortPath))
                { }
                QString toString() const { return fullPath; }

            private:
                QString fullPath;
        };
        class Query : public QUrlQuery
        {
            public:
                using QUrlQuery::QUrlQuery;
                Query() = default;
                explicit Query(std::initializer_list< QPair<QString, QString> > l)
                {
                    setQueryItems(l);
                }
                using QUrlQuery::addQueryItem;
                void addQueryItem(const QString& name, int value)
                {
                    addQueryItem(name, QString::number(value));
                }
        };
        class Data : public QJsonObject
        {
            public:
                Data() = default;
                explicit Data(std::initializer_list< QPair<QString, QJsonValue> > l)
                {
                    for (auto i: l)
                        insert(i.first, i.second);
                }
                using QJsonObject::insert;
                void insert(const QString& name, const QStringList& sl);
                template <typename T>
                void insert(const QString& name, const QVector<T>& vv)
                {
                    QJsonArray ja;
                    std::copy(vv.begin(), vv.end(), std::back_inserter(ja));
                    insert(name, ja);
                }

                QByteArray dump() const;
        };
        enum class HttpVerb { Get, Put, Post, Delete };

        class RequestConfig
        {
            public:
                RequestConfig(QString name, HttpVerb type, ApiPath endpoint,
                              QMimeType contentType, QByteArray data,
                              Query query = Query(), bool needsToken = true)
                    : _name(name), _type(type), _endpoint(endpoint)
                    , _query(query), _contentType(contentType), _data(data)
                    , _needsToken(needsToken)
                { }

                RequestConfig(QString name, HttpVerb type, ApiPath endpoint,
                              Query query, Data data = Data(),
                              bool needsToken = true)
                    : RequestConfig(name, type, endpoint,
                        JsonMimeType, data.dump(), query, needsToken)
                { }

                RequestConfig(QString name, HttpVerb type, ApiPath endpoint,
                              bool needsToken = true)
                    : RequestConfig(name, type, endpoint,
                                    Query(), Data(), needsToken)
                { }

                QString name() const { return _name; }
                HttpVerb type() const { return _type; }
                QString apiPath() const { return _endpoint.toString(); }
                QUrlQuery query() const { return _query; }
                void setQuery(const Query& q) { _query = q; }
                QMimeType contentType() const { return _contentType; }
                QByteArray data() const { return _data; }
                void setData(const Data& json) { _data = json.dump(); }
                bool needsToken() const { return _needsToken; }

            protected:
                const QString _name;
                const HttpVerb _type;
                ApiPath _endpoint;
                Query _query;
                QMimeType _contentType;
                QByteArray _data;
                bool _needsToken;

                static const QMimeType JsonMimeType;
        };
    }
}
