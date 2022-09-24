// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "qolmoutboundsession.h"

#include "logging.h"
#include "qolmutils.h"

#include <olm/olm.h>

using namespace Quotient;

OlmErrorCode QOlmOutboundGroupSession::lastErrorCode() const {
    return olm_outbound_group_session_last_error_code(m_groupSession);
}

const char* QOlmOutboundGroupSession::lastError() const
{
    return olm_outbound_group_session_last_error(m_groupSession);
}

QOlmOutboundGroupSession::QOlmOutboundGroupSession(OlmOutboundGroupSession *session)
    : m_groupSession(session)
{}

QOlmOutboundGroupSession::~QOlmOutboundGroupSession()
{
    olm_clear_outbound_group_session(m_groupSession);
    delete[](reinterpret_cast<uint8_t *>(m_groupSession));
}

QOlmOutboundGroupSessionPtr QOlmOutboundGroupSession::create()
{
    auto *olmOutboundGroupSession = olm_outbound_group_session(new uint8_t[olm_outbound_group_session_size()]);
    if (const auto randomLength = olm_init_outbound_group_session_random_length(
            olmOutboundGroupSession);
        olm_init_outbound_group_session(olmOutboundGroupSession,
                                        RandomBuffer(randomLength).bytes(),
                                        randomLength)
        == olm_error()) {
        // FIXME: create the session object earlier
        QOLM_INTERNAL_ERROR_X("Failed to initialise an outbound group session",
                              olm_outbound_group_session_last_error(
                                  olmOutboundGroupSession));
    }

    return std::make_unique<QOlmOutboundGroupSession>(olmOutboundGroupSession);
}

QByteArray QOlmOutboundGroupSession::pickle(const PicklingMode &mode) const
{
    QByteArray pickledBuf(
        olm_pickle_outbound_group_session_length(m_groupSession), '\0');
    auto key = toKey(mode);
    if (olm_pickle_outbound_group_session(m_groupSession, key.data(),
                                          key.length(), pickledBuf.data(),
                                          pickledBuf.length())
        == olm_error())
        QOLM_INTERNAL_ERROR("Failed to pickle the outbound group session");

    key.clear();
    return pickledBuf;
}

QOlmExpected<QOlmOutboundGroupSessionPtr> QOlmOutboundGroupSession::unpickle(
    QByteArray&& pickled, const PicklingMode& mode)
{
    auto *olmOutboundGroupSession = olm_outbound_group_session(new uint8_t[olm_outbound_group_session_size()]);
    auto key = toKey(mode);
    if (olm_unpickle_outbound_group_session(olmOutboundGroupSession, key.data(),
                                            key.length(), pickled.data(),
                                            pickled.length())
        == olm_error()) {
        // FIXME: create the session object earlier and use lastError()
        qWarning(E2EE) << "Failed to unpickle an outbound group session:"
                       << olm_outbound_group_session_last_error(
                              olmOutboundGroupSession);
        return olm_outbound_group_session_last_error_code(
            olmOutboundGroupSession);
    }

    key.clear();
    return std::make_unique<QOlmOutboundGroupSession>(olmOutboundGroupSession);
}

QByteArray QOlmOutboundGroupSession::encrypt(const QByteArray& plaintext) const
{
    const auto messageMaxLength =
        olm_group_encrypt_message_length(m_groupSession, plaintext.length());
    QByteArray messageBuf(messageMaxLength, '\0');
    if (olm_group_encrypt(m_groupSession,
                          reinterpret_cast<const uint8_t*>(plaintext.data()),
                          plaintext.length(),
                          reinterpret_cast<uint8_t*>(messageBuf.data()),
                          messageBuf.length())
        == olm_error())
        QOLM_INTERNAL_ERROR("Failed to encrypt a message");

    return messageBuf;
}

uint32_t QOlmOutboundGroupSession::sessionMessageIndex() const
{
    return olm_outbound_group_session_message_index(m_groupSession);
}

QByteArray QOlmOutboundGroupSession::sessionId() const
{
    const auto idMaxLength = olm_outbound_group_session_id_length(m_groupSession);
    QByteArray idBuffer(idMaxLength, '\0');
    if (olm_outbound_group_session_id(
            m_groupSession, reinterpret_cast<uint8_t*>(idBuffer.data()),
            idBuffer.length())
        == olm_error())
        QOLM_INTERNAL_ERROR("Failed to obtain group session id");

    return idBuffer;
}

QByteArray QOlmOutboundGroupSession::sessionKey() const
{
    const auto keyMaxLength = olm_outbound_group_session_key_length(m_groupSession);
    QByteArray keyBuffer(keyMaxLength, '\0');
    if (olm_outbound_group_session_key(
            m_groupSession, reinterpret_cast<uint8_t*>(keyBuffer.data()),
            keyMaxLength)
        == olm_error())
        QOLM_INTERNAL_ERROR("Failed to obtain group session key");

    return keyBuffer;
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
