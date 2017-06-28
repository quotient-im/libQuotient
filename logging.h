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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(MAIN)
Q_DECLARE_LOGGING_CATEGORY(PROFILER)
Q_DECLARE_LOGGING_CATEGORY(EVENTS)
Q_DECLARE_LOGGING_CATEGORY(EPHEMERAL)
Q_DECLARE_LOGGING_CATEGORY(JOBS)
Q_DECLARE_LOGGING_CATEGORY(SYNCJOB)

namespace QMatrixClient
{
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
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
            return debug_object;
#else
            return debug_object.noquote();
#endif
    };

    /**
     * @brief A helper operator to facilitate usage of formatJson (and possibly
     * other manipulators)
     *
     * @param debug_object to output the json to
     * @param qdm a QDebug manipulator
     * @return a copy of debug_object that has its mode altered by qdm
     */
    inline QDebug operator<< (QDebug debug_object, QDebugManip qdm)
    {
        return qdm(debug_object);
    }
}
