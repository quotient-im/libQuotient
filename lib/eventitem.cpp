// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "eventitem.h"

#include "events/roomavatarevent.h"
#include "events/roommessageevent.h"

using namespace Quotient;

void PendingEventItem::setFileUploaded(const QUrl& remoteUrl)
{
    // TODO: eventually we might introduce hasFileContent to RoomEvent,
    // and unify the code below.
    if (auto* rme = getAs<RoomMessageEvent>()) {
        Q_ASSERT(rme->hasFileContent());
        rme->editContent([remoteUrl](EventContent::TypedBase& ec) {
            ec.fileInfo()->url = remoteUrl;
        });
    }
    if (auto* rae = getAs<RoomAvatarEvent>()) {
        Q_ASSERT(rae->content().fileInfo());
        rae->editContent(
            [remoteUrl](EventContent::FileInfo& fi) { fi.url = remoteUrl; });
    }
    setStatus(EventStatus::FileUploaded);
}

// Not exactly sure why but this helps with the linker not finding
// Quotient::EventStatus::staticMetaObject when building Quaternion
#include "moc_eventitem.cpp"
