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
            explicit InviteUserJob(QString roomId, QString userId);

    };

} // namespace QMatrixClient
