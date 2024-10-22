// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "eventitem.h"

#include "events/roomavatarevent.h"
#include "events/roommessageevent.h"

using namespace Quotient;

void PendingEventItem::setFileUploaded(const FileSourceInfo& uploadedFileData)
{
    if (auto* rme = getAs<RoomMessageEvent>())
        rme->updateFileSourceInfo(uploadedFileData);

    if (auto* rae = getAs<RoomAvatarEvent>()) {
        rae->editContent([&uploadedFileData](EventContent::FileInfo& fi) {
            fi.source = uploadedFileData;
        });
    }
    setStatus(EventStatus::FileUploaded);
}

// Not exactly sure why but this helps with the linker not finding
// Quotient::EventStatus::staticMetaObject when building Quaternion
#include "moc_eventitem.cpp" // NOLINT(bugprone-suspicious-include)
