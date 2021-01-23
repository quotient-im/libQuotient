// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <qolminboundsession.h>

std::variant<QOlmInboundGroupSession, OlmInboundGroupSession> QOlmInboundGroupSession::create(const QString &key)
{
    auto olmInboundGroupSessionBuf = QByteArray(olm_inbound_group_session_size(), '0');

    const auto olmInboundGroupSession = olm_inbound_group_session(olmInboundGroupSessionBuf.data());

    QByteArray keyBuf = key.toUtf8();

    const auto error = olm_init_inbound_group_session(olmInboundGroupSession, keyBuf.data(), keyBuf.size());

    if (error == olm_error()) {
        return 
    }

    if create_error == errors::olm_error() {
        Err(Self::last_error(olm_inbound_group_session_ptr))
    } else {
        Ok(OlmInboundGroupSession {
            group_session_ptr: olm_inbound_group_session_ptr,
            group_session_buf: olm_inbound_group_session_buf,
        })
    }

}
