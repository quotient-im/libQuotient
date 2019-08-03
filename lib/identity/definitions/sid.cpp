/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "sid.h"

using namespace QMatrixClient;

void JsonObjectConverter<Sid>::dumpTo(QJsonObject& jo, const Sid& pod)
{
    addParam<>(jo, QStringLiteral("sid"), pod.sid);
}

void JsonObjectConverter<Sid>::fillFrom(const QJsonObject& jo, Sid& result)
{
    fromJson(jo.value("sid"_ls), result.sid);
}
