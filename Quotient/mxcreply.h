// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "events/filesourceinfo.h"

#include <QtNetwork/QNetworkReply>

namespace Quotient {
class QUOTIENT_API MxcReply : public QNetworkReply
{
    Q_OBJECT
public:
    enum DeferredFlag { Deferred };
    enum ErrorFlag { Error };

    explicit MxcReply(QNetworkReply* reply,
                      const EncryptedFileMetadata& fileMetadata);
    explicit MxcReply(DeferredFlag);
    explicit MxcReply(ErrorFlag);

    void setNetworkReply(QNetworkReply* newReply,
                         const EncryptedFileMetadata& fileMetadata = {});

    qint64 bytesAvailable() const override;

public Q_SLOTS:
    void abort() override;

protected:
    qint64 readData(char* data, qint64 maxSize) override;

private:
    class Private;
    ImplPtr<Private> d = ZeroImpl<Private>();
};
} // namespace Quotient
