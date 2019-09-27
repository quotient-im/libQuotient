/******************************************************************************
 * Copyright (C) 2017 Kitsune Ral <kitsune-ral@users.sf.net>
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
