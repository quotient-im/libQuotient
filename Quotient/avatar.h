// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "util.h"

#include <QtCore/QFuture>
#include <QtCore/QUrl>
#include <QtGui/QIcon>

#include <functional>

namespace Quotient {
class Connection;

class QUOTIENT_API Avatar {
public:
    explicit Avatar();
    explicit Avatar(QUrl url);

    // TODO: use std::move_only_function once C++23 is here
    using get_callback_t = std::function<void()>;
    using upload_callback_t = std::function<void(QUrl)>;

    QImage get(Connection* connection, int dimension,
               get_callback_t callback) const;
    QImage get(Connection* connection, int w, int h,
               get_callback_t callback) const;

    [[deprecated("Use the QFuture-returning overload instead")]]
    bool upload(Connection* connection, const QString& fileName,
                upload_callback_t callback) const;
    [[deprecated("Use the QFuture-returning overload instead")]]
    bool upload(Connection* connection, QIODevice* source,
                upload_callback_t callback) const;
    QFuture<QUrl> upload(Connection* connection, const QString& fileName) const;
    QFuture<QUrl> upload(Connection* connection, QIODevice* source) const;

    QString mediaId() const;
    QUrl url() const;
    bool updateUrl(const QUrl& newUrl);

private:
    class Private;
    ImplPtr<Private> d;
};
} // namespace Quotient
