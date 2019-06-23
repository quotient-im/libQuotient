/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once



#include "converters.h"


namespace QMatrixClient
{

// Data structures

/// Used by clients to discover homeserver information.
struct HomeserverInformation
{
    /// The base URL for the homeserver for client-server connections.
    QString baseUrl;


};

template <> struct JsonObjectConverter<HomeserverInformation>
{
    static void dumpTo(QJsonObject& jo, const HomeserverInformation& pod);
    static void fillFrom(const QJsonObject& jo, HomeserverInformation& pod);};



} // namespace QMatrixClient
