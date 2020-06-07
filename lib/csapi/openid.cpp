/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "openid.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

RequestOpenIdTokenJob::RequestOpenIdTokenJob(const QString& userId,
                                             const QJsonObject& body)
    : BaseJob(HttpVerb::Post, QStringLiteral("RequestOpenIdTokenJob"),
              QStringLiteral("/_matrix/client/r0") % "/user/" % userId
                  % "/openid/request_token")
{
    setRequestData(Data(toJson(body)));
}
