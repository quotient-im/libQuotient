/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "event_context.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

auto queryToGetEventContext(Omittable<int> limit, const QString& filter)
{
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("filter"), filter);
    return _q;
}

QUrl GetEventContextJob::makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                        const QString& eventId,
                                        Omittable<int> limit,
                                        const QString& filter)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/rooms/" % roomId % "/context/"
                                       % eventId,
                                   queryToGetEventContext(limit, filter));
}

GetEventContextJob::GetEventContextJob(const QString& roomId,
                                       const QString& eventId,
                                       Omittable<int> limit,
                                       const QString& filter)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetEventContextJob"),
              QStringLiteral("/_matrix/client/r0") % "/rooms/" % roomId
                  % "/context/" % eventId,
              queryToGetEventContext(limit, filter))
{}
