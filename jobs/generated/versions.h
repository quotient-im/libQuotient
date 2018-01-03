/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"

#include <QtCore/QString>
#include <QtCore/QVector>


namespace QMatrixClient
{
    // Operations

    class GetVersionsJob : public BaseJob
    {
        public:
            explicit GetVersionsJob();
            ~GetVersionsJob() override;

            const QVector<QString>& versions() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
