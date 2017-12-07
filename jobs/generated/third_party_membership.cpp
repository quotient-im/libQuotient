/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#include "third_party_membership.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

InviteBy3PIDJob::InviteBy3PIDJob(const QString& roomId, const QString& idServer, const QString& medium, const QString& address)
    : BaseJob(HttpVerb::Post, "InviteBy3PIDJob",
        basePath % "/rooms/" % roomId % "/invite",
        Query { }
    )
{
    QJsonObject _data;
    _data.insert("id_server", toJson(idServer));
    _data.insert("medium", toJson(medium));
    _data.insert("address", toJson(address));
    setRequestData(_data);
}

