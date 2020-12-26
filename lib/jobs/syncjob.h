/******************************************************************************
 * SPDX-FileCopyrightText: 2016 Felix Rohrbach <kde@fxrh.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "../csapi/definitions/sync_filter.h"
#include "../syncdata.h"
#include "basejob.h"

namespace Quotient {
class SyncJob : public BaseJob {
public:
    explicit SyncJob(const QString& since = {}, const QString& filter = {},
                     int timeout = -1, const QString& presence = {});
    explicit SyncJob(const QString& since, const Filter& filter,
                     int timeout = -1, const QString& presence = {});

    SyncData&& takeData() { return std::move(d); }

protected:
    Status prepareResult() override;

private:
    SyncData d;
};
} // namespace Quotient
