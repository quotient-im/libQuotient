// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "networkaccessmanager.h"

#include "logging_categories_p.h"
#include "mxcreply.h"

#include "events/filesourceinfo.h"
#include "jobs/downloadfilejob.h" // For DownloadFileJob::makeRequestUrl() only

#include <QtCore/QCoreApplication>
#include <QtCore/QReadWriteLock>
#include <QtCore/QSettings>
#include <QtCore/QStringBuilder>
#include <QtCore/QThread>
#include <QtNetwork/QNetworkReply>

using namespace Quotient;

namespace {
class {
public:
    void addBaseUrl(const QString& accountId, const QUrl& baseUrl)
    {
        const QWriteLocker _(&namLock);
        baseUrls.insert(accountId, baseUrl);
    }
    void dropBaseUrl(const QString& accountId)
    {
        const QWriteLocker _(&namLock);
        baseUrls.remove(accountId);
    }
    QUrl getBaseUrl(const QString& accountId) const
    {
        const QReadLocker _(&namLock);
        return baseUrls.value(accountId);
    }
    void addIgnoredSslError(const QSslError& error)
    {
        const QWriteLocker _(&namLock);
        ignoredSslErrors.push_back(error);
    }
    void clearIgnoredSslErrors()
    {
        const QWriteLocker _(&namLock);
        ignoredSslErrors.clear();
    }
    QList<QSslError> getIgnoredSslErrors() const
    {
        const QReadLocker _(&namLock);
        return ignoredSslErrors;
    }

private:
    mutable QReadWriteLock namLock{};
    QHash<QString, QUrl> baseUrls{};
    QList<QSslError> ignoredSslErrors{};
} d;

} // anonymous namespace

void NetworkAccessManager::addBaseUrl(const QString& accountId,
                                      const QUrl& homeserver)
{
    Q_ASSERT(!accountId.isEmpty() && homeserver.isValid());
    d.addBaseUrl(accountId, homeserver);
}

void NetworkAccessManager::dropBaseUrl(const QString& accountId)
{
    d.dropBaseUrl(accountId);
}

QList<QSslError> NetworkAccessManager::ignoredSslErrors()
{
    return d.getIgnoredSslErrors();
}

void NetworkAccessManager::addIgnoredSslError(const QSslError& error)
{
    d.addIgnoredSslError(error);
}

void NetworkAccessManager::clearIgnoredSslErrors()
{
    d.clearIgnoredSslErrors();
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
        reply->ignoreSslErrors(d.getIgnoredSslErrors());
        return reply;
    }
    const QUrlQuery query{ url.query() };
    const auto accountId = query.queryItemValue(QStringLiteral("user_id"));
    if (accountId.isEmpty()) {
        // Using QSettings here because Quotient::NetworkSettings
        // doesn't provide multi-threading guarantees
        if (static thread_local const QSettings s;
            s.value("Network/allow_direct_media_requests"_ls).toBool()) //
        {
            // TODO: Make the best effort with a direct unauthenticated request
            // to the media server
            qCWarning(NETWORK)
                << "Direct unauthenticated mxc requests are not implemented";
            return new MxcReply();
        }
        qCWarning(NETWORK)
            << "No connection specified, cannot convert mxc request";
        return new MxcReply();
    }
    const auto& baseUrl = d.getBaseUrl(accountId);
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
    implReply->ignoreSslErrors(d.getIgnoredSslErrors());
    const auto& fileMetadata = FileMetadataMap::lookup(
        query.queryItemValue(QStringLiteral("room_id")),
        query.queryItemValue(QStringLiteral("event_id")));
    return new MxcReply(implReply, fileMetadata);
}

QStringList NetworkAccessManager::supportedSchemesImplementation() const
{
    return QNetworkAccessManager::supportedSchemesImplementation()
           << QStringLiteral("mxc");
}
