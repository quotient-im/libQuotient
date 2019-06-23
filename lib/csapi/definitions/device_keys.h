/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once



#include "converters.h"

#include <QtCore/QHash>

namespace QMatrixClient
{

// Data structures

/// Device identity keys
struct DeviceKeys
{
    /// The ID of the user the device belongs to. Must match the user ID usedwhen logging in.
    QString userId;
    /// The ID of the device these keys belong to. Must match the device ID usedwhen logging in.
    QString deviceId;
    /// The encryption algorithms supported by this device.
    QStringList algorithms;
    /// Public identity keys. The names of the properties should be in theformat ``<algorithm>:<device_id>``. The keys themselves should beencoded as specified by the key algorithm.
    QHash<QString, QString> keys;
    /// Signatures for the device key object. A map from user ID, to a map from``<algorithm>:<device_id>`` to the signature.The signature is calculated using the process described at `SigningJSON`_.
    QHash<QString, QHash<QString, QString>> signatures;


};

template <> struct JsonObjectConverter<DeviceKeys>
{
    static void dumpTo(QJsonObject& jo, const DeviceKeys& pod);
    static void fillFrom(const QJsonObject& jo, DeviceKeys& pod);};



} // namespace QMatrixClient
