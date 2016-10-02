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

#ifndef QMATRIXCLIENT_ENCRYPTIONMANAGER_H
#define QMATRIXCLIENT_ENCRYPTIONMANAGER_H

#include <QtCore/QString>

#include <vector>

namespace QMatrixClient
{
    /**
     * EncryptionManager manages the cryptographic operations for the
     * E2E encryption of Matrix using libolm.
     *
     * An instance of EncryptionManager manages one user account.
     */
    class EncryptionManager
    {
        public:
            EncryptionManager(QString userId);
            ~EncryptionManager();

            /**
             * Check whether this is a valid instace of EncryptionManager,
             * ready to use.
             */
            bool isValid();

            /**
             * Set the device id and create all necessairy long term keys.
             *
             * If any error happens, this function returns false.
             */
            bool initialize(QString deviceId);

            const std::vector<unsigned char>& publicIdentityKeys() const;

        private:
            class Private;
            Private* d;
    };
}

#endif // QMATRIXCLIENT_ENCRYPTIONMANAGER_H

