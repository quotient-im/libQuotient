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

#include "filetransfer.h"

#include "roomcontroller.h"
#include "csapi/content-repo.h"
#include "jobs/downloadfilejob.h"

#include <QtCore/QPointer>
#include <QtCore/QFileInfo>

#include <cmath>

#if !(defined __GLIBCXX__ && __GLIBCXX__ <= 20150123)
using std::llround;
#endif

using namespace Quotient;

class FileTransfer::Private {
public:
    Private(BaseJob* j, const QString& fileName, bool isUploading)
        : job(j), localFileInfo(fileName), isUpload(isUploading)
    {}

    QPointer<BaseJob> job;
    QFileInfo localFileInfo;
    bool isUpload;
    Status status = Started;
    qint64 progress = 0;
    qint64 total = -1;
};

FileTransfer::FileTransfer(DownloadFileJob* j, const QUrl& fileUrl)
    : d(new Private(j, j->targetFileName(), false))
{
    if (!isJobRunning(j)) {
        d->status = Failed;
        qCWarning(MAIN).noquote() << "Download could not start";
        return;
    }
    connect(j, &BaseJob::downloadProgress, this, &FileTransfer::updateProgress);
    connect(j, &BaseJob::success, this, [this, fileUrl] {
        complete(fileUrl, localPath());
    });
    connect(j, &BaseJob::failure, this, &FileTransfer::fail);
}

FileTransfer::FileTransfer(UploadContentJob* j, const QString& fileName)
    : d(new Private(j, fileName, true))
{
    if (!isJobRunning(j)) {
        d->status = Failed;
        qCWarning(MAIN).noquote() << "Upload could not start";
        return;
    }
    connect(j, &BaseJob::downloadProgress, this, &FileTransfer::updateProgress);
    connect(j, &BaseJob::success, this, [this, j] {
        complete(localPath(), j->contentUri());
    });
    connect(j, &BaseJob::failure, this, &FileTransfer::fail);
}

FileTransfer::~FileTransfer()
{
    if (status() == Started)
        cancel();
}

bool FileTransfer::isUpload() const { return d->isUpload; }

QUrl FileTransfer::localDir() const
{
    return QUrl::fromLocalFile(d->localFileInfo.absolutePath());
}

QUrl FileTransfer::localPath() const
{
    return QUrl::fromLocalFile(d->localFileInfo.absoluteFilePath());
}

FileTransfer::Status FileTransfer::status() const { return d->status; }

bool FileTransfer::isActive() const
{
    return d->status == Started || d->status == Completed;
}

BaseJob* FileTransfer::job() const { return d->job; }

static constexpr auto int_max = qint64(std::numeric_limits<int>::max());

int FileTransfer::progress() const
{
    return int(d->total > int_max
                   ? llround(double(d->progress) / d->total * int_max)
                   : d->progress);
}

int FileTransfer::totalSize() const
{
    return int(std::min(d->total, int_max));
}

void FileTransfer::cancel()
{
    if (status() == Cancelled)
        return;

    if (isJobRunning(job()))
        job()->abandon();
    d->status = Cancelled;
    emit cancelled();
    deleteLater();
}

void FileTransfer::updateProgress(qint64 p, qint64 t)
{
    if (t == 0) {
        t = -1;
        if (p == 0)
            p = -1;
    }
    if (PROFILER().isDebugEnabled() && p != -1) {
        if (t != -1)
            qCDebug(PROFILER).nospace()
                << "Transfer progress: " << p << "b/" << t
                << "b = " << llround(double(p) / t * 100) << "%";
        else
            qCDebug(PROFILER).nospace() << "Transfer progress: " << p << "b";
    }
    d->progress = p;
    d->total = t;
    emit progressUpdated(progress(), totalSize());
}

void FileTransfer::complete(QUrl from, QUrl to)
{
    d->status = Completed;
    emit statusChanged();
    emit completed(from, to);
}

void FileTransfer::fail(BaseJob* j)
{
    const auto& caption = d->isUpload ? QStringLiteral("upload from")
                                      : QStringLiteral("download to");
    d->status = Failed;
    qCWarning(MAIN).noquote() << "Failed" << caption << localPath();
    const auto errorMessage = j->errorString();
    if (!errorMessage.isEmpty())
        qCWarning(MAIN) << "Message:" << errorMessage;
    emit failed(errorMessage);
}
