// SPDX-FileCopyrightText: Tobias Fella <fella@posteo.de>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtNetwork/QNetworkReply>
#include <memory>

namespace Quotient {
class Room;
class Connection;
class MxcReply : public QNetworkReply
{
public:
    MxcReply(QNetworkReply* reply, Room* room, const QString &eventId);
    MxcReply(QNetworkReply* reply);

    bool isSequential() const override;

public slots:
    void abort() override;

protected:
    qint64 readData(char *data, qint64 maxSize) override;
private:
    class Private;
    std::unique_ptr<Private> d;
};
}