/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#pragma once

#include "../basejob.h"

#include <QtCore/QString>


namespace QMatrixClient
{

    // Operations

    class BanJob : public BaseJob
    {
        public:
            explicit BanJob(QString roomId, QString user_id, QString reason = {});

    };
    class UnbanJob : public BaseJob
    {
        public:
            explicit UnbanJob(QString roomId, QString user_id);

    };

} // namespace QMatrixClient
