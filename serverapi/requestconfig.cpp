#include "requestconfig.h"

#include <QtCore/QStringBuilder>
#include <QtCore/QJsonDocument>
#include <QtCore/QMimeDatabase>

using namespace QMatrixClient::ServerApi;

ApiPath::ApiPath(QString shortPath, QString scope, QString version)
    : fullPath("/_matrix/" % scope % "/" % version % "/" % shortPath)
{ }

void Data::insert(const QString& name, const QStringList& sl)
{
    insert(name, QJsonArray::fromStringList(sl));
}

QByteArray Data::dump() const
{
    return QJsonDocument(*this).toJson();
}

const QMimeType RequestConfig::JsonMimeType =
        QMimeDatabase().mimeTypeForName("application/json");
