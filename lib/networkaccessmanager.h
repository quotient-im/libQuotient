// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtNetwork/QNetworkAccessManager>

#include <memory>

namespace Quotient {
class Room;
class Connection;
class NetworkAccessManager : public QNetworkAccessManager {
    Q_OBJECT
public:
    NetworkAccessManager(QObject* parent = nullptr);
    ~NetworkAccessManager() override;

    QList<QSslError> ignoredSslErrors() const;
    void addIgnoredSslError(const QSslError& error);
    void clearIgnoredSslErrors();
    void ignoreSslErrors(bool ignore = true) const;

    /** Get a pointer to the singleton */
    static NetworkAccessManager* instance();

public Q_SLOTS:
    QStringList supportedSchemesImplementation() const;

private:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest& request,
                                 QIODevice* outgoingData = Q_NULLPTR) override;

    class Private;
    std::unique_ptr<Private> d;
};
} // namespace Quotient
