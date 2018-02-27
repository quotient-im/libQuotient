/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"

#include <QtCore/QVector>

#include "converters.h"

namespace QMatrixClient
{
    // Operations

    class SearchUserDirectoryJob : public BaseJob
    {
        public:
            // Inner data structures

            struct User
            {
                QString userId;
                QString displayName;
                QString avatarUrl;
                
                operator QJsonObject() const;
            };

            // End of inner data structures

            explicit SearchUserDirectoryJob(const QString& searchTerm, double limit = {});
            ~SearchUserDirectoryJob() override;

            const QVector<User>& results() const;
            bool limited() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient
