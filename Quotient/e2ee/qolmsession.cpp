// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "qolmsession.h"

#include "../logging_categories_p.h"

#include <olm/olm.h>

using namespace Quotient;

OlmErrorCode QOlmSession::lastErrorCode() const {
    return olm_session_last_error_code(olmData);
}

const char* QOlmSession::lastError() const
{
    return olm_session_last_error(olmData);
}

QByteArray QOlmSession::pickle(const PicklingKey &key) const
{
    const auto pickleLength = olm_pickle_session_length(olmData);
    auto pickledBuf = byteArrayForOlm(pickleLength);
    if (olm_pickle_session(olmData, key.data(), key.size(),
                           pickledBuf.data(), unsignedSize(pickledBuf))
        == olm_error())
        QOLM_INTERNAL_ERROR("Failed to pickle an Olm session");

    return pickledBuf;
}

QOlmExpected<QOlmSession> QOlmSession::unpickle(QByteArray&& pickled,
                                                const PicklingKey &key)
{
    QOlmSession olmSession{};
    if (olm_unpickle_session(olmSession.olmData, key.data(), key.size(),
                             pickled.data(), unsignedSize(pickled))
        == olm_error()) {
        const auto errorCode = olmSession.lastErrorCode();
        QOLM_FAIL_OR_LOG_X(errorCode == OLM_OUTPUT_BUFFER_TOO_SMALL,
                           "Failed to unpickle an Olm session"_ls,
                           olmSession.lastError());
        return errorCode;
    }

    return olmSession;
}

QOlmMessage QOlmSession::encrypt(const QByteArray& plaintext) const
{
    const auto messageMaxLength =
        olm_encrypt_message_length(olmData, unsignedSize(plaintext));
    auto messageBuf = byteArrayForOlm(messageMaxLength);
    // NB: The type has to be calculated before calling olm_encrypt()
    const auto messageType = olm_encrypt_message_type(olmData);
    if (const auto randomLength = olm_encrypt_random_length(olmData);
        olm_encrypt(olmData, plaintext.data(), unsignedSize(plaintext),
                    getRandom(randomLength).data(), randomLength,
                    messageBuf.data(), messageMaxLength)
        == olm_error()) {
        QOLM_INTERNAL_ERROR("Failed to encrypt the message");
    }

    return QOlmMessage(messageBuf, QOlmMessage::Type(messageType));
}

QOlmExpected<QByteArray> QOlmSession::decrypt(const QOlmMessage& message) const
{
    const auto ciphertext = message.toCiphertext();
    const auto messageTypeValue = message.type();

    // We need to clone the message because
    // olm_decrypt_max_plaintext_length destroys the input buffer
    const auto plaintextMaxLen =
        olm_decrypt_max_plaintext_length(olmData, messageTypeValue,
                                         QByteArray(ciphertext).data(),
                                         unsignedSize(ciphertext));
    if (plaintextMaxLen == olm_error()) {
        qWarning(E2EE) << "Couldn't calculate decrypted message length:"
                       << lastError();
        return lastErrorCode();
    }

    auto plaintextBuf = byteArrayForOlm(plaintextMaxLen);
    const auto actualLength = olm_decrypt(olmData, messageTypeValue,
                                          QByteArray(ciphertext).data(),
                                          unsignedSize(ciphertext),
                                          plaintextBuf.data(), plaintextMaxLen);
    if (actualLength == olm_error()) {
        QOLM_FAIL_OR_LOG(OLM_OUTPUT_BUFFER_TOO_SMALL, "Failed to decrypt the message"_ls);
        return lastErrorCode();
    }
    // actualLength cannot be more than plainTextLength because the resulting
    // text would overflow the allocated memory; but it can be less, in theory
    plaintextBuf.truncate(static_cast<int>(actualLength));
    return plaintextBuf;
}

QByteArray QOlmSession::sessionId() const
{
    const auto idMaxLength = olm_session_id_length(olmData);
    auto idBuffer = byteArrayForOlm(idMaxLength);
    if (olm_session_id(olmData, idBuffer.data(), idMaxLength) == olm_error())
        QOLM_INTERNAL_ERROR("Failed to obtain Olm session id");

    return idBuffer;
}

bool QOlmSession::hasReceivedMessage() const
{
    return olm_session_has_received_message(olmData) != 0;
}

bool QOlmSession::matchesInboundSession(const QOlmMessage& preKeyMessage) const
{
    Q_ASSERT(preKeyMessage.type() == QOlmMessage::Type::PreKey);
    QByteArray oneTimeKeyBuf(preKeyMessage.data());
    const auto maybeMatches =
        olm_matches_inbound_session(olmData, oneTimeKeyBuf.data(),
                                    unsignedSize(oneTimeKeyBuf));
    if (maybeMatches == olm_error())
        qWarning(E2EE) << "Error matching an inbound session:" << lastError();

    return maybeMatches == 1; // Any errors are treated as non-match
}

bool QOlmSession::matchesInboundSessionFrom(
    QByteArray theirIdentityKey, const QOlmMessage& preKeyMessage) const
{
    auto oneTimeKeyMessageBuf = preKeyMessage.toCiphertext();
    const auto maybeMatches = olm_matches_inbound_session_from(
        olmData, theirIdentityKey.data(), unsignedSize(theirIdentityKey),
        oneTimeKeyMessageBuf.data(), unsignedSize(oneTimeKeyMessageBuf));

    if (maybeMatches == olm_error())
        qCWarning(E2EE) << "Error matching an inbound session:" << lastError();

    return maybeMatches == 1;
}

QOlmSession::QOlmSession()
    : olmDataHolder(
        makeCStruct(olm_session, olm_session_size, olm_clear_session))
{}
