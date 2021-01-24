// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "olm/qolminboundsession.h"
#include <QDebug>
#include <iostream>
using namespace Quotient;

OlmError lastError(OlmInboundGroupSession *session) {
    const std::string error_raw = olm_inbound_group_session_last_error(session);

    std::cout << error_raw;
    return fromString(error_raw);
}

QOlmInboundGroupSession::QOlmInboundGroupSession(OlmInboundGroupSession *session, QByteArray buffer)
    : m_groupSession(session)
    , m_buffer(buffer)
{
}

QOlmInboundGroupSession::~QOlmInboundGroupSession()
{
    olm_clear_inbound_group_session(m_groupSession);
}

std::variant<QOlmInboundGroupSession, OlmError> QOlmInboundGroupSession::create(const QByteArray &key)
{
    QByteArray olmInboundGroupSessionBuf(olm_inbound_group_session_size(), '0');
    const auto olmInboundGroupSession = olm_inbound_group_session(olmInboundGroupSessionBuf.data());

    const auto temp = key;

    const auto error = olm_init_inbound_group_session(olmInboundGroupSession,
            reinterpret_cast<const uint8_t *>(temp.data()), temp.size());

    if (error == olm_error()) {
        return lastError(olmInboundGroupSession);
    }

    return QOlmInboundGroupSession(olmInboundGroupSession, std::move(olmInboundGroupSessionBuf));
}


std::variant<QOlmInboundGroupSession, OlmError> QOlmInboundGroupSession::import(const QByteArray &key)
{
    QByteArray olmInboundGroupSessionBuf(olm_inbound_group_session_size(), '0');
    const auto olmInboundGroupSession = olm_inbound_group_session(olmInboundGroupSessionBuf.data());
    QByteArray keyBuf = key;

    const auto error = olm_import_inbound_group_session(olmInboundGroupSession,
            reinterpret_cast<const uint8_t *>(keyBuf.data()), keyBuf.size());
    if (error == olm_error()) {
        return lastError(olmInboundGroupSession);
    }

    return QOlmInboundGroupSession(olmInboundGroupSession, std::move(olmInboundGroupSessionBuf));
}

QByteArray toKey(const PicklingMode &mode)
{
    if (std::holds_alternative<Unencrypted>(mode)) {
        return "";
    }
    return std::get<Encrypted>(mode).key;
}

std::variant<QByteArray, OlmError> QOlmInboundGroupSession::pickle(const PicklingMode &mode) const
{
    QByteArray pickledBuf(olm_pickle_inbound_group_session_length(m_groupSession), '0');
    const QByteArray key = toKey(mode);
    const auto error = olm_pickle_inbound_group_session(m_groupSession, key.data(), key.length(), pickledBuf.data(),
            pickledBuf.length());
    if (error == olm_error()) {
        return lastError(m_groupSession);
    }
    return pickledBuf;
}

std::variant<QOlmInboundGroupSession, OlmError> QOlmInboundGroupSession::unpickle(QByteArray &picked, const PicklingMode &mode)
{
    QByteArray groupSessionBuf(olm_inbound_group_session_size(), '0');
    auto groupSession = olm_inbound_group_session(groupSessionBuf.data());
    const QByteArray key = toKey(mode);
    const auto error = olm_unpickle_inbound_group_session(groupSession, key.data(), key.length(), picked.data(), picked.size());
    if (error == olm_error()) {
        return lastError(groupSession);
    }
    return QOlmInboundGroupSession(groupSession, std::move(groupSessionBuf));
}

std::variant<std::pair<QString, uint32_t>, OlmError> QOlmInboundGroupSession::decrypt(QString &message)
{
    // This is for capturing the output of olm_group_decrypt
    uint32_t messageIndex = 0;

    // We need to clone the message because
    // olm_decrypt_max_plaintext_length destroys the input buffer
    QByteArray messageBuf = message.toUtf8();

    QByteArray plaintextBuf(olm_group_decrypt_max_plaintext_length(m_groupSession,
                reinterpret_cast<uint8_t *>(messageBuf.data()), messageBuf.length()), '0');
    const auto messageLen = messageBuf.length();
    const auto plaintextMaxLen =  plaintextBuf.length();

    const auto plaintextLen = olm_group_decrypt(m_groupSession, reinterpret_cast<uint8_t *>(messageBuf.data()),
            messageLen, reinterpret_cast<uint8_t *>(plaintextBuf.data()), plaintextMaxLen, &messageIndex);

    // Error code or plaintext length is returned
    const auto decryptError = plaintextLen;

    if (decryptError == olm_error()) {
        return lastError(m_groupSession);
    }

    plaintextBuf.truncate(plaintextLen);
    return std::make_pair<QString, qint32>(QString(plaintextBuf), messageIndex);
}

std::variant<QByteArray, OlmError> QOlmInboundGroupSession::exportSession(uint32_t messageIndex)
{
    const auto keyLen = olm_export_inbound_group_session_length(m_groupSession);
    QByteArray keyBuf(keyLen, '0');
    const auto error = olm_export_inbound_group_session(m_groupSession, reinterpret_cast<uint8_t *>(keyBuf.data()), keyLen, messageIndex);

    if (error == olm_error()) {
        return lastError(m_groupSession);
    }
    return keyBuf;
}

uint32_t QOlmInboundGroupSession::firstKnownIndex() const
{
    return olm_inbound_group_session_first_known_index(m_groupSession);
}

std::variant<QByteArray, OlmError> QOlmInboundGroupSession::sessionId() const
{
    QByteArray sessionIdBuf(olm_inbound_group_session_id_length(m_groupSession), '0');
    const auto error = olm_inbound_group_session_id(m_groupSession, reinterpret_cast<uint8_t *>(sessionIdBuf.data()),
            sessionIdBuf.length());
    if (error == olm_error()) {
        return lastError(m_groupSession);
    }
    return sessionIdBuf;
}

bool QOlmInboundGroupSession::isVerified() const
{
    return olm_inbound_group_session_is_verified(m_groupSession) != 0;
}
