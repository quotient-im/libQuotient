// SPDX-FileCopyrightText: 2017 Elvis Angelaccio <elvid.angelaccio@kde.org>
// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "logging.h"

#define LOGGING_CATEGORY(Name, Id) Q_LOGGING_CATEGORY((Name), (Id), QtInfoMsg)

namespace Quotient {

LOGGING_CATEGORY(MAIN, "quotient.main")
LOGGING_CATEGORY(EVENTS, "quotient.events")
LOGGING_CATEGORY(STATE, "quotient.events.state")
LOGGING_CATEGORY(MEMBERS, "quotient.events.members")
LOGGING_CATEGORY(MESSAGES, "quotient.events.messages")
LOGGING_CATEGORY(EPHEMERAL, "quotient.events.ephemeral")
LOGGING_CATEGORY(E2EE, "quotient.e2ee")
LOGGING_CATEGORY(JOBS, "quotient.jobs")
LOGGING_CATEGORY(SYNCJOB, "quotient.jobs.sync")
LOGGING_CATEGORY(THUMBNAILJOB, "quotient.jobs.thumbnail")
LOGGING_CATEGORY(NETWORK, "quotient.network")
LOGGING_CATEGORY(PROFILER, "quotient.profiler")
LOGGING_CATEGORY(DATABASE, "quotient.database")

}
