// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "quotient_export.h"

#include <QtNetwork/QNetworkReply>
#include <memory>

namespace Quotient {
class Room;

class QUOTIENT_API MxcReply : public QNetworkReply
{
    Q_OBJECT
public:
    explicit MxcReply();
    explicit MxcReply(QNetworkReply *reply);
    MxcReply(QNetworkReply* reply, Room* room, const QString& eventId);
    ~MxcReply() override;

public Q_SLOTS:
    void abort() override;

protected:
    qint64 readData(char *data, qint64 maxSize) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};
}
