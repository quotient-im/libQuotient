/******************************************************************************
 * Copyright (C) 2016 Felix Rohrbach <kde@fxrh.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#pragma once

#include "../csapi/definitions/sync_filter.h"
#include "../syncdata.h"
#include "basejob.h"

namespace QMatrixClient {
class SyncJob : public BaseJob {
public:
    explicit SyncJob(const QString& since = {}, const QString& filter = {},
                     int timeout = -1, const QString& presence = {});
    explicit SyncJob(const QString& since, const Filter& filter,
                     int timeout = -1, const QString& presence = {});

    SyncData&& takeData() { return std::move(d); }

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    SyncData d;
};
} // namespace QMatrixClient
