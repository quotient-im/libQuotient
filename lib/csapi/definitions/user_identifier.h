/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once



#include "converters.h"

#include <QtCore/QVariant>

namespace QMatrixClient
{

// Data structures

/// Identification information for a user
struct UserIdentifier
{
    /// The type of identification.  See `Identifier types`_ for supported values and additional property descriptions.
    QString type;


    /// Identification information for a user
    QVariantHash additionalProperties;

};

template <> struct JsonObjectConverter<UserIdentifier>
{
    static void dumpTo(QJsonObject& jo, const UserIdentifier& pod);
    static void fillFrom(QJsonObject jo, UserIdentifier& pod);};



} // namespace QMatrixClient
