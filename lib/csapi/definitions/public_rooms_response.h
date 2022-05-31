/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include "csapi/definitions/public_rooms_chunk.h"

namespace Quotient {
/// A list of the rooms on the server.
struct PublicRoomsResponse {
    /// A paginated chunk of public rooms.
    QVector<QJsonObject> chunk;

    /// A pagination token for the response. The absence of this token
    /// means there are no more results to fetch and the client should
    /// stop paginating.
    QString nextBatch;

    /// A pagination token that allows fetching previous results. The
    /// absence of this token means there are no results before this
    /// batch, i.e. this is the first batch.
    QString prevBatch;

    /// An estimate on the total number of public rooms, if the
    /// server has an estimate.
    Omittable<int> totalRoomCountEstimate;
};

template <>
struct JsonObjectConverter<PublicRoomsResponse> {
    static void dumpTo(QJsonObject& jo, const PublicRoomsResponse& pod)
    {
        addParam<>(jo, QStringLiteral("chunk"), pod.chunk);
        addParam<IfNotEmpty>(jo, QStringLiteral("next_batch"), pod.nextBatch);
        addParam<IfNotEmpty>(jo, QStringLiteral("prev_batch"), pod.prevBatch);
        addParam<IfNotEmpty>(jo, QStringLiteral("total_room_count_estimate"),
                             pod.totalRoomCountEstimate);
    }
    static void fillFrom(const QJsonObject& jo, PublicRoomsResponse& pod)
    {
        fromJson(jo.value("chunk"_ls), pod.chunk);
        fromJson(jo.value("next_batch"_ls), pod.nextBatch);
        fromJson(jo.value("prev_batch"_ls), pod.prevBatch);
        fromJson(jo.value("total_room_count_estimate"_ls),
                 pod.totalRoomCountEstimate);
    }
};

} // namespace Quotient
