// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "qolmoutboundsession.h"

#include "../logging_categories_p.h"

using namespace Quotient;

QOlmOutboundGroupSession::QOlmOutboundGroupSession(rust::Box<megolm::GroupSession> session)
    : olmData(std::move(session))
{
}

QByteArray QOlmOutboundGroupSession::pickle(const PicklingKey& key) const
{
    //TODO: This is terrible :(
    std::array<std::uint8_t, 32> _key;
    std::copy(key.data(), key.data() + 32, _key.begin());
    auto pickle = olmData->pickle(_key);
    return {pickle.data(), (qsizetype) pickle.size()};
}

QOlmExpected<QOlmOutboundGroupSession> QOlmOutboundGroupSession::unpickle(
    QByteArray&& pickled, const PicklingKey& key)
{
    //TODO: This is terrible :(
    std::array<std::uint8_t, 32> _key;
    std::copy(key.data(), key.data() + 32, _key.begin());
    auto session = megolm::group_session_from_pickle(rust::String(pickled.data(), pickled.size()), _key);

    return QOlmOutboundGroupSession(std::move(session));
}

QByteArray QOlmOutboundGroupSession::encrypt(const QByteArray& plaintext)
{
    auto data = olmData->encrypt(rust::String(plaintext.data(), plaintext.size()))->to_base64();
    return {data.data(), (qsizetype) data.size()};
}

uint32_t QOlmOutboundGroupSession::sessionMessageIndex() const
{
    return olmData->message_index();
}

QByteArray QOlmOutboundGroupSession::sessionId() const
{
    auto id = olmData->session_id();
    return {id.data(), (qsizetype) id.size()};
}

QByteArray QOlmOutboundGroupSession::sessionKey() const
{
    auto key = olmData->session_key()->to_base64();
    return {key.data(), (qsizetype) key.size()};
}

int QOlmOutboundGroupSession::messageCount() const
{
    return m_messageCount;
}

void QOlmOutboundGroupSession::setMessageCount(int messageCount)
{
    m_messageCount = messageCount;
}

QDateTime QOlmOutboundGroupSession::creationTime() const
{
    return m_creationTime;
}

void QOlmOutboundGroupSession::setCreationTime(const QDateTime& creationTime)
{
    m_creationTime = creationTime;
}

QOlmExpected<QOlmOutboundGroupSession> QOlmOutboundGroupSession::create()
{
    return QOlmOutboundGroupSession(megolm::new_group_session());
}
