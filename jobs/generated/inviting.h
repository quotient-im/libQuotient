/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#pragma once

#include "../basejob.h"

#include <QtCore/QString>


namespace QMatrixClient
{

    // Operations
    
    class InviteUserJob : public BaseJob
    {
        public:
            InviteUserJob(QString roomId, QString user_id);
        
    };

} // namespace QMatrixClient
