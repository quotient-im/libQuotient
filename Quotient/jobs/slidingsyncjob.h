// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "../csapi/definitions/sync_filter.h"
#include "../slidingsyncdata.h"
#include "basejob.h"
#include "events/stateevent.h"
#include "events/roomevent.h"


namespace Quotient {

class QUOTIENT_API SlidingSyncJob : public BaseJob {
public:
    explicit SlidingSyncJob(const QString& txnId, QString pos = {});

    SlidingSyncData takeData() { return std::move(d); }

protected:
    Status prepareResult() override;

private:
    SlidingSyncData d;
};
} // namespace Quotient
