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
        basePath % "/rooms/" % roomId % "/invite")
{
    QJsonObject _data;
    addParam<>(_data, "id_server", idServer);
    addParam<>(_data, "medium", medium);
    addParam<>(_data, "address", address);
    setRequestData(_data);
}

