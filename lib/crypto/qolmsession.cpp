// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef Quotient_E2EE_ENABLED
#include "qolmsession.h"
#include "crypto/qolmutils.h"
#include "logging.h"
#include <cstring>
#include <QDebug>

using namespace Quotient;

QOlmError lastError(OlmSession* session) {
    const std::string error_raw = olm_session_last_error(session);

    return fromString(error_raw);
}

Quotient::QOlmSession::~QOlmSession()
{
    olm_clear_session(m_session);
    delete[](reinterpret_cast<uint8_t *>(m_session));
}

OlmSession* QOlmSession::create()
{
    return olm_session(new uint8_t[olm_session_size()]);
}

std::variant<std::unique_ptr<QOlmSession>, QOlmError> QOlmSession::createInbound(QOlmAccount *account, const QOlmMessage &preKeyMessage, bool from, const QString &theirIdentityKey)
{
    if (preKeyMessage.type() != QOlmMessage::PreKey) {
        qCDebug(E2EE) << "The message is not a pre-key";
        throw BadMessageFormat;
    }

    const auto olmSession = create();

    QByteArray oneTimeKeyMessageBuf = preKeyMessage.toCiphertext();
    QByteArray theirIdentityKeyBuf = theirIdentityKey.toUtf8();
    size_t error = 0;
    if (from) {
        error = olm_create_inbound_session_from(olmSession, account->data(), theirIdentityKeyBuf.data(), theirIdentityKeyBuf.length(), oneTimeKeyMessageBuf.data(), oneTimeKeyMessageBuf.length());
    } else {
        error = olm_create_inbound_session(olmSession, account->data(), oneTimeKeyMessageBuf.data(), oneTimeKeyMessageBuf.length());
    }

    if (error == olm_error()) {
        const auto lastErr = lastError(olmSession);
        if (lastErr == QOlmError::NotEnoughRandom) {
            throw lastErr;
        }
        return lastErr;
    }

    return std::make_unique<QOlmSession>(olmSession);
}

std::variant<std::unique_ptr<QOlmSession>, QOlmError> QOlmSession::createInboundSession(QOlmAccount *account, const QOlmMessage &preKeyMessage)
{
    return createInbound(account, preKeyMessage);
}

std::variant<std::unique_ptr<QOlmSession>, QOlmError> QOlmSession::createInboundSessionFrom(QOlmAccount *account, const QString &theirIdentityKey, const QOlmMessage &preKeyMessage)
{
    return createInbound(account, preKeyMessage, true, theirIdentityKey);
}

std::variant<std::unique_ptr<QOlmSession>, QOlmError> QOlmSession::createOutboundSession(QOlmAccount *account, const QString &theirIdentityKey, const QString &theirOneTimeKey)
{
    auto *olmOutboundSession = create();
    const auto randomLen = olm_create_outbound_session_random_length(olmOutboundSession);
    QByteArray randomBuf = getRandom(randomLen);

    QByteArray theirIdentityKeyBuf = theirIdentityKey.toUtf8();
    QByteArray theirOneTimeKeyBuf = theirOneTimeKey.toUtf8();
    const auto error = olm_create_outbound_session(olmOutboundSession,
                                                   account->data(),
                                                   reinterpret_cast<uint8_t *>(theirIdentityKeyBuf.data()), theirIdentityKeyBuf.length(),
                                                   reinterpret_cast<uint8_t *>(theirOneTimeKeyBuf.data()), theirOneTimeKeyBuf.length(),
                                                   reinterpret_cast<uint8_t *>(randomBuf.data()), randomBuf.length());

    if (error == olm_error()) {
        const auto lastErr = lastError(olmOutboundSession);
        if (lastErr == QOlmError::NotEnoughRandom) {
            throw lastErr;
        }
        return lastErr;
    }

    randomBuf.clear();
    return std::make_unique<QOlmSession>(olmOutboundSession);
}

std::variant<QByteArray, QOlmError> QOlmSession::pickle(const PicklingMode &mode)
{
    QByteArray pickledBuf(olm_pickle_session_length(m_session), '0');
    QByteArray key = toKey(mode);
    const auto error = olm_pickle_session(m_session, key.data(), key.length(),
            pickledBuf.data(), pickledBuf.length());

    if (error == olm_error()) {
        return lastError(m_session);
    }

    key.clear();

    return pickledBuf;
}

std::variant<std::unique_ptr<QOlmSession>, QOlmError> QOlmSession::unpickle(const QByteArray &pickled, const PicklingMode &mode)
{
    QByteArray pickledBuf = pickled;
    auto *olmSession = create();
    QByteArray key = toKey(mode);
    const auto error = olm_unpickle_session(olmSession, key.data(), key.length(),
            pickledBuf.data(), pickledBuf.length());
    if (error == olm_error()) {
        return lastError(olmSession);
    }

    key.clear();
    return std::make_unique<QOlmSession>(olmSession);
}

