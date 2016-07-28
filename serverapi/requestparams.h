#pragma once

#include <QtCore/QString>
#include <QtCore/QUrlQuery>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>

namespace QMatrixClient
{
    /**
     * This class defines the generic set of parameters for a request to
     * a Matrix server. It also provides a set of wrapper classes that ease
     * creation of input from a list of key-value pairs. Thanks to an
     * implicit QList<> constructor, such list can be passed as a braced-init
     * list.
     */
    class RequestParams
    {
        public: // Supplementary type definitions
            class Query : public QUrlQuery
            {
                public:
                    using QUrlQuery::QUrlQuery;
                    Query() = default;
                    Query(QList<QPair<QString, QString> > l)
                    {
                        setQueryItems(l);
                    }
            };
            class Data : public QJsonObject
            {
                public:
                    using QJsonObject::QJsonObject;
                    Data() = default;
                    Data(QList<QPair<QString, QString> > l)
                    {
                        for (auto i: l)
                            insert(i.first, i.second);
                    }
            };
            enum class HttpType { Get, Put, Post };

        public: // Methods
            RequestParams(HttpType t, QString p,
                    Query q = Query(), Data d = Data(), bool needsToken = true)
                : m_type(t), m_endpoint(p), m_query(q), m_data(d)
                , m_needsToken(needsToken)
            { }

            HttpType type() const { return m_type; }
            QString apiPath() const { return m_endpoint; }
            QUrlQuery query() const { return m_query; }
            QByteArray data() const { return QJsonDocument(m_data).toJson(); }
            bool needsToken() const { return m_needsToken; }

        private:
            HttpType m_type;
            QString m_endpoint;
            Query m_query;
            Data m_data;
            bool m_needsToken;
    };

}
