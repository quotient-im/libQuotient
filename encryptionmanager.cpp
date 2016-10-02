/******************************************************************************
 * Copyright (C) 2016 Felix Rohrbach <kde@fxrh.de>
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
 ******************************************************************************/


#include "encryptionmanager.h"

#include <olm/olm.h>

#include <climits>
#include <random>
#include <algorithm>
#include <functional>

#include <QtCore/QDebug>
#include <QtCore/QByteArray>
#include <QtCore/QJsonDocument>

using namespace QMatrixClient;

using random_bytes_engine = std::independent_bits_engine<
    std::default_random_engine, CHAR_BIT, unsigned char>;

class EncryptionManager::Private
{
    public:
        Private(QString id) : userId(id)
                            , valid(false)
                            , account(nullptr) {}

        QString userId;
        QString deviceId;
        bool valid;

        OlmAccount* account;
        QJsonObject publicIdentityKeys;

        random_bytes_engine rbe;
};


EncryptionManager::EncryptionManager(QString userId)
    : d(new Private(userId))
{
    d->account = olm_account( malloc(olm_account_size()) );
}

EncryptionManager::~EncryptionManager()
{
    free(d->account);
}

QString EncryptionManager::userId() const
{
    return d->userId;
}

QString EncryptionManager::deviceId() const
{
    return d->deviceId;
}

bool EncryptionManager::isValid() const
{
    return d->valid;
}

bool EncryptionManager::initialize(QString deviceId)
{
    d->deviceId = deviceId;

    size_t randomSize = olm_create_account_random_length(d->account);
    qDebug() << "randomSize" << randomSize;
    std::vector<unsigned char> randomData(randomSize);
    std::generate(begin(randomData), end(randomData), std::ref(d->rbe));
    size_t create_error = olm_create_account(d->account, (void*) &randomData[0], randomSize);

    if( create_error == olm_error() )
    {
        qDebug() << "OLM ERROR: ACCOUNT: creating: " << olm_account_last_error(d->account);
        return false;
    }

    qDebug() << "OLM: successfully created keys";

    size_t publicIdentitySize = olm_account_identity_keys_length(d->account);
    QByteArray keyData;
    keyData.resize(publicIdentitySize);
    size_t readError = olm_account_identity_keys(d->account, (void*) keyData.data(), publicIdentitySize);
    if( readError == olm_error() )
    {
        qDebug() << "OLM ERROR: ACCOUNT: reading keys: " << olm_account_last_error(d->account);
        return false;
    }
    
    QJsonDocument keyJson = QJsonDocument::fromJson(keyData);
    d->publicIdentityKeys = keyJson.object();

    qDebug() << "OLM: successfully read keys";

    d->valid = true;
    return true;
}

const QJsonObject& EncryptionManager::publicIdentityKeys() const
{
    return d->publicIdentityKeys;
}
