// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef Quotient_E2EE_ENABLED
#include "olm/qolmoutboundsession.h"
#include "olm/utils.h"

using namespace Quotient;

OlmError lastError(OlmOutboundGroupSession *session) {
    const std::string error_raw = olm_outbound_group_session_last_error(session);

    return fromString(error_raw);
}

QOlmOutboundGroupSession::QOlmOutboundGroupSession(OlmOutboundGroupSession *session, const QByteArray &buffer)
    : m_groupSession(session)
    , m_buffer(buffer)
{
}

QOlmOutboundGroupSession::~QOlmOutboundGroupSession()
{
    olm_clear_outbound_group_session(m_groupSession);
}

std::variant<QOlmOutboundGroupSession, OlmError> QOlmOutboundGroupSession::create()
{
    QByteArray sessionBuffer(olm_outbound_group_session_size(), '0');
    auto *olmOutboundGroupSession = olm_outbound_group_session(sessionBuffer.data());
    const auto randomLen = olm_init_outbound_group_session_random_length(olmOutboundGroupSession);
    QByteArray randomBuf = getRandom(randomLen);

    const auto error = olm_init_outbound_group_session(olmOutboundGroupSession, 
            reinterpret_cast<uint8_t *>(randomBuf.data()), randomBuf.length());

    if (error == olm_error()) {
        return lastError(olmOutboundGroupSession);
    }

    randomBuf.clear();

    return QOlmOutboundGroupSession(olmOutboundGroupSession, sessionBuffer);
}

std::variant<QByteArray, OlmError> QOlmOutboundGroupSession::pickle(const PicklingMode &mode)
{
    QByteArray pickledBuf(olm_pickle_outbound_group_session_length(m_groupSession), '0');
    QByteArray key = toKey(mode);
    const auto error = olm_pickle_outbound_group_session(m_groupSession, key.data(), key.length(),
            pickledBuf.data(), pickledBuf.length());

    if (error == olm_error()) {
        return lastError(m_groupSession);
    }

    key.clear();

    return pickledBuf;
}


std::variant<QOlmOutboundGroupSession, OlmError> QOlmOutboundGroupSession::unpickle(QByteArray &pickled, const PicklingMode &mode)
{
    QByteArray pickledBuf = pickled;
    QByteArray olmOutboundGroupSessionBuf(olm_outbound_group_session_size(), '0');
    QByteArray key = toKey(mode);
    auto olmOutboundGroupSession = olm_outbound_group_session(reinterpret_cast<uint8_t *>(olmOutboundGroupSessionBuf.data()));
    const auto error = olm_unpickle_outbound_group_session(olmOutboundGroupSession, key.data(), key.length(),
            pickled.data(), pickled.length());
    if (error == olm_error()) {
        return lastError(olmOutboundGroupSession);
    }

    key.clear();
    return QOlmOutboundGroupSession(olmOutboundGroupSession, olmOutboundGroupSessionBuf);
}

std::variant<QString, OlmError> QOlmOutboundGroupSession::encrypt(QString &plaintext)
{
    QByteArray plaintextBuf = plaintext.toUtf8();
    const auto messageMaxLen = olm_group_encrypt_message_length(m_groupSession, plaintextBuf.length());
    QByteArray messageBuf(messageMaxLen, '0');
    const auto error = olm_group_encrypt(m_groupSession, reinterpret_cast<uint8_t *>(plaintextBuf.data()),
            plaintextBuf.length(), reinterpret_cast<uint8_t *>(messageBuf.data()), messageBuf.length());

    if (error == olm_error()) {
        return lastError(m_groupSession);
    }

    return messageBuf;
}

uint32_t QOlmOutboundGroupSession::sessionMessageIndex() const
{
    return olm_outbound_group_session_message_index(m_groupSession);
}

std::variant<QByteArray, OlmError> QOlmOutboundGroupSession::sessionId() const
{
    const auto idMaxLength = olm_outbound_group_session_id_length(m_groupSession);
    QByteArray idBuffer(idMaxLength, '0');
    const auto error = olm_outbound_group_session_id(m_groupSession, reinterpret_cast<uint8_t *>(idBuffer.data()),
            idBuffer.length());
    if (error == olm_error()) {
        return lastError(m_groupSession);
    }
    return idBuffer;
}

std::variant<QByteArray, OlmError> QOlmOutboundGroupSession::sessionKey() const
{
    const auto keyMaxLength = olm_outbound_group_session_key_length(m_groupSession);
    QByteArray keyBuffer(keyMaxLength, '0');
    const auto error = olm_outbound_group_session_key(m_groupSession, reinterpret_cast<uint8_t *>(keyBuffer.data()),
            keyMaxLength);
    if (error == olm_error()) {
        return lastError(m_groupSession);
    }
    return keyBuffer;
}

#endif
