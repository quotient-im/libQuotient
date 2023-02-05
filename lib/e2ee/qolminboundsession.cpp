// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "qolminboundsession.h"
#include "../logging.h"

#include <cstring>
#include <iostream>
#include <olm/olm.h>

using namespace Quotient;

OlmErrorCode QOlmInboundGroupSession::lastErrorCode() const {
    return olm_inbound_group_session_last_error_code(olmData);
}

const char* QOlmInboundGroupSession::lastError() const
{
    return olm_inbound_group_session_last_error(olmData);
}

QOlmInboundGroupSession::QOlmInboundGroupSession()
    : m_groupSession(makeCStruct(olm_inbound_group_session,
                                 olm_inbound_group_session_size,
                                 olm_clear_inbound_group_session))
{}

QOlmExpected<QOlmInboundGroupSession> QOlmInboundGroupSession::create(
    const QByteArray& key)
{
    QOlmInboundGroupSession groupSession{};
    if (olm_init_inbound_group_session(groupSession.olmData,
                                       reinterpret_cast<const uint8_t*>(
                                           key.constData()),
                                       unsignedSize(key))
        == olm_error()) {
        qWarning(E2EE) << "Failed to create an inbound group session:"
                       << groupSession.lastError();
        return groupSession.lastErrorCode();
    }

    return groupSession;
}

QOlmExpected<QOlmInboundGroupSession> QOlmInboundGroupSession::importSession(
    const QByteArray& key)
{
    QOlmInboundGroupSession groupSession{};
    if (olm_import_inbound_group_session(
            groupSession.olmData, reinterpret_cast<const uint8_t*>(key.data()),
            unsignedSize(key))
        == olm_error()) {
        qWarning(E2EE) << "Failed to import an inbound group session:"
                       << groupSession.lastError();
        return groupSession.lastErrorCode();
    }

    return groupSession;
}

QByteArray QOlmInboundGroupSession::pickle(const PicklingKey& key) const
{
    const auto pickleLength = olm_pickle_inbound_group_session_length(olmData);
    auto pickledBuf = byteArrayForOlm(pickleLength);
    if (olm_pickle_inbound_group_session(olmData, key.data(), key.size(),
                                         pickledBuf.data(), pickleLength)
        == olm_error()) {
        QOLM_INTERNAL_ERROR("Failed to pickle the inbound group session");
    }
    return pickledBuf;
}

QOlmExpected<QOlmInboundGroupSession> QOlmInboundGroupSession::unpickle(
    QByteArray&& pickled, const PicklingKey& key)
{
    QOlmInboundGroupSession groupSession{};
    if (olm_unpickle_inbound_group_session(groupSession.olmData, key.data(),
                                           key.size(), pickled.data(),
                                           unsignedSize(pickled))
        == olm_error()) {
        qWarning(E2EE) << "Failed to unpickle an inbound group session:"
                       << groupSession.lastError();
        return groupSession.lastErrorCode();
    }

    return groupSession;
}

QOlmExpected<std::pair<QByteArray, uint32_t>> QOlmInboundGroupSession::decrypt(
    const QByteArray& message)
{
    // This is for capturing the output of olm_group_decrypt
    uint32_t messageIndex = 0;

    // We need to clone the message because
    // olm_decrypt_max_plaintext_length destroys the input buffer
    const auto plaintextLength = olm_group_decrypt_max_plaintext_length(
        olmData, reinterpret_cast<uint8_t*>(QByteArray(message).data()),
        unsignedSize(message));
    auto plaintextBuf = byteArrayForOlm(plaintextLength);

    const auto actualLength = olm_group_decrypt(
        olmData, reinterpret_cast<uint8_t*>(QByteArray(message).data()),
        unsignedSize(message), reinterpret_cast<uint8_t*>(plaintextBuf.data()),
        plaintextLength, &messageIndex);
    if (actualLength == olm_error()) {
        qWarning(E2EE) << "Failed to decrypt the message:" << lastError();
        return lastErrorCode();
    }

    // actualLength cannot be more than plainTextLength because the resulting
    // text would overflow the allocated memory; but it can be less, in theory
    plaintextBuf.truncate(static_cast<int>(actualLength));
    return std::pair{ plaintextBuf, messageIndex };
}

QOlmExpected<QByteArray> QOlmInboundGroupSession::exportSession(
    uint32_t messageIndex)
{
    const auto keyLength = olm_export_inbound_group_session_length(olmData);
    auto keyBuf = byteArrayForOlm(keyLength);
    if (olm_export_inbound_group_session(
            olmData, reinterpret_cast<uint8_t*>(keyBuf.data()), keyLength,
            messageIndex)
        == olm_error()) {
        QOLM_FAIL_OR_LOG(OLM_OUTPUT_BUFFER_TOO_SMALL,
                         "Failed to export the inbound group session");
        return lastErrorCode();
    }
    return keyBuf;
}

uint32_t QOlmInboundGroupSession::firstKnownIndex() const
{
    return olm_inbound_group_session_first_known_index(olmData);
}

QByteArray QOlmInboundGroupSession::sessionId() const
{
    const auto sessionIdLength = olm_inbound_group_session_id_length(olmData);
    auto sessionIdBuf = byteArrayForOlm(sessionIdLength);
    if (olm_inbound_group_session_id(
            olmData, reinterpret_cast<uint8_t*>(sessionIdBuf.data()),
            sessionIdLength)
        == olm_error())
        QOLM_INTERNAL_ERROR("Failed to obtain the group session id");

    return sessionIdBuf;
}

bool QOlmInboundGroupSession::isVerified() const
{
    return olm_inbound_group_session_is_verified(olmData) != 0;
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
