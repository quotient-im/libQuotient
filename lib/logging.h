// SPDX-FileCopyrightText: 2017 Elvis Angelaccio <elvid.angelaccio@kde.org>
// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(MAIN)
Q_DECLARE_LOGGING_CATEGORY(STATE)
Q_DECLARE_LOGGING_CATEGORY(MEMBERS)
Q_DECLARE_LOGGING_CATEGORY(MESSAGES)
Q_DECLARE_LOGGING_CATEGORY(EVENTS)
Q_DECLARE_LOGGING_CATEGORY(EPHEMERAL)
Q_DECLARE_LOGGING_CATEGORY(E2EE)
Q_DECLARE_LOGGING_CATEGORY(JOBS)
Q_DECLARE_LOGGING_CATEGORY(SYNCJOB)
Q_DECLARE_LOGGING_CATEGORY(THUMBNAILJOB)
Q_DECLARE_LOGGING_CATEGORY(NETWORK)
Q_DECLARE_LOGGING_CATEGORY(PROFILER)

namespace Quotient {
// QDebug manipulators

using QDebugManip = QDebug (*)(QDebug);

/**
 * @brief QDebug manipulator to setup the stream for JSON output
 *
 * Originally made to encapsulate the change in QDebug behavior in Qt 5.4
 * and the respective addition of QDebug::noquote().
 * Together with the operator<<() helper, the proposed usage is
 * (similar to std:: I/O manipulators):
 *
 * @example qCDebug() << formatJson << json_object; // (QJsonObject, etc.)
 */
inline QDebug formatJson(QDebug debug_object)
{
    return debug_object.noquote();
}

//! Suppress full qualification of enums/QFlags when logging
inline QDebug terse(QDebug dbg)
{
    return
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
        dbg.setVerbosity(0), dbg;
#else
        dbg.verbosity(QDebug::MinimumVerbosity);
#endif
}

inline qint64 profilerMinNsecs()
{
    return
#ifdef PROFILER_LOG_USECS
        PROFILER_LOG_USECS
#else
        200
#endif
        * 1000;
}
} // namespace Quotient

/**
 * @brief A helper operator to facilitate usage of formatJson (and possibly
 * other manipulators)
 *
 * @param debug_object to output the json to
 * @param qdm a QDebug manipulator
 * @return a copy of debug_object that has its mode altered by qdm
 */
inline QDebug operator<<(QDebug debug_object, Quotient::QDebugManip qdm)
{
    return qdm(debug_object);
}

inline QDebug operator<<(QDebug debug_object, const QElapsedTimer& et)
{
    auto val = et.nsecsElapsed() / 1000;
    if (val < 1000)
        debug_object << val << "µs";
    else
        debug_object << val / 1000 << "ms";
    return debug_object;
}
