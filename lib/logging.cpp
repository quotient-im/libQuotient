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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "logging.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
#define LOGGING_CATEGORY(Name, Id) Q_LOGGING_CATEGORY((Name), (Id), QtInfoMsg)
#else
#define LOGGING_CATEGORY(Name, Id) Q_LOGGING_CATEGORY((Name), (Id))
#endif

// Use LOGGING_CATEGORY instead of Q_LOGGING_CATEGORY in the rest of the code
LOGGING_CATEGORY(MAIN, "libqmatrixclient.main")
LOGGING_CATEGORY(PROFILER, "libqmatrixclient.profiler")
LOGGING_CATEGORY(EVENTS, "libqmatrixclient.events")
LOGGING_CATEGORY(EPHEMERAL, "libqmatrixclient.events.ephemeral")
LOGGING_CATEGORY(JOBS, "libqmatrixclient.jobs")
LOGGING_CATEGORY(SYNCJOB, "libqmatrixclient.jobs.sync")
