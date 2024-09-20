// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "networkaccessmanager.h"

#include "connectiondata.h"
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
    struct ConnectionData {
        QString accountId;
        HomeserverData hsData;
    };

    void addConnection(const QString& accountId, HomeserverData hsData)
    {
        if (hsData.baseUrl.isEmpty())
            return;

        const QWriteLocker _(&namLock);
        if (auto it = std::ranges::find(connectionData, accountId, &ConnectionData::accountId);
            it != connectionData.end())
            it->hsData = std::move(hsData);
        else // Xcode doesn't like emplace_back() below for some reason (anon class?..)
            connectionData.push_back({ accountId, std::move(hsData) });
    }
    void addSpecVersions(QStringView accountId, const QStringList& versions)
    {
        if (versions.isEmpty())
            return;

        const QWriteLocker _(&namLock);
        auto it = std::ranges::find(connectionData, accountId, &ConnectionData::accountId);
        if (QUO_ALARM_X(it == connectionData.end(), "Quotient::NAM: Trying to save supported spec "
                                                    "versions on an inexistent account"))
            return;

        it->hsData.supportedSpecVersions = versions;
    }
    void dropConnection(QStringView accountId)
    {
        const QWriteLocker _(&namLock);
        std::erase_if(connectionData,
                      [&accountId](const ConnectionData& cd) { return cd.accountId == accountId; });
    }
    HomeserverData getConnection(const QString& accountId) const
    {
        const QReadLocker _(&namLock);
        auto it = std::ranges::find(connectionData, accountId, &ConnectionData::accountId);
        return it == connectionData.cend() ? HomeserverData{} : it->hsData;
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
    void setAccessToken(const QString& userId, const QByteArray& accessToken)
    {
        const QWriteLocker _(&namLock);
        if (auto it = std::ranges::find(connectionData, userId, &ConnectionData::accountId);
            it != connectionData.end()) {
            it->hsData.accessToken = accessToken;
        }
    }

private:
    mutable QReadWriteLock namLock{};
    std::vector<ConnectionData> connectionData{};
    QList<QSslError> ignoredSslErrors{};
} d;

} // anonymous namespace

void NetworkAccessManager::addAccount(const QString& accountId, const QUrl& homeserver,
                                      const QByteArray& accessToken)
{
    Q_ASSERT(!accountId.isEmpty());
    d.addConnection(accountId, { homeserver, accessToken });
}

void NetworkAccessManager::setAccessToken(const QString& userId, const QByteArray& token)
{
    d.setAccessToken(userId, token);
}

void NetworkAccessManager::updateAccountSpecVersions(QStringView accountId,
                                                     const QStringList& versions)
{
    Q_ASSERT(!accountId.isEmpty());
    d.addSpecVersions(accountId, versions);
}

void NetworkAccessManager::dropAccount(QStringView accountId)
{
    d.dropConnection(accountId);
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
    if (url.scheme() != "mxc"_L1) {
        auto reply =
            QNetworkAccessManager::createRequest(op, request, outgoingData);
        reply->ignoreSslErrors(d.getIgnoredSslErrors());
        return reply;
    }
    const QUrlQuery query{ url.query() };
    const auto accountId = query.queryItemValue(u"user_id"_s);
    if (accountId.isEmpty()) {
        // Using QSettings here because Quotient::NetworkSettings
        // doesn't provide multi-threading guarantees
        if (static thread_local const QSettings s;
            s.value("Network/allow_direct_media_requests"_L1).toBool()) //
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
    const auto& hsData = d.getConnection(accountId);
    if (!hsData.baseUrl.isValid()) {
        // Strictly speaking, it should be an assert...
        qCCritical(NETWORK) << "Homeserver for" << accountId
                            << "not found, cannot convert mxc request";
        return new MxcReply();
    }

    // Convert mxc:// URL into normal http(s) for the given homeserver
    QNetworkRequest rewrittenRequest(request);
    rewrittenRequest.setUrl(DownloadFileJob::makeRequestUrl(hsData, url));
    rewrittenRequest.setRawHeader("Authorization", "Bearer "_ba + hsData.accessToken);

    auto* implReply = QNetworkAccessManager::createRequest(op, rewrittenRequest);
    implReply->ignoreSslErrors(d.getIgnoredSslErrors());
    const auto& fileMetadata = FileMetadataMap::lookup(query.queryItemValue(u"room_id"_s),
                                                       query.queryItemValue(u"event_id"_s));
    return new MxcReply(implReply, fileMetadata);
}

QStringList NetworkAccessManager::supportedSchemesImplementation() const
{
    return QNetworkAccessManager::supportedSchemesImplementation() << u"mxc"_s;
}
