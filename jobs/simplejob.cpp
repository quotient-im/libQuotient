#include "simplejob.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>

using namespace QMatrixClient;

SimpleJob::SimpleJob(ConnectionData* conndata, JobHttpType jobType, QString name, bool needsToken)
    : BaseJob(conndata, jobType, name, needsToken)
{ }

QVariant SimpleJob::resultItem(QString jsonKey) const
{
    return resultItems[jsonKey];
}

void SimpleJob::parseJson(const QJsonDocument& data)
{
    // If fillResult() fails it will invoke fail() from inside; but if it
    // succeeds it will just return true, leaving room for additional
    // processing in parseJson() overrides. Alternatively, SimpleJson::parseJson()
    // can be called the last thing from an override, or not called at all
    // (but then think again if you should derive from SimpleJob).
    if (fillResult(data))
       emitResult();
}

bool SimpleJob::fillResult(const QJsonDocument& data)
{
    QJsonObject json = data.object();
    QStringList failed;
    for (auto pItem = resultItems.begin(); pItem != resultItems.end(); ++pItem)
    {
        QJsonValue jsonVal = json[pItem.key()];
        if (jsonVal.type() == QJsonValue::Undefined)
        {
            qDebug() << "Couldn't find JSON value for key" << pItem.key();
            failed.push_back(pItem.key());
            continue;
        }
        // We need a temporary here because QVariant::convert()
        // is very intrusive: if it fails, it resets the QVariant
        // type - and we need to preserve the QVariant type inside
        // the result hashmap.
        QVariant v = jsonVal.toVariant();
        if (v.convert(pItem->type()))
            *pItem = v;
        else
        {
            qDebug() << "Couldn't convert" << jsonVal << "to" << pItem->type();
            failed.push_back(pItem.key());
        }
    }
    if (!failed.empty())
        fail(UserDefinedError, "Failed to load keys: " + failed.join(", "));

    return failed.empty();
}
