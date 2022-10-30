// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "qolmsession.h"

#include "logging.h"

#include <cstring>
#include <olm/olm.h>

using namespace Quotient;

OlmErrorCode QOlmSession::lastErrorCode() const {
    return olm_session_last_error_code(olmData);
}

const char* QOlmSession::lastError() const
{
    return olm_session_last_error(olmData);
}


QByteArray QOlmSession::pickle(const PicklingMode &mode) const
{
    QByteArray pickledBuf(olm_pickle_session_length(olmData), '\0');
    QByteArray key = toKey(mode);
    if (olm_pickle_session(olmData, key.data(), key.length(), pickledBuf.data(),
                           pickledBuf.length())
        == olm_error())
        QOLM_INTERNAL_ERROR("Failed to pickle an Olm session");

    key.clear();
    return pickledBuf;
}

QOlmExpected<QOlmSession> QOlmSession::unpickle(QByteArray&& pickled,
                                                const PicklingMode& mode)
{
    QOlmSession olmSession{};
    auto key = toKey(mode);
    if (olm_unpickle_session(olmSession.olmData, key.data(), key.length(),
                             pickled.data(), pickled.length())
        == olm_error()) {
        const auto errorCode = olmSession.lastErrorCode();
        QOLM_FAIL_OR_LOG_X(errorCode == OLM_OUTPUT_BUFFER_TOO_SMALL,
                           "Failed to unpickle an Olm session",
                           olmSession.lastError());
        return errorCode;
    }

    key.clear();
    return olmSession;
}

QOlmMessage QOlmSession::encrypt(const QByteArray& plaintext) const
{
    const auto messageMaxLength =
        olm_encrypt_message_length(olmData, plaintext.length());
    QByteArray messageBuf(messageMaxLength, '\0');
    // NB: The type has to be calculated before calling olm_encrypt()
    const auto messageType = olm_encrypt_message_type(olmData);
    if (const auto randomLength = olm_encrypt_random_length(olmData);
        olm_encrypt(olmData, plaintext.data(), plaintext.length(),
                    RandomBuffer(randomLength), randomLength, messageBuf.data(),
                    messageMaxLength)
        == olm_error()) {
        QOLM_INTERNAL_ERROR("Failed to encrypt the message");
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
        olmData, messageTypeValue, messageBuf.data(), messageBuf.length());
    if (plaintextMaxLen == olm_error()) {
        qWarning(E2EE) << "Couldn't calculate decrypted message length:"
                       << lastError();
        return lastErrorCode();
    }

    QByteArray plaintextBuf(plaintextMaxLen, '\0');
    QByteArray messageBuf2(ciphertext.length(), '\0');
    std::copy(message.begin(), message.end(), messageBuf2.begin());

    const auto plaintextResultLen =
        olm_decrypt(olmData, messageTypeValue, messageBuf2.data(),
                    messageBuf2.length(), plaintextBuf.data(), plaintextMaxLen);
    if (plaintextResultLen == olm_error()) {
        QOLM_FAIL_OR_LOG(OLM_OUTPUT_BUFFER_TOO_SMALL,
                         "Failed to decrypt the message");
        return lastErrorCode();
    }
    plaintextBuf.truncate(plaintextResultLen);
    return plaintextBuf;
}

QByteArray QOlmSession::sessionId() const
{
    const auto idMaxLength = olm_session_id_length(olmData);
    QByteArray idBuffer(idMaxLength, '\0');
    if (olm_session_id(olmData, idBuffer.data(), idMaxLength) == olm_error())
        QOLM_INTERNAL_ERROR("Failed to obtain Olm session id");

    return idBuffer;
}

bool QOlmSession::hasReceivedMessage() const
{
    return olm_session_has_received_message(olmData);
}

bool QOlmSession::matchesInboundSession(const QOlmMessage& preKeyMessage) const
{
    Q_ASSERT(preKeyMessage.type() == QOlmMessage::Type::PreKey);
    QByteArray oneTimeKeyBuf(preKeyMessage.data());
    const auto maybeMatches =
        olm_matches_inbound_session(olmData, oneTimeKeyBuf.data(),
                                    oneTimeKeyBuf.length());
    if (maybeMatches == olm_error())
        qWarning(E2EE) << "Error matching an inbound session:" << lastError();

    return maybeMatches == 1; // Any errors are treated as non-match
}

bool QOlmSession::matchesInboundSessionFrom(
    const QString& theirIdentityKey, const QOlmMessage& preKeyMessage) const
{
    const auto theirIdentityKeyBuf = theirIdentityKey.toUtf8();
    auto oneTimeKeyMessageBuf = preKeyMessage.toCiphertext();
    const auto maybeMatches = olm_matches_inbound_session_from(
        olmData, theirIdentityKeyBuf.data(), theirIdentityKeyBuf.length(),
        oneTimeKeyMessageBuf.data(), oneTimeKeyMessageBuf.length());

    if (maybeMatches == olm_error())
        qCWarning(E2EE) << "Error matching an inbound session:" << lastError();

    return maybeMatches == 1;
}

QOlmSession::QOlmSession()
    : olmDataHolder(
        makeCStruct(olm_session, olm_session_size, olm_clear_session))
{}
