/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include <QtCore/QJsonObject>
#include <QtCore/QVector>
#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    /// Definition of valid values for a field.
    struct FieldType
    {
        /// A regular expression for validation of a field's value.
        QString regexp;
        /// An placeholder serving as a valid example of the field value.
        QString placeholder;
    };

    QJsonObject toJson(const FieldType& pod);

    template <> struct FromJson<FieldType>
    {
        FieldType operator()(const QJsonValue& jv);
    };

    /// All location or user fields should have an entry here.
    struct FieldTypes
    {
        /// Definition of valid values for a field.
        Omittable<FieldType> fieldname;
    };

    QJsonObject toJson(const FieldTypes& pod);

    template <> struct FromJson<FieldTypes>
    {
        FieldTypes operator()(const QJsonValue& jv);
    };

    struct ThirdPartyProtocol
    {
        /// Fields used to identify a third party user.
        QStringList userFields;
        /// Fields used to identify a third party location.
        QStringList locationFields;
        /// An icon representing the third party protocol.
        QString icon;
        /// All location or user fields should have an entry here.
        Omittable<FieldTypes> fieldTypes;
        /// A list of objects representing independent instances of configuration.
        /// For instance multiple networks on IRC if multiple are bridged by the
        /// same bridge.
        QVector<QJsonObject> instances;
    };

    QJsonObject toJson(const ThirdPartyProtocol& pod);

    template <> struct FromJson<ThirdPartyProtocol>
    {
        ThirdPartyProtocol operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient
