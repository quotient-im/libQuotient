// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "Quotient/quotient_export.h"

#include <QtNetwork/QNetworkAccessManager>

namespace Quotient {

class QUOTIENT_API NetworkAccessManager : public QNetworkAccessManager {
    Q_OBJECT
public:
    using QNetworkAccessManager::QNetworkAccessManager;

    static void addAccount(const QString& accountId, const QUrl& homeserver,
                           const QByteArray& accessToken = {});
    static void updateAccountSpecVersions(QStringView accountId, const QStringList &versions);
    static void dropAccount(QStringView accountId);

    static QList<QSslError> ignoredSslErrors();
    static void addIgnoredSslError(const QSslError& error);
    static void clearIgnoredSslErrors();

    static void setAccessToken(const QString& userId, const QByteArray& token);

    //! Get a NAM instance for the current thread
    static NetworkAccessManager* instance();

private Q_SLOTS:
    QStringList supportedSchemesImplementation() const; // clazy:exclude=const-signal-or-slot

private:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest& request,
                                 QIODevice* outgoingData = nullptr) override;
};
} // namespace Quotient
