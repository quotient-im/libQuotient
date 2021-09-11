// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtNetwork/QNetworkReply>
#include <memory>

namespace Quotient {
class Room;

class MxcReply : public QNetworkReply
{
public:
    explicit MxcReply();
    explicit MxcReply(QNetworkReply *reply);
    MxcReply(QNetworkReply* reply, Room* room, const QString& eventId);

public Q_SLOTS:
    void abort() override;

protected:
    qint64 readData(char *data, qint64 maxSize) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};
}
