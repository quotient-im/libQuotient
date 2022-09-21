// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "qolmsession.h"

#include "e2ee/qolmutils.h"
#include "logging.h"

#include <cstring>
#include <olm/olm.h>

using namespace Quotient;

OlmErrorCode QOlmSession::lastErrorCode() const {
    return olm_session_last_error_code(m_session);
}

const char* QOlmSession::lastError() const
{
    return olm_session_last_error(m_session);
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

QOlmExpected<QOlmSessionPtr> QOlmSession::createInbound(
    QOlmAccount* account, const QOlmMessage& preKeyMessage, bool from,
    const QString& theirIdentityKey)
{
    if (preKeyMessage.type() != QOlmMessage::PreKey) {
        qCCritical(E2EE) << "The message is not a pre-key; will try to create "
                            "the inbound session anyway";
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
        // FIXME: the QOlmSession object should be created earlier
        const auto lastErr = olm_session_last_error_code(olmSession);
        qCWarning(E2EE) << "Error when creating inbound session" << lastErr;
        return lastErr;
    }

    return std::make_unique<QOlmSession>(olmSession);
}

QOlmExpected<QOlmSessionPtr> QOlmSession::createInboundSession(
    QOlmAccount* account, const QOlmMessage& preKeyMessage)
{
    return createInbound(account, preKeyMessage);
}

QOlmExpected<QOlmSessionPtr> QOlmSession::createInboundSessionFrom(
    QOlmAccount* account, const QString& theirIdentityKey,
    const QOlmMessage& preKeyMessage)
{
    return createInbound(account, preKeyMessage, true, theirIdentityKey);
}

QOlmExpected<QOlmSessionPtr> QOlmSession::createOutboundSession(
    QOlmAccount* account, const QByteArray& theirIdentityKey,
    const QByteArray& theirOneTimeKey)
{
    auto* olmOutboundSession = create();
    const auto randomLength =
        olm_create_outbound_session_random_length(olmOutboundSession);

    if (olm_create_outbound_session(
            olmOutboundSession, account->data(), theirIdentityKey.data(),
            theirIdentityKey.length(), theirOneTimeKey.data(),
            theirOneTimeKey.length(), RandomBuffer(randomLength), randomLength)
        == olm_error()) {
        // FIXME: the QOlmSession object should be created earlier
        const auto lastErr = olm_session_last_error_code(olmOutboundSession);
        if (lastErr == OLM_NOT_ENOUGH_RANDOM) {
            throw lastErr;
        }
        return lastErr;
    }

    return std::make_unique<QOlmSession>(olmOutboundSession);
}

QOlmExpected<QByteArray> QOlmSession::pickle(const PicklingMode &mode) const
{
    QByteArray pickledBuf(olm_pickle_session_length(m_session), '\0');
    QByteArray key = toKey(mode);
    if (olm_pickle_session(m_session, key.data(), key.length(),
                           pickledBuf.data(), pickledBuf.length())
        == olm_error())
        return lastErrorCode();

    key.clear();
    return pickledBuf;
}

QOlmExpected<QOlmSessionPtr> QOlmSession::unpickle(QByteArray&& pickled,
                                                   const PicklingMode& mode)
{
    auto *olmSession = create();
    auto key = toKey(mode);
    if (olm_unpickle_session(olmSession, key.data(), key.length(),
                             pickled.data(), pickled.length())
        == olm_error()) {
        // FIXME: the QOlmSession object should be created earlier
        return olm_session_last_error_code(olmSession);
    }

    key.clear();
    return std::make_unique<QOlmSession>(olmSession);
}

QOlmMessage QOlmSession::encrypt(const QByteArray& plaintext)
{
    const auto messageMaxLength =
        olm_encrypt_message_length(m_session, plaintext.length());
    QByteArray messageBuf(messageMaxLength, '0');
    // NB: The type has to be calculated before calling olm_encrypt()
    const auto messageType = olm_encrypt_message_type(m_session);
    const auto randomLength = olm_encrypt_random_length(m_session);
    if (olm_encrypt(m_session, plaintext.data(), plaintext.length(),
                    RandomBuffer(randomLength), randomLength, messageBuf.data(),
                    messageBuf.length())
        == olm_error()) {
        throw lastError();
    }

    return QOlmMessage(messageBuf, QOlmMessage::Type(messageType));
}

QOlmExpected<QByteArray> QOlmSession::decrypt(const QOlmMessage &message) const
{
    const auto ciphertext = message.toCiphertext();
    const auto messageTypeValue = message.type();

    // We need to clone the message because
    // olm_decrypt_max_plaintext_length destroys the input buffer
    QByteArray messageBuf(ciphertext.length(), '\0');
    std::copy(message.begin(), message.end(), messageBuf.begin());

    const auto plaintextMaxLen = olm_decrypt_max_plaintext_length(
        m_session, messageTypeValue, messageBuf.data(), messageBuf.length());
    if (plaintextMaxLen == olm_error()) {
        return lastError();
    }

    QByteArray plaintextBuf(plaintextMaxLen, '\0');
    QByteArray messageBuf2(ciphertext.length(), '\0');
    std::copy(message.begin(), message.end(), messageBuf2.begin());

    const auto plaintextResultLen =
        olm_decrypt(m_session, messageTypeValue, messageBuf2.data(),
                    messageBuf2.length(), plaintextBuf.data(), plaintextMaxLen);
    if (plaintextResultLen == olm_error()) {
        const auto lastErr = lastErrorCode();
        if (lastErr == OLM_OUTPUT_BUFFER_TOO_SMALL) {
            throw lastErr;
        }
        return lastErr;
    }
    plaintextBuf.truncate(plaintextResultLen);
    return plaintextBuf;
}

QByteArray QOlmSession::sessionId() const
{
    const auto idMaxLength = olm_session_id_length(m_session);
    QByteArray idBuffer(idMaxLength, '0');
    if (olm_session_id(m_session, idBuffer.data(), idMaxLength) == olm_error()) {
        throw lastError();
    }
    return idBuffer;
}

bool QOlmSession::hasReceivedMessage() const
{
    return olm_session_has_received_message(m_session);
}

bool QOlmSession::matchesInboundSession(const QOlmMessage& preKeyMessage) const
{
    Q_ASSERT(preKeyMessage.type() == QOlmMessage::Type::PreKey);
    QByteArray oneTimeKeyBuf(preKeyMessage.data());
    const auto maybeMatches =
        olm_matches_inbound_session(m_session, oneTimeKeyBuf.data(),
                                    oneTimeKeyBuf.length());
    if (maybeMatches == olm_error())
        qWarning(E2EE) << "Error matching an inbound session:"
                       << olm_session_last_error(m_session);
    return maybeMatches == 1; // Any errors are treated as non-match
}

bool QOlmSession::matchesInboundSessionFrom(
    const QString& theirIdentityKey, const QOlmMessage& preKeyMessage) const
{
    const auto theirIdentityKeyBuf = theirIdentityKey.toUtf8();
    auto oneTimeKeyMessageBuf = preKeyMessage.toCiphertext();
    const auto maybeMatches = olm_matches_inbound_session_from(
        m_session, theirIdentityKeyBuf.data(), theirIdentityKeyBuf.length(),
        oneTimeKeyMessageBuf.data(), oneTimeKeyMessageBuf.length());

    if (maybeMatches == olm_error())
        qCWarning(E2EE) << "Error matching an inbound session:"
                        << olm_session_last_error(m_session);
    return maybeMatches == 1;
}

QOlmSession::QOlmSession(OlmSession *session)
    : m_session(session)
{}
