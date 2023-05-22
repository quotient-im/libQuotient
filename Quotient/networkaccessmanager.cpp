// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "networkaccessmanager.h"

#include "logging.h"
#include "mxcreply.h"

#include "events/filesourceinfo.h"

#include "jobs/downloadfilejob.h" // For DownloadFileJob::makeRequestUrl() only

#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>
#include <QtCore/QStringBuilder>
#include <QtCore/QThread>
#include <QtCore/QReadWriteLock>
#include <QtNetwork/QNetworkReply>

using namespace Quotient;

class Q_DECL_HIDDEN NetworkAccessManager::Private {
public:
    static inline QHash<QString, QUrl> baseUrls{};
    static inline QReadWriteLock baseUrlsLock{};

    QList<QSslError> ignoredSslErrors;

    static QUrl getBaseUrl(const QString& accountId)
    {
        return QReadLocker(&baseUrlsLock), baseUrls.value(accountId);
    }
};

NetworkAccessManager::NetworkAccessManager(QObject* parent)
    : QNetworkAccessManager(parent), d(makeImpl<Private>())
{}

void NetworkAccessManager::addBaseUrl(const QString& accountId,
                                      const QUrl& homeserver)
{
    Q_ASSERT(!accountId.isEmpty() && homeserver.isValid());
    const QWriteLocker l(&Private::baseUrlsLock);
    Private::baseUrls.insert(accountId, homeserver);
}

void NetworkAccessManager::dropBaseUrl(const QString& accountId)
{
    const QWriteLocker l(&Private::baseUrlsLock);
    Private::baseUrls.remove(accountId);
}

QList<QSslError> NetworkAccessManager::ignoredSslErrors() const
{
    return d->ignoredSslErrors;
}

void NetworkAccessManager::ignoreSslErrors(bool ignore) const
{
    if (ignore) {
        connect(this, &QNetworkAccessManager::sslErrors, this,
                [](QNetworkReply* reply) { reply->ignoreSslErrors(); });
    } else {
        disconnect(this, &QNetworkAccessManager::sslErrors, this, nullptr);
    }
}

void NetworkAccessManager::addIgnoredSslError(const QSslError& error)
{
    d->ignoredSslErrors << error;
}

void NetworkAccessManager::clearIgnoredSslErrors()
{
    d->ignoredSslErrors.clear();
}

NetworkAccessManager* NetworkAccessManager::instance()
{
    thread_local auto* nam = [] {
        auto* namInit = new NetworkAccessManager();
        connect(QThread::currentThread(), &QThread::finished, namInit,
                &QObject::deleteLater);
        return namInit;
    }();
    return nam;
}

QNetworkReply* NetworkAccessManager::createRequest(
    Operation op, const QNetworkRequest& request, QIODevice* outgoingData)
{
    const auto url = request.url();
    if (url.scheme() != "mxc"_ls) {
        auto reply =
            QNetworkAccessManager::createRequest(op, request, outgoingData);
        reply->ignoreSslErrors(d->ignoredSslErrors);
        return reply;
    }
    const QUrlQuery query{ url.query() };
    const auto accountId = query.queryItemValue(QStringLiteral("user_id"));
    if (accountId.isEmpty()) {
        // Using QSettings here because Quotient::NetworkSettings
        // doesn't provide multithreading guarantees
        static thread_local const QSettings s;
        if (!s.value("Network/allow_direct_media_requests"_ls).toBool()) {
            qCWarning(NETWORK)
                << "No connection specified, cannot convert mxc request";
            return new MxcReply();
        }
        // TODO: Make the best effort with a direct unauthenticated request
        // to the media server
        qCWarning(NETWORK)
            << "Direct unauthenticated mxc requests are not implemented";
        return new MxcReply();
    }
    const auto& baseUrl = Private::getBaseUrl(accountId);
    if (!baseUrl.isValid()) {
        // Strictly speaking, it should be an assert...
        qCCritical(NETWORK) << "Homeserver for" << accountId
                            << "not found, cannot convert mxc request";
        return new MxcReply();
    }

    // Convert mxc:// URL into normal http(s) for the given homeserver
    QNetworkRequest rewrittenRequest(request);
    rewrittenRequest.setUrl(DownloadFileJob::makeRequestUrl(baseUrl, url));

    auto* implReply = QNetworkAccessManager::createRequest(op, rewrittenRequest);
    implReply->ignoreSslErrors(d->ignoredSslErrors);
    const auto& fileMetadata = FileMetadataMap::lookup(
        query.queryItemValue(QStringLiteral("room_id")),
        query.queryItemValue(QStringLiteral("event_id")));
    return new MxcReply(implReply, fileMetadata);
}

QStringList NetworkAccessManager::supportedSchemesImplementation() const
{
    auto schemes = QNetworkAccessManager::supportedSchemesImplementation();
    schemes += QStringLiteral("mxc");
    return schemes;
}
