// SPDX-FileCopyrightText: 2020 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "util.h"

#include <QtCore/QUrl>
#include <QtCore/QObject>

class QTcpServer;
class QTcpSocket;

namespace Quotient {
class Connection;

/*! Single sign-on (SSO) session encapsulation
 *
 * This class is responsible for setting up of a new SSO session, providing
 * a URL to be opened (usually, in a web browser) and handling the callback
 * response after completing the single sign-on, all the way to actually
 * logging the user in. It does NOT open and render the SSO URL, it only does
 * the necessary backstage work.
 *
 * Clients only need to open the URL; the rest is done for them.
 * Client code can look something like:
 * \code
 * QDesktopServices::openUrl(
 *     connection->prepareForSso(initialDeviceName)->ssoUrl());
 * \endcode
 */
class QUOTIENT_API SsoSession : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl ssoUrl READ ssoUrl CONSTANT)
    Q_PROPERTY(QUrl callbackUrl READ callbackUrl CONSTANT)
public:
    SsoSession(Connection* connection, const QString& initialDeviceName,
               const QString& deviceId = {});
    ~SsoSession() override;
    QUrl ssoUrl() const;
    QUrl callbackUrl() const;

private:
    class Private;
    ImplPtr<Private> d;
};
} // namespace Quotient
