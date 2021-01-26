// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef Quotient_E2EE_ENABLED
#include "olm/session.h"
#include "olm/utils.h"
#include "logging.h"

using namespace Quotient;

OlmError lastError(OlmSession* session) {
    const std::string error_raw = olm_session_last_error(session);

    return fromString(error_raw);
}

Quotient::QOlmSession::~QOlmSession()
{
    olm_clear_session(m_session);
}

OlmSession* QOlmSession::create()
{
    return olm_session(new uint8_t[olm_session_size()]);
}

std::variant<std::unique_ptr<QOlmSession>, OlmError> QOlmSession::createInbound(QOlmAccount *account, const Message &preKeyMessage, bool from, const QString &theirIdentityKey)
{
    if (preKeyMessage.type() != Message::PreKey) {
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
        if (lastErr == OlmError::NotEnoughRandom) {
            throw lastErr;
        }
        return lastErr;
    }

    return std::make_unique<QOlmSession>(olmSession);
}

std::variant<std::unique_ptr<QOlmSession>, OlmError> QOlmSession::createInboundSession(QOlmAccount *account, const Message &preKeyMessage)
{
    return createInbound(account, preKeyMessage);
}

std::variant<std::unique_ptr<QOlmSession>, OlmError> QOlmSession::createInboundSessionFrom(QOlmAccount *account, const QString &theirIdentityKey, const Message &preKeyMessage)
{
    return createInbound(account, preKeyMessage, true, theirIdentityKey);
}

std::variant<std::unique_ptr<QOlmSession>, OlmError> QOlmSession::createOutboundSession(QOlmAccount *account, const QString &theirIdentityKey, const QString &theirOneTimeKey)
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
        if (lastErr == OlmError::NotEnoughRandom) {
            throw lastErr;
        }
        return lastErr;
    }

    randomBuf.clear();
    return std::make_unique<QOlmSession>(olmOutboundSession);
}

std::variant<QByteArray, OlmError> QOlmSession::pickle(const PicklingMode &mode)
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

std::variant<std::unique_ptr<QOlmSession>, OlmError> QOlmSession::unpickle(QByteArray &pickled, const PicklingMode &mode)
{
    QByteArray pickledBuf = pickled;
    auto *olmSession = create();
    QByteArray key = toKey(mode);
    const auto error = olm_unpickle_session(olmSession, key.data(), key.length(),
            pickled.data(), pickled.length());
    if (error == olm_error()) {
        return lastError(olmSession);
    }

    key.clear();
    return std::make_unique<QOlmSession>(olmSession);
}

std::variant<Message, OlmError> QOlmSession::encrypt(const QString &plaintext)
{
    QByteArray plaintextBuf = plaintext.toUtf8();
    const auto messageMaxLen = olm_encrypt_message_length(m_session, plaintextBuf.length());
    QByteArray messageBuf(messageMaxLen, '0');
    const auto randomLen = olm_encrypt_random_length(m_session);
    QByteArray randomBuf = getRandom(randomLen);
    const auto error = olm_encrypt(m_session,
                                   reinterpret_cast<uint8_t *>(plaintextBuf.data()), plaintextBuf.length(),
                                   reinterpret_cast<uint8_t *>(randomBuf.data()), randomBuf.length(),
                                   reinterpret_cast<uint8_t *>(messageBuf.data()), messageBuf.length());

    if (error == olm_error()) {
        return lastError(m_session);
    }

    return Message::fromCiphertext(messageBuf);
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

QOlmSession::QOlmSession(OlmSession *session): m_session(session)
{

}

#endif // Quotient_E2EE_ENABLED



