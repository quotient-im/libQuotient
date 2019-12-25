/******************************************************************************
 * Copyright (C) 2019 The Quotient project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#pragma once

#include <QtCore/QObject>
#include <QtCore/QUrl>

namespace Quotient {
class BaseJob;
class DownloadFileJob;
class UploadContentJob;

/** The data structure used to expose file transfer information to views
 *
 * This is specifically tuned to work with QML exposing all traits as
 * Q_PROPERTY values.
 */
class FileTransfer : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isUpload READ isUpload CONSTANT)
    Q_PROPERTY(QUrl localDir READ localDir CONSTANT)
    Q_PROPERTY(QUrl localPath READ localPath CONSTANT)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool isActive READ isActive NOTIFY statusChanged)
    Q_PROPERTY(BaseJob* job READ job NOTIFY statusChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressUpdated)
    Q_PROPERTY(int total READ totalSize NOTIFY progressUpdated)

public:
    enum Status { Started, Completed, Failed, Cancelled };
    Q_ENUM(Status)

    bool isUpload() const;
    QUrl localDir() const;
    QUrl localPath() const;
    Status status() const;
    bool isActive() const;
    BaseJob* job() const;
    // JavaScript (and therefore QML) cannot handle 64-bit integers, so use int
    int progress() const;
    int totalSize() const;

public slots:
    /// Interrupt the transfer, emit cancelled(), and self-destruct
    /*! Use this instead of deleting the object directly */
    void cancel();
    /// Interrupt the transfer and restart the

signals:
    void statusChanged();
    /// The network job has reported progress on the file transfer
    void progressUpdated(qint64 newProgress, qint64 total);
    /// The transfer has been completed
    void completed(QUrl from, QUrl to);
    /// The transfer has failed
    void failed(QString errorMessage = {});
    /// The transfer has been cancelled
    /*! This differs from failing in that the transfer record is immediately
     * deleted, while the failed transfer record requires explicit deletion */
    void cancelled();

private:
    class Private;
    QScopedPointer<Private> d;

    friend class RoomController;
    // Use uploadFile()/downloadFile() from RoomController to make FileTransfer
    explicit FileTransfer(DownloadFileJob *j, const QUrl &fileUrl);
    explicit FileTransfer(UploadContentJob* j, const QString& fileName);
    // To delete an object from outside, use cancel()
    ~FileTransfer() override;
    void updateProgress(qint64 p, qint64 t);
    void complete(QUrl from, QUrl to);
    void fail(BaseJob* j);
};

}
