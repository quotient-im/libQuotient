// SPDX-FileCopyrightText: 2020 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ssosession.h"

#include "connection.h"
#include "csapi/sso_login_redirect.h"

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtCore/QCoreApplication>
#include <QtCore/QStringBuilder>
#include <QNetworkProxy>

using namespace Quotient;

class Q_DECL_HIDDEN SsoSession::Private {
public:
    Private(SsoSession* q, QString initialDeviceName, QString deviceId,
            Connection* connection)
        : initialDeviceName(std::move(initialDeviceName))
        , deviceId(std::move(deviceId))
        , connection(connection)
    {
        auto* server = new QTcpServer(q);
        // User might apply an application level proxy and we don't need them here.
        server->setProxy(QNetworkProxy::NoProxy);
        if (!server->listen())
            qCritical(MAIN)
                << "Could not open the port, SSO callback won't work:" << server->errorString();
        // The "/returnToApplication" part is just a hint for the end-user,
        // the callback will work without it equally well.
        callbackUrl = QStringLiteral("http://localhost:%1/returnToApplication")
                          .arg(server->serverPort());
        ssoUrl = connection->getUrlForApi<RedirectToSSOJob>(callbackUrl);

        QObject::connect(server, &QTcpServer::newConnection, q, [this, q, server] {
            qCDebug(MAIN) << "SSO callback initiated";
            socket = server->nextPendingConnection();
            server->close();
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [this] {
                requestData.append(socket->readAll());
                if (!socket->atEnd() && !requestData.endsWith("\r\n\r\n")) {
                    qDebug(MAIN) << "Incomplete request, waiting for more data";
                    return;
                }
                processCallback();
            });
            QObject::connect(socket, &QTcpSocket::disconnected, socket,
                             &QTcpSocket::deleteLater);
            QObject::connect(socket, &QObject::destroyed, q,
                             &QObject::deleteLater);
        });
        qCDebug(MAIN) << "SSO session constructed";
    }
    ~Private() { qCDebug(MAIN) << "SSO session deconstructed"; }
    Q_DISABLE_COPY_MOVE(Private)

    void processCallback();
    void sendHttpResponse(const QByteArray& code, const QByteArray& msg);
    void onError(const QByteArray& code, const QString& errorMsg);

    QString initialDeviceName;
    QString deviceId;
    Connection* connection;
    QString callbackUrl {};
    QUrl ssoUrl {};
    QTcpSocket* socket = nullptr;
    QByteArray requestData {};
};

SsoSession::SsoSession(Connection* connection, const QString& initialDeviceName,
                       const QString& deviceId)
    : QObject(connection)
    , d(makeImpl<Private>(this, initialDeviceName, deviceId, connection))
{}

QUrl SsoSession::ssoUrl() const { return d->ssoUrl; }

QUrl SsoSession::callbackUrl() const { return QUrl(d->callbackUrl); }

void SsoSession::Private::processCallback()
{
    // https://matrix.org/docs/guides/sso-for-client-developers
    // Inspired by Clementine's src/internet/core/localredirectserver.cpp
    // (see at https://github.com/clementine-player/Clementine/)
    const auto& requestParts = requestData.split(' ');
    if (requestParts.size() < 2 || requestParts[1].isEmpty()) {
        onError("400 Bad Request", tr("Malformed single sign-on callback"));
        return;
    }
    const auto& QueryItemName = QStringLiteral("loginToken");
    QUrlQuery query { QUrl(QString::fromUtf8(requestParts[1])).query() };
    if (!query.hasQueryItem(QueryItemName)) {
        onError("400 Bad Request", tr("No login token in SSO callback"));
        return;
    }
    qCDebug(MAIN) << "Found the token in SSO callback, logging in";
    connection->loginWithToken(query.queryItemValue(QueryItemName).toLatin1(),
                               initialDeviceName, deviceId);
    connect(connection, &Connection::connected, socket, [this] {
        const auto msg =
            tr("The application '%1' has successfully logged in as a user %2 "
               "with device id %3. This window can be closed. Thank you.\r\n")
                .arg(QCoreApplication::applicationName(), connection->userId(),
                     connection->deviceId());
        sendHttpResponse("200 OK", msg.toHtmlEscaped().toUtf8());
        socket->disconnectFromHost();
    });
    connect(connection, &Connection::loginError, socket, [this] {
        onError("401 Unauthorised", tr("Login failed"));
    });
}

void SsoSession::Private::sendHttpResponse(const QByteArray& code,
                                           const QByteArray& msg)
{
    socket->write("HTTP/1.0 ");
    socket->write(code);
    socket->write("\r\n"
                  "Content-type: text/html;charset=UTF-8\r\n"
                  "\r\n\r\n");
    socket->write(msg);
    socket->write("\r\n");
}

void SsoSession::Private::onError(const QByteArray& code,
                                  const QString& errorMsg)
{
    qCWarning(MAIN).nospace() << errorMsg;
    sendHttpResponse(code, "<h3>" + errorMsg.toUtf8() + "</h3>");
    // [kitsune] Yeah, I know, dirty. Maybe the "right" way would be to have
    // an intermediate signal but that seems just a fight for purity.
    emit connection->loginError(errorMsg, QString::fromUtf8(requestData));
    socket->disconnectFromHost();
}
