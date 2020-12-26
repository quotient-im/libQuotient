/******************************************************************************
 * SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "basejob.h"

#include <QtCore/QJsonObject>

using namespace Quotient;

class PostReadMarkersJob : public BaseJob {
public:
    explicit PostReadMarkersJob(const QString& roomId,
                                const QString& readUpToEventId)
        : BaseJob(
            HttpVerb::Post, "PostReadMarkersJob",
            QStringLiteral("_matrix/client/r0/rooms/%1/read_markers").arg(roomId))
    {
        setRequestData(
            QJsonObject { { QStringLiteral("m.fully_read"), readUpToEventId } });
    }
};
