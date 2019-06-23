/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "push_condition.h"


using namespace QMatrixClient;

    
void JsonObjectConverter<PushCondition>::dumpTo(QJsonObject& jo, const PushCondition& pod)
{
    addParam<>(jo, QStringLiteral("kind"), pod.kind);
    addParam<IfNotEmpty>(jo, QStringLiteral("key"), pod.key);
    addParam<IfNotEmpty>(jo, QStringLiteral("pattern"), pod.pattern);
    addParam<IfNotEmpty>(jo, QStringLiteral("is"), pod.is);

}
    
void JsonObjectConverter<PushCondition>::fillFrom(const QJsonObject& jo, PushCondition& result)
{
    fromJson(jo.value("kind"_ls), result.kind);
    fromJson(jo.value("key"_ls), result.key);
    fromJson(jo.value("pattern"_ls), result.pattern);
    fromJson(jo.value("is"_ls), result.is);

}
    


