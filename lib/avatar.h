// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtCore/QUrl>
#include <QtGui/QIcon>

#include <functional>
#include <memory>

namespace Quotient {
class Connection;

class Avatar {
public:
    explicit Avatar();
    explicit Avatar(QUrl url);
    Avatar(Avatar&&);
    ~Avatar();
    Avatar& operator=(Avatar&&);

    using get_callback_t = std::function<void()>;
    using upload_callback_t = std::function<void(QUrl)>;

    QImage get(Connection* connection, int dimension,
               get_callback_t callback) const;
    QImage get(Connection* connection, int w, int h,
               get_callback_t callback) const;

    bool upload(Connection* connection, const QString& fileName,
                upload_callback_t callback) const;
    bool upload(Connection* connection, QIODevice* source,
                upload_callback_t callback) const;

    QString mediaId() const;
    QUrl url() const;
    bool updateUrl(const QUrl& newUrl);

private:
    class Private;
    std::unique_ptr<Private> d;
};
} // namespace Quotient
/// \deprecated Use namespace Quotient instead
namespace QMatrixClient = Quotient;
