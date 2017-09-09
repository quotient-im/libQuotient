/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#include "inviting.h"


#include "../converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

    

static const auto basePath = QStringLiteral("/_matrix/client/r0");

    
InviteUserJob::InviteUserJob(const ConnectionData* connection, 
            QString roomId
        , 
            QString user_id
        )
    : BaseJob(connection, HttpVerb::Post, "InviteUserJob"
        , basePath % "/rooms/" % roomId % "/invite"
        , Query {  }
        , Data { 
              { "user_id", toJson(user_id) }
         }
        
    )
{ }
    

    

