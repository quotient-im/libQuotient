// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "qolminboundsession.h"
#include "e2ee/e2ee_common.h"

#include "../logging_categories_p.h"

#include <cstring>

using namespace Quotient;

QOlmInboundGroupSession::QOlmInboundGroupSession(rust::Box<megolm::InboundGroupSession> session)
    : olmData(std::move(session))
{}

QOlmExpected<QOlmInboundGroupSession> QOlmInboundGroupSession::create(const QByteArray& key)
{
    return QOlmInboundGroupSession(megolm::new_inbound_group_session(*megolm::session_key_from_base64(rust::String(key.data(), key.size()))));
}

QOlmExpected<QOlmInboundGroupSession> QOlmInboundGroupSession::importSession(const QByteArray& key)
{
    return QOlmInboundGroupSession(megolm::import_inbound_group_session(*megolm::exported_session_key_from_base64(rust::String(key.data(), key.size()))));
}

QByteArray QOlmInboundGroupSession::pickle(const PicklingKey& key) const
{
    //TODO: This is terrible :(
    std::array<uint8_t, 32> _key;
    std::copy(key.data(), key.data() + 32, _key.begin());
    auto data = olmData->pickle(_key);
    return {data.data(), (qsizetype) data.size()};
}

QOlmExpected<QOlmInboundGroupSession> QOlmInboundGroupSession::unpickle(
    QByteArray&& pickled, const PicklingKey& key)
{
    const auto &keyBytes = viewAsByteArray(key);

    //TODO: This is terrible :(
    std::array<std::uint8_t, 32> _key;
    std::copy(key.data(), key.data() + 32, _key.begin());
    auto session = megolm::inbound_group_session_from_pickle(rust::String(pickled.data(), pickled.size()), _key);
    return QOlmInboundGroupSession(std::move(session));
}

QOlmExpected<std::pair<QByteArray, uint32_t>> QOlmInboundGroupSession::decrypt(
    const QByteArray& message)
{
    auto plaintext = olmData->decrypt(*megolm::megolm_message_from_base64(::rust::Str(message.data(), message.size())));
    return std::pair{ QByteArray { plaintext.plaintext.data(), (qsizetype) plaintext.plaintext.size() }, plaintext.message_index };
}

QOlmExpected<QByteArray> QOlmInboundGroupSession::exportSession(
    uint32_t messageIndex)
{
    auto data = olmData->export_at(messageIndex)->to_base64();
    return QByteArray {data.data(), (qsizetype) data.size()};
}

uint32_t QOlmInboundGroupSession::firstKnownIndex() const
{
    return olmData->first_known_index();
}

QByteArray QOlmInboundGroupSession::sessionId() const
{
    auto id = olmData->session_id();
    return {id.data(), (qsizetype) id.length()};
}

// bool QOlmInboundGroupSession::isVerified() const
// {
//     return olmData->is
// }

QByteArray QOlmInboundGroupSession::olmSessionId() const
{
    return m_olmSessionId;
}
void QOlmInboundGroupSession::setOlmSessionId(const QByteArray& newOlmSessionId)
{
    m_olmSessionId = newOlmSessionId;
}

QString QOlmInboundGroupSession::senderId() const
{
    return m_senderId;
}
void QOlmInboundGroupSession::setSenderId(const QString& senderId)
{
    m_senderId = senderId;
}
