/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *******************************************************************************/

#include "uploaddevicekeys.h"

#include "../encryptionmanager.h"
#include "../connectiondata.h"

#include <QtCore/QJsonArray>

using namespace QMatrixClient;

class UploadDeviceKeys::Private
{
    public:
        Private(EncryptionManager* m) : encryptionManager(m) {}

        EncryptionManager* encryptionManager;
};

UploadDeviceKeys::UploadDeviceKeys(ConnectionData* connection, EncryptionManager* encryptionManager)
    : BaseJob(connection, JobHttpType::PostJob, "UploadDeviceKeys")
    , d(new Private(encryptionManager))
{
}

UploadDeviceKeys::~UploadDeviceKeys()
{
    delete d;
}

QString UploadDeviceKeys::apiPath() const
{
    return "_matrix/client/unstable/keys/upload";
}

QJsonObject UploadDeviceKeys::data() const
{
    QString deviceId = d->encryptionManager->deviceId();
    QJsonObject json;
    QJsonArray algorithms = { "m.olm.v1.curve25519-aes-sha2", "m.megolm.v1.aes-sha2" };
    json.insert("algorithms", algorithms);
    json.insert("device_id", deviceId );
    QJsonObject keys;
    keys.insert(QString("curve25519:%1").arg(deviceId), QString::fromUtf8(d->encryptionManager->publicIdentityKeys().toBase64()));
    keys.insert(QString("ed25519:%1").arg(deviceId), QString::fromUtf8(d->encryptionManager->publicIdentityKeys().toBase64()));
    json.insert("keys", keys);
    json.insert("user_id", d->encryptionManager->userId() );
    return json;
}

BaseJob::Status UploadDeviceKeys::parseJson(const QJsonDocument& data)
{
    return Status( Success );
}

