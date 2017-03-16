/***************************************************************************
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
    class ApiPath
    {
        public:
            ApiPath(QString shortPath,
                    QString scope = "client",
                    QString version = "r0");

            operator QString() const { return fullPath; }

        private:
            QString fullPath;
    };

//    QString makeApiPath(QString shortPath,
//                        QString scope = "client",
//                        QString version = "r0");
    QUrlQuery makeQuery(std::initializer_list< QPair<QString, QString> > l);

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

    enum class JobHttpType { GetJob, PutJob, PostJob, DeleteJob };

    class RequestConfig
    {
        public:
            RequestConfig(QString name, JobHttpType type, QString endpoint,
                          QMimeType contentType, QByteArray data,
                          const QUrlQuery& query = {}, bool needsToken = true)
                : _name(name), _type(type), _endpoint(endpoint)
                , _query(query), _contentType(contentType), _data(data)
                , _needsToken(needsToken)
            { }

            RequestConfig(QString name, JobHttpType type, QString endpoint,
                          const QUrlQuery &query = {}, const Data &data = {},
                          bool needsToken = true)
                : RequestConfig(name, type, endpoint,
                    JsonMimeType(), data.dump(), query, needsToken)
            { }

            RequestConfig(QString name, JobHttpType type, QString endpoint,
                          bool needsToken)
                : RequestConfig(name, type, endpoint, {}, {}, needsToken)
            { }

            QString name() const { return _name; }
            JobHttpType type() const { return _type; }
            QString apiPath() const { return _endpoint; }
            QUrlQuery query() const { return _query; }
            void setQuery(const QUrlQuery& q) { _query = q; }
            QMimeType contentType() const { return _contentType; }
            QByteArray data() const { return _data; }
            void setData(const Data& json) { _data = json.dump(); }
            bool needsToken() const { return _needsToken; }

        protected:
            const QString _name;
            const JobHttpType _type;
            QString _endpoint;
            QUrlQuery _query;
            QMimeType _contentType;
            QByteArray _data;
            bool _needsToken;

            static QMimeType JsonMimeType();
    };
}
