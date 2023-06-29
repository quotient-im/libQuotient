// SPDX-FileCopyrightText: 2017 Elvis Angelaccio <elvid.angelaccio@kde.org>
// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtCore/QLoggingCategory>

#define QUO_LOGGING_CATEGORY(Name, Id) \
    inline Q_LOGGING_CATEGORY((Name), (Id), QtInfoMsg)

namespace Quotient {

QUO_LOGGING_CATEGORY(MAIN, "quotient.main")
QUO_LOGGING_CATEGORY(EVENTS, "quotient.events")
QUO_LOGGING_CATEGORY(STATE, "quotient.events.state")
QUO_LOGGING_CATEGORY(MEMBERS, "quotient.events.members")
QUO_LOGGING_CATEGORY(MESSAGES, "quotient.events.messages")
QUO_LOGGING_CATEGORY(EPHEMERAL, "quotient.events.ephemeral")
QUO_LOGGING_CATEGORY(E2EE, "quotient.e2ee")
QUO_LOGGING_CATEGORY(JOBS, "quotient.jobs")
QUO_LOGGING_CATEGORY(SYNCJOB, "quotient.jobs.sync")
QUO_LOGGING_CATEGORY(THUMBNAILJOB, "quotient.jobs.thumbnail")
QUO_LOGGING_CATEGORY(NETWORK, "quotient.network")
QUO_LOGGING_CATEGORY(PROFILER, "quotient.profiler")
QUO_LOGGING_CATEGORY(DATABASE, "quotient.database")

} // namespace Quotient

