// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "../csapi/definitions/sync_filter.h"
#include "../syncdata.h"
#include "basejob.h"

#include <functional>

namespace Quotient {
class QUOTIENT_API SyncJob : public BaseJob {
public:
    static constexpr auto defaultTimeout = std::chrono::seconds(30);
    static constexpr auto defaultTimeoutMillis =
        std::chrono::milliseconds(defaultTimeout).count();

    explicit SyncJob(const QString& since = {}, const QString& filter = {},
                     int timeout = defaultTimeoutMillis, const QString& presence = {});
    explicit SyncJob(const QString& since, const Filter& filter, int timeout = defaultTimeoutMillis,
                     const QString& presence = {});

    SyncData takeData() { return std::move(d); }

    //! \brief Set a function to process the raw response before it gets parsed
    //!
    //! If \p processor returns false, the job ends with IncorrectResponse without parsing
    //! the response, so that it is retried with the same `since` token. This ensures that
    //! a response is not acknowledged to the server (by using its `next_batch` token) before
    //! its end-to-end encryption data have been processed.
    void setRawResponseProcessor(std::function<bool(const QByteArray&)> processor);

protected:
    Status prepareResult() override;

private:
    SyncData d;
    std::function<bool(const QByteArray&)> rawResponseProcessor;
};
} // namespace Quotient