QOlmMessage QOlmSession::encrypt(const QString &plaintext)
{
    QByteArray plaintextBuf = plaintext.toUtf8();
    const auto messageMaxLen = olm_encrypt_message_length(m_session, plaintextBuf.length());
    QByteArray messageBuf(messageMaxLen, '0');
    const auto messageType = encryptMessageType();
    const auto randomLen = olm_encrypt_random_length(m_session);
    QByteArray randomBuf = getRandom(randomLen);
    const auto error = olm_encrypt(m_session,
                                   reinterpret_cast<uint8_t *>(plaintextBuf.data()), plaintextBuf.length(),
                                   reinterpret_cast<uint8_t *>(randomBuf.data()), randomBuf.length(),
                                   reinterpret_cast<uint8_t *>(messageBuf.data()), messageBuf.length());

    if (error == olm_error()) {
        throw lastError(m_session);
    }

    return QOlmMessage(messageBuf, messageType);
}

std::variant<QString, QOlmError> QOlmSession::decrypt(const QOlmMessage &message) const
{
    const auto messageType = message.type();
    const auto ciphertext = message.toCiphertext();
    const auto messageTypeValue = messageType == QOlmMessage::Type::General
        ? OLM_MESSAGE_TYPE_MESSAGE : OLM_MESSAGE_TYPE_PRE_KEY;

    // We need to clone the message because
    // olm_decrypt_max_plaintext_length destroys the input buffer
    QByteArray messageBuf(ciphertext.length(), '0');
    std::copy(message.begin(), message.end(), messageBuf.begin());

    const auto plaintextMaxLen = olm_decrypt_max_plaintext_length(m_session, messageTypeValue,
            reinterpret_cast<uint8_t *>(messageBuf.data()), messageBuf.length());

    if (plaintextMaxLen == olm_error()) {
        return lastError(m_session);
    }

    QByteArray plaintextBuf(plaintextMaxLen, '0');
    QByteArray messageBuf2(ciphertext.length(), '0');
    std::copy(message.begin(), message.end(), messageBuf2.begin());

    const auto plaintextResultLen = olm_decrypt(m_session, messageTypeValue,
            reinterpret_cast<uint8_t *>(messageBuf2.data()), messageBuf2.length(),
            reinterpret_cast<uint8_t *>(plaintextBuf.data()), plaintextMaxLen);

    if (plaintextResultLen == olm_error()) {
        const auto lastErr = lastError(m_session);
        if (lastErr == QOlmError::OutputBufferTooSmall) {
            throw lastErr;
        }
        return lastErr;
    }
    QByteArray output(plaintextResultLen, '0');
    std::memcpy(output.data(), plaintextBuf.data(), plaintextResultLen);
    plaintextBuf.clear();
    return output;
}

QOlmMessage::Type QOlmSession::encryptMessageType()
{
    const auto messageTypeResult = olm_encrypt_message_type(m_session);
    if (messageTypeResult == olm_error()) {
        throw lastError(m_session);
    }
    if (messageTypeResult == OLM_MESSAGE_TYPE_PRE_KEY) {
        return QOlmMessage::PreKey;
    }
    return QOlmMessage::General;
}

QByteArray QOlmSession::sessionId() const
{
    const auto idMaxLength = olm_session_id_length(m_session);
    QByteArray idBuffer(idMaxLength, '0');
    const auto error = olm_session_id(m_session, reinterpret_cast<uint8_t *>(idBuffer.data()),
            idBuffer.length());
    if (error == olm_error()) {
        throw lastError(m_session);
    }
    return idBuffer;
}

bool QOlmSession::hasReceivedMessage() const
{
    return olm_session_has_received_message(m_session);
}

std::variant<bool, QOlmError> QOlmSession::matchesInboundSession(const QOlmMessage &preKeyMessage) const
{
    Q_ASSERT(preKeyMessage.type() == QOlmMessage::Type::PreKey);
    QByteArray oneTimeKeyBuf(preKeyMessage.data());
    const auto matchesResult = olm_matches_inbound_session(m_session, oneTimeKeyBuf.data(), oneTimeKeyBuf.length());

    if (matchesResult == olm_error()) {
        return lastError(m_session);
    }
    switch (matchesResult) {
        case 0:
            return false;
        case 1:
            return true;
        default:
            return QOlmError::Unknown;
    }
}
std::variant<bool, QOlmError> QOlmSession::matchesInboundSessionFrom(const QString &theirIdentityKey, const QOlmMessage &preKeyMessage) const
{
    const auto theirIdentityKeyBuf = theirIdentityKey.toUtf8();
    auto oneTimeKeyMessageBuf = preKeyMessage.toCiphertext();
    const auto error = olm_matches_inbound_session_from(m_session, theirIdentityKeyBuf.data(), theirIdentityKeyBuf.length(),
            oneTimeKeyMessageBuf.data(), oneTimeKeyMessageBuf.length());

    if (error == olm_error()) {
        return lastError(m_session);
    }
    switch (error) {
        case 0:
            return false;
        case 1:
            return true;
        default:
            return QOlmError::Unknown;
    }
}

QOlmSession::QOlmSession(OlmSession *session)
    : m_session(session)
{
}

#endif // Quotient_E2EE_ENABLED



