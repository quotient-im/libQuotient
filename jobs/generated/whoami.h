/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#pragma once

#include "../basejob.h"

#include <QtCore/QString>


namespace QMatrixClient
{
    // Operations

    class GetTokenOwnerJob : public BaseJob
    {
        public:
            explicit GetTokenOwnerJob();
            ~GetTokenOwnerJob() override;

            const QString& userId() const;
            
        protected:
            Status parseJson(const QJsonDocument& data) override;
            
        private:
            class Private;
            Private* d;
    };
} // namespace QMatrixClient
