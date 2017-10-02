/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#include "kicking.h"


#include "../converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

    

static const auto basePath = QStringLiteral("/_matrix/client/r0");

KickJob::KickJob(QString roomId, QString user_id, QString reason)
    : BaseJob(HttpVerb::Post, "KickJob"
        , basePath % "/rooms/" % roomId % "/kick"
        , Query {  }
        , Data { 
              { "user_id", toJson(user_id) }, 
              { "reason", toJson(reason) }
         }
    )
{ }
    

    

