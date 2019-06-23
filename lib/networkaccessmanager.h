/******************************************************************************
 * Copyright (C) 2018 Kitsune Ral <kitsune-ral@users.sf.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#pragma once

#include <QtNetwork/QNetworkAccessManager>

#include <memory>

namespace QMatrixClient
{
class NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    NetworkAccessManager(QObject* parent = nullptr);
    ~NetworkAccessManager() override;

    QList<QSslError> ignoredSslErrors() const;
    void addIgnoredSslError(const QSslError& error);
    void clearIgnoredSslErrors();

    /** Get a pointer to the singleton */
    static NetworkAccessManager* instance();

private:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest& request,
                                 QIODevice* outgoingData = Q_NULLPTR) override;

    class Private;
    std::unique_ptr<Private> d;
};
} // namespace QMatrixClient
