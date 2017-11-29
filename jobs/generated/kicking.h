/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#pragma once

#include "../basejob.h"

#include <QtCore/QString>


namespace QMatrixClient
{

    // Operations

    class KickJob : public BaseJob
    {
        public:
            explicit KickJob(QString roomId, QString userId, QString reason = {});

    };

} // namespace QMatrixClient
