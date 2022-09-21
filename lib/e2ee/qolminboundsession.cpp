// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "qolminboundsession.h"
#include "qolmutils.h"
#include "../logging.h"

#include <cstring>
#include <iostream>
#include <olm/olm.h>

using namespace Quotient;

OlmErrorCode QOlmInboundGroupSession::lastErrorCode() const {
    return olm_inbound_group_session_last_error_code(m_groupSession);
}

const char* QOlmInboundGroupSession::lastError() const
{
    return olm_inbound_group_session_last_error(m_groupSession);
}

QOlmInboundGroupSession::QOlmInboundGroupSession(OlmInboundGroupSession *session)
    : m_groupSession(session)
{}

QOlmInboundGroupSession::~QOlmInboundGroupSession()
{
    olm_clear_inbound_group_session(m_groupSession);
    //delete[](reinterpret_cast<uint8_t *>(m_groupSession));
}

std::unique_ptr<QOlmInboundGroupSession> QOlmInboundGroupSession::create(const QByteArray &key)
{
    const auto olmInboundGroupSession = olm_inbound_group_session(new uint8_t[olm_inbound_group_session_size()]);
    if (olm_init_inbound_group_session(
            olmInboundGroupSession,
            reinterpret_cast<const uint8_t*>(key.constData()), key.size())
        == olm_error()) {
        // FIXME: create QOlmInboundGroupSession earlier and use lastErrorCode()
        throw olm_inbound_group_session_last_error_code(olmInboundGroupSession);
    }

    return std::make_unique<QOlmInboundGroupSession>(olmInboundGroupSession);
}

std::unique_ptr<QOlmInboundGroupSession> QOlmInboundGroupSession::import(const QByteArray &key)
{
    const auto olmInboundGroupSession = olm_inbound_group_session(new uint8_t[olm_inbound_group_session_size()]);

    if (olm_import_inbound_group_session(
            olmInboundGroupSession,
            reinterpret_cast<const uint8_t*>(key.data()), key.size())
        == olm_error()) {
        // FIXME: create QOlmInboundGroupSession earlier and use lastError()
        throw olm_inbound_group_session_last_error_code(olmInboundGroupSession);
    }

    return std::make_unique<QOlmInboundGroupSession>(olmInboundGroupSession);
}

QByteArray QOlmInboundGroupSession::pickle(const PicklingMode& mode) const
{
    QByteArray pickledBuf(
        olm_pickle_inbound_group_session_length(m_groupSession), '\0');
    if (const auto key = toKey(mode);
        olm_pickle_inbound_group_session(m_groupSession, key.data(),
                                         key.length(), pickledBuf.data(),
                                         pickledBuf.length())
        == olm_error()) {
        throw lastError();
    }
    return pickledBuf;
}

QOlmExpected<QOlmInboundGroupSessionPtr> QOlmInboundGroupSession::unpickle(
    QByteArray&& pickled, const PicklingMode& mode)
{
    const auto groupSession = olm_inbound_group_session(new uint8_t[olm_inbound_group_session_size()]);
    auto key = toKey(mode);
    if (olm_unpickle_inbound_group_session(groupSession, key.data(),
                                           key.length(), pickled.data(),
                                           pickled.size())
        == olm_error()) {
        // FIXME: create QOlmInboundGroupSession earlier and use lastError()
        return olm_inbound_group_session_last_error_code(groupSession);
    }
    key.clear();

    return std::make_unique<QOlmInboundGroupSession>(groupSession);
}

QOlmExpected<std::pair<QByteArray, uint32_t>> QOlmInboundGroupSession::decrypt(
    const QByteArray& message)
{
    // This is for capturing the output of olm_group_decrypt
    uint32_t messageIndex = 0;

    // We need to clone the message because
    // olm_decrypt_max_plaintext_length destroys the input buffer
    QByteArray messageBuf(message.length(), '\0');
    std::copy(message.begin(), message.end(), messageBuf.begin());

    QByteArray plaintextBuf(olm_group_decrypt_max_plaintext_length(
                                m_groupSession,
                                reinterpret_cast<uint8_t*>(messageBuf.data()),
                                messageBuf.length()),
                            '\0');

    messageBuf = QByteArray(message.length(), '\0');
    std::copy(message.begin(), message.end(), messageBuf.begin());

    const auto plaintextLen = olm_group_decrypt(m_groupSession, reinterpret_cast<uint8_t *>(messageBuf.data()),
            messageBuf.length(), reinterpret_cast<uint8_t *>(plaintextBuf.data()), plaintextBuf.length(), &messageIndex);
    if (plaintextLen == olm_error()) {
        qWarning(E2EE) << "Failed to decrypt the message:" << lastError();
        return lastErrorCode();
    }

    QByteArray output(plaintextLen, '\0');
    std::memcpy(output.data(), plaintextBuf.data(), plaintextLen);

    return std::make_pair(output, messageIndex);
}

QOlmExpected<QByteArray> QOlmInboundGroupSession::exportSession(
    uint32_t messageIndex)
{
    const auto keyLength = olm_export_inbound_group_session_length(m_groupSession);
    QByteArray keyBuf(keyLength, '\0');
    if (olm_export_inbound_group_session(
            m_groupSession, reinterpret_cast<uint8_t*>(keyBuf.data()),
            keyLength, messageIndex)
        == olm_error()) {
        return lastErrorCode();
    }
    return keyBuf;
}

uint32_t QOlmInboundGroupSession::firstKnownIndex() const
{
    return olm_inbound_group_session_first_known_index(m_groupSession);
}

QByteArray QOlmInboundGroupSession::sessionId() const
{
    QByteArray sessionIdBuf(olm_inbound_group_session_id_length(m_groupSession),
                            '\0');
    if (olm_inbound_group_session_id(
            m_groupSession, reinterpret_cast<uint8_t*>(sessionIdBuf.data()),
            sessionIdBuf.length())
        == olm_error()) {
        throw lastError();
    }
    return sessionIdBuf;
}

bool QOlmInboundGroupSession::isVerified() const
{
    return olm_inbound_group_session_is_verified(m_groupSession) != 0;
}

QString QOlmInboundGroupSession::olmSessionId() const
{
    return m_olmSessionId;
}
void QOlmInboundGroupSession::setOlmSessionId(const QString& newOlmSessionId)
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
