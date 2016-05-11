#pragma once

#include <QtCore/QVariantHash>

#include "basejob.h"

namespace QMatrixClient
{
    class SimpleJob : public BaseJob
    {
        public:
            /**
             * This template allows to add and access result items of jobs
             * in a type-safe way. When you derive from SimpleJob, specify
             * a member that you expect to be filled from the response JSON
             * as ResultItem<Type> field instead of "Type field;". Then in the
             * constructor of the derived class, initialize each result item
             * with the response's JSON key you need to fetch this item from
             * and the job that will parse the response (usually the second
             * parameter will be *this).
             * Only top-level JSON keys and primitive (non-object) types are
             * supported so far; support of inner keys and Event* lists is in plans.
             * Accessing the fetched item is as simple as accessing the respective
             * field as if it were a read accessor call with the same name.
             * get() method does the same, for cases when operator()()
             * cannot/may not be used.
             * Changes of result items through ResultItem<> are not allowed
             * and won't be - it's supposed to be a read-only interface.
             */
            template <typename T>
            class ResultItem
            {
                public:
                    explicit ResultItem(QString k, SimpleJob& j)
                        : jsonKey(k), job(j)
                    {
                        job.resultItems.insert(jsonKey, T());
                    }

                    const T get() const
                    {
                        return job.resultItem(jsonKey).value<T>();
                    }
                    const T operator()() const { return get(); }

                private:
                    SimpleJob& job;
                    QString jsonKey;
            };

            SimpleJob(class ConnectionData* conndata, JobHttpType jobType,
                      QString name = QString(), bool needsToken = true);
            virtual ~SimpleJob() = default;

            QVariant resultItem(QString jsonKey) const;
        protected:
            virtual void parseJson(const QJsonDocument &data) override;

            /**
             * Fills the hashmap of result items from the passed QJsonDocument.
             * The list of result items should be filled before calling this
             * function with pairs of JSON keys and empty QVariants initialized
             * with an expected type or UnknownType if any type is acceptable.
             *
             * @see SimpleJob::ResultItem
             */
            bool fillResult(const QJsonDocument& data);
        private:
            // JSON key mapped to the value (already with the right type
            // inside the QVariant)
            QVariantHash resultItems;

            template <typename ResultItemT>
            friend class ResultItem;
    };
}
