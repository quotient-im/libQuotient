#include "quotient_common.h"

#include <QtCore/QDebug>

using namespace Quotient;

template <typename Enum>
inline QDebug suppressScopeAndDump(QDebug dbg, Enum e)
{
    // Suppress "Quotient::" prefix
    QDebugStateSaver _dss(dbg);
    dbg.setVerbosity(QDebug::MinimumVerbosity);
    return qt_QMetaEnum_debugOperator(dbg, std::underlying_type_t<Enum>(e),
                                      qt_getEnumMetaObject(e),
                                      qt_getEnumName(e));
}

template <typename Enum>
inline QDebug suppressScopeAndDump(QDebug dbg, const QFlags<Enum>& f)
{
    // Suppress "Quotient::" prefix
    QDebugStateSaver _dss(dbg);
    dbg.setVerbosity(QDebug::MinimumVerbosity);
    return qt_QMetaEnum_flagDebugOperator_helper(dbg, f);
}

QDebug operator<<(QDebug dbg, Membership m)
{
    return suppressScopeAndDump(dbg, m);
}

QDebug operator<<(QDebug dbg, MembershipMask mm)
{
    return suppressScopeAndDump(dbg, mm) << ")";
}

QDebug operator<<(QDebug dbg, JoinState js)
{
    return suppressScopeAndDump(dbg, js);
}

QDebug operator<<(QDebug dbg, JoinStates jss)
{
    return suppressScopeAndDump(dbg, jss) << ")";
}
