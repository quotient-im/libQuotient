// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "networkaccessmanager.h"

#include "logging.h"
#include "mxcreply.h"
#include "connection.h"

#include "events/filesourceinfo.h"

#include "jobs/downloadfilejob.h" // For DownloadFileJob::makeRequestUrl() only

#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>
#include <QtCore/QStringBuilder>
#include <QtCore/QThread>
#include <QtCore/QReadWriteLock>
#include <QtNetwork/QNetworkReply>

using namespace Quotient;

namespace {

static constexpr auto DirectMediaRequestsSetting =
    "Network/allow_direct_media_requests"_ls;

class {
public:
    void addBaseUrl(const QString& accountId, const QUrl& baseUrl)
    {
        QWriteLocker{ &namLock }, baseUrls.insert(accountId, baseUrl);
    }
    void dropBaseUrl(const QString& accountId)
    {
        QWriteLocker{ &namLock }, baseUrls.remove(accountId);
    }
    QUrl getBaseUrl(const QString& accountId) const
    {
        return QReadLocker{ &namLock }, baseUrls.value(accountId);
    }
    void addIgnoredSslError(const QSslError& error)
    {
        QWriteLocker{ &namLock }, ignoredSslErrors.push_back(error);
    }
    void clearIgnoredSslErrors()
    {
        QWriteLocker{ &namLock }, ignoredSslErrors.clear();
    }
    QList<QSslError> getIgnoredSslErrors() const
    {
        return QReadLocker{ &namLock }, ignoredSslErrors;
    }
    void allowDirectMediaRequests(bool allow)
    {
        if (allow)
            directMediaRequestsAreAllowed.test_and_set();
        else
            directMediaRequestsAreAllowed.clear();
    }
    bool directMediaRequestsAllowed() const
    {
        return directMediaRequestsAreAllowed.test();
    }

private:
    mutable QReadWriteLock namLock{};
    QHash<QString, QUrl> baseUrls{};
    QList<QSslError> ignoredSslErrors{};
    // This one is small enough to be atomic and not need a read-write lock
    std::atomic_flag directMediaRequestsAreAllowed{ false };
} d;

std::once_flag directMediaRequestsInitFlag;

} // anonymous namespace

void NetworkAccessManager::allowDirectMediaRequests(bool allow, bool permanent)
{
    d.allowDirectMediaRequests(allow);
    if (permanent)
        QSettings().setValue(DirectMediaRequestsSetting, allow);
}

bool NetworkAccessManager::directMediaRequestsAllowed()
{
    return d.directMediaRequestsAllowed();
}

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
    // Initialise direct media requests allowance at the very first NAM creation
    std::call_once(directMediaRequestsInitFlag, [] {
        NetworkAccessManager::allowDirectMediaRequests(
            QSettings().value(DirectMediaRequestsSetting).toBool());
    });
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
    Q_ASSERT(!url.isRelative());

    const auto createImplRequest = [this, op, request, outgoingData,
                                    url](const QUrl& baseUrl) {
        QNetworkRequest rewrittenRequest(request);
        rewrittenRequest.setUrl(DownloadFileJob::makeRequestUrl(baseUrl, url));
        auto* implReply = QNetworkAccessManager::createRequest(op,
                                                               rewrittenRequest,
                                                               outgoingData);
        implReply->ignoreSslErrors(d.getIgnoredSslErrors());
        return implReply;
    };

    const QUrlQuery query{ url.query() };
    const auto accountId = query.queryItemValue(QStringLiteral("user_id"));
    if (accountId.isEmpty()) {
        // Using QSettings here because Quotient::NetworkSettings
        // doesn't provide multi-threading guarantees
        if (directMediaRequestsAllowed()) {
            // Best effort with an unauthenticated request directly to the media
            // homeserver (rather than via own homeserver)
            auto* mxcReply = new MxcReply(MxcReply::Deferred);
            // Connection class is, by the moment of this call, reentrant (it
            // is not early on when user/room object factories and E2EE are set;
            // but if you have an mxc link you are already well past that, most
            // likely) so we can create and use it here, even if a connection
            // to the same homeserver exists already.
            auto* c = new Connection(mxcReply);
            connect(c, &Connection::homeserverChanged, mxcReply,
                    [mxcReply, createImplRequest, c](const QUrl& baseUrl) {
                        mxcReply->setNetworkReply(createImplRequest(baseUrl));
                        c->deleteLater();
                    });
            // Hack up a minimum "viable" MXID on the target homeserver
            // to satisfy resolveServer()
            c->resolveServer("@:"_ls % request.url().host());
            return mxcReply;
        }
        qCWarning(NETWORK)
            << "No connection specified, cannot convert mxc request";
        return new MxcReply(MxcReply::Error);
    }
    const auto& baseUrl = d.getBaseUrl(accountId);
    if (!baseUrl.isValid()) {
        // Strictly speaking, it should be an assert...
        qCCritical(NETWORK) << "Homeserver for" << accountId
                            << "not found, cannot convert mxc request";
        return new MxcReply(MxcReply::Error);
    }

    // Convert mxc:// URL into normal http(s) for the given homeserver
    auto* implReply = createImplRequest(baseUrl);
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
