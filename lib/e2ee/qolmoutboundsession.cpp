// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "qolmoutboundsession.h"

#include "logging.h"

#include <olm/olm.h>

using namespace Quotient;

OlmErrorCode QOlmOutboundGroupSession::lastErrorCode() const {
    return olm_outbound_group_session_last_error_code(olmData);
}

const char* QOlmOutboundGroupSession::lastError() const
{
    return olm_outbound_group_session_last_error(olmData);
}

QOlmOutboundGroupSession::QOlmOutboundGroupSession()
    : m_groupSession(makeCStruct(olm_outbound_group_session,
                                 olm_outbound_group_session_size,
                                 olm_clear_outbound_group_session))
{
    if (const auto randomLength =
            olm_init_outbound_group_session_random_length(olmData);
        olm_init_outbound_group_session(olmData,
                                        RandomBuffer(randomLength).bytes(),
                                        randomLength)
        == olm_error()) {
        QOLM_INTERNAL_ERROR("Failed to initialise an outbound group session");
    }
}

QByteArray QOlmOutboundGroupSession::pickle(const PicklingMode &mode) const
{
    const auto pickleLength =
        olm_pickle_outbound_group_session_length(olmData);
    QByteArray pickledBuf(pickleLength, '\0');
    auto key = toKey(mode);
    if (olm_pickle_outbound_group_session(olmData, key.data(), key.length(),
                                          pickledBuf.data(), pickleLength)
        == olm_error())
        QOLM_INTERNAL_ERROR("Failed to pickle the outbound group session");

    key.clear();
    return pickledBuf;
}

QOlmExpected<QOlmOutboundGroupSession> QOlmOutboundGroupSession::unpickle(
    QByteArray&& pickled, const PicklingMode& mode)
{
    QOlmOutboundGroupSession groupSession{};
    auto key = toKey(mode);
    if (olm_unpickle_outbound_group_session(groupSession.olmData, key.data(),
                                            key.length(), pickled.data(),
                                            pickled.length())
        == olm_error()) {
        qWarning(E2EE) << "Failed to unpickle an outbound group session:"
                       << groupSession.lastError();
        return groupSession.lastErrorCode();
    }

    key.clear();
    return groupSession;
}

QByteArray QOlmOutboundGroupSession::encrypt(const QByteArray& plaintext) const
{
    const auto messageMaxLength =
        olm_group_encrypt_message_length(olmData, plaintext.length());
    QByteArray messageBuf(messageMaxLength, '\0');
    if (olm_group_encrypt(olmData,
                          reinterpret_cast<const uint8_t*>(plaintext.data()),
                          plaintext.length(),
                          reinterpret_cast<uint8_t*>(messageBuf.data()),
                          messageMaxLength)
        == olm_error())
        QOLM_INTERNAL_ERROR("Failed to encrypt a message");

    return messageBuf;
}

uint32_t QOlmOutboundGroupSession::sessionMessageIndex() const
{
    return olm_outbound_group_session_message_index(olmData);
}

QByteArray QOlmOutboundGroupSession::sessionId() const
{
    const auto idMaxLength = olm_outbound_group_session_id_length(olmData);
    QByteArray idBuffer(idMaxLength, '\0');
    if (olm_outbound_group_session_id(
            olmData, reinterpret_cast<uint8_t*>(idBuffer.data()), idMaxLength)
        == olm_error())
        QOLM_INTERNAL_ERROR("Failed to obtain group session id");

    return idBuffer;
}

QByteArray QOlmOutboundGroupSession::sessionKey() const
{
    const auto keyMaxLength = olm_outbound_group_session_key_length(olmData);
    QByteArray keyBuffer(keyMaxLength, '\0');
    if (olm_outbound_group_session_key(
            olmData, reinterpret_cast<uint8_t*>(keyBuffer.data()), keyMaxLength)
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
