/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "sid.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const Sid& pod)
{
    QJsonObject jo;
    addParam<>(jo, QStringLiteral("sid"), pod.sid);
    return jo;
}

Sid FromJsonObject<Sid>::operator()(const QJsonObject& jo) const
{
    Sid result;
    result.sid =
        fromJson<QString>(jo.value("sid"_ls));

    return result;
}

