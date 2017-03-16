#include "requestconfig.h"

#include <QtCore/QStringBuilder>
#include <QtCore/QJsonDocument>
#include <QtCore/QMimeDatabase>

using namespace QMatrixClient;

ApiPath::ApiPath(QString shortPath, QString scope, QString version)
    : fullPath("/_matrix/" % scope % "/" % version % shortPath)
{ }

//QString QMatrixClient::makeApiPath(QString shortPath, QString scope, QString version)
//{
//    return "/_matrix/" % scope % "/" % version % shortPath;
//}

QUrlQuery QMatrixClient::makeQuery(std::initializer_list<QPair<QString, QString> > l)
{
    QUrlQuery q;
    q.setQueryItems(l);
    return q;
}

void Data::insert(const QString& name, const QStringList& sl)
{
    insert(name, QJsonArray::fromStringList(sl));
}

QByteArray Data::dump() const
{
    return QJsonDocument(*this).toJson();
}

QMimeType RequestConfig::JsonMimeType()
{
    static const auto jsonMimeType =
            QMimeDatabase().mimeTypeForName("application/json");
    Q_ASSERT_X(jsonMimeType.isValid(), __FUNCTION__,
       "MIME database doesn't have JSON type; check your mime/packages file(s)");
    return jsonMimeType;
}
