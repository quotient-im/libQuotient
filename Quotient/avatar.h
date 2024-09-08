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
    explicit Avatar(Connection* parent, const QUrl& url = {});

#ifdef __cpp_lib_move_only_function // AppleClang 15 doesn't have it
    using get_callback_t = std::move_only_function<void()>;
    using upload_callback_t = std::move_only_function<void(QUrl)>;
#else
    using get_callback_t = std::function<void()>;
    using upload_callback_t = std::function<void(QUrl)>;
#endif


    QImage get(int dimension, get_callback_t callback) const;
    QImage get(int w, int h, get_callback_t callback) const;

    [[deprecated("Use the QFuture-returning overload instead")]]
    bool upload(const QString& fileName, upload_callback_t callback) const;
    [[deprecated("Use the QFuture-returning overload instead")]]
    bool upload(QIODevice* source, upload_callback_t callback) const;
    QFuture<QUrl> upload(const QString& fileName) const;
    QFuture<QUrl> upload(QIODevice* source) const;

    bool isEmpty() const;
    QString mediaId() const;
    QUrl url() const;
    bool updateUrl(const QUrl& newUrl);

    static bool isUrlValid(const QUrl& u);

private:
    class Private;
    ImplPtr<Private> d;
};
} // namespace Quotient
