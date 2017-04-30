/******************************************************************************
 * Copyright (C) 2017 Elvis Angelaccio <elvid.angelaccio@kde.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "util.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
Q_LOGGING_CATEGORY(MAIN, "libqmatrixclient.main", QtInfoMsg)
Q_LOGGING_CATEGORY(EVENTS, "libqmatrixclient.events", QtInfoMsg)
Q_LOGGING_CATEGORY(JOBS, "libqmatrixclient.jobs", QtInfoMsg)
#else
Q_LOGGING_CATEGORY(MAIN, "libqmatrixclient.main")
Q_LOGGING_CATEGORY(EVENTS, "libqmatrixclient.events")
Q_LOGGING_CATEGORY(JOBS, "libqmatrixclient.jobs")
#endif

