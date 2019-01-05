/******************************************************************************
 * Copyright (C) 2018 Kitsune Ral <kitsune-ral@users.sf.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "eventitem.h"

#include "events/roommessageevent.h"
#include "events/roomavatarevent.h"

using namespace QMatrixClient;

void PendingEventItem::setFileUploaded(const QUrl& remoteUrl)
{
    // TODO: eventually we might introduce hasFileContent to RoomEvent,
    // and unify the code below.
    if (auto* rme = getAs<RoomMessageEvent>())
    {
        Q_ASSERT(rme->hasFileContent());
        rme->editContent([remoteUrl] (EventContent::TypedBase& ec) {
            ec.fileInfo()->url = remoteUrl;
        });
    }
    if (auto* rae = getAs<RoomAvatarEvent>())
    {
        Q_ASSERT(rae->content().fileInfo());
        rae->editContent([remoteUrl] (EventContent::FileInfo& fi) {
            fi.url = remoteUrl;
        });
    }
    setStatus(EventStatus::FileUploaded);
}
