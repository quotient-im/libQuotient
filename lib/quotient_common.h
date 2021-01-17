// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <qobjectdefs.h>

namespace Quotient {
Q_NAMESPACE

/** Enumeration with flags defining the network job running policy
 * So far only background/foreground flags are available.
 *
 * \sa Connection::callApi, Connection::run
 */
enum RunningPolicy { ForegroundRequest = 0x0, BackgroundRequest = 0x1 };

Q_ENUM_NS(RunningPolicy)

enum UriResolveResult : short {
    StillResolving = -1,
    UriResolved = 0,
    CouldNotResolve,
    IncorrectAction,
    InvalidUri,
    NoAccount
};
Q_ENUM_NS(UriResolveResult)

} // namespace Quotient
/// \deprecated Use namespace Quotient instead
namespace QMatrixClient = Quotient;
