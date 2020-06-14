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

enum ResourceResolveResult : short {
    StillResolving = -1,
    Resolved = 0,
    UnknownMatrixId,
    MalformedMatrixId,
    NoAccount,
    EmptyMatrixId
};
Q_ENUM_NS(ResourceResolveResult)

} // namespace Quotient
/// \deprecated Use namespace Quotient instead
namespace QMatrixClient = Quotient;
