// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "qolmsession.h"

#include "../logging_categories_p.h"

#include <cstring>
#include <vodozemac.h>

using namespace Quotient;

QByteArray QOlmSession::pickle(const PicklingKey &key) const
{
    //TODO: This is terrible :(
    std::array<std::uint8_t, 32> _key;
    std::copy(key.data(), key.data() + 32, _key.begin());
    const auto &pickle = m_session->pickle(_key);
    return {pickle.data(), (qsizetype) pickle.size()};

}

QOlmExpected<QOlmSession> QOlmSession::unpickle(QByteArray&& pickled,
                                                const PicklingKey &key)
{
    //TODO: This is terrible :(
    std::array<std::uint8_t, 32> _key;
    std::copy(key.data(), key.data() + 32, _key.begin());

    auto session = olm::session_from_pickle(rust::String(pickled.data(), pickled.size()), _key);
    return QOlmSession(std::move(session));
}

QOlmMessage QOlmSession::encrypt(const QByteArray& plaintext)
{
    ::rust::String plain(plaintext.data(), plaintext.length());
    const auto &cipher = m_session->encrypt(plain)->to_parts();
    const auto &ciphertext = cipher.ciphertext;

    QByteArray data(ciphertext.data(), ciphertext.length());

    //TODO signal?
    return QOlmMessage(data, cipher.message_type);
}

QOlmExpected<QByteArray> QOlmSession::tryDecrypt(const QOlmMessage& message)
{
    const auto ciphertext = message.toCiphertext();

    auto olmMessage = olm::olm_message_from_parts(olm::OlmMessageParts{
        .message_type = message.type(),
        .ciphertext = ::rust::String(message.toCiphertext().data(), message.toCiphertext().size())
    });
    auto decryptedResult = m_session->decrypt(*olmMessage.into_raw());
    if (decryptedResult->has_error()) {
        return 1; //TODO return correct error
    }
    auto decrypted = olm::session_decrypt_result_value(std::move(decryptedResult));
    return QByteArray(decrypted.data(), decrypted.size());
}

QByteArray QOlmSession::sessionId() const
{
    auto id = m_session->session_id();
    return QByteArray(id.data(), id.size());
}

QOlmSession::QOlmSession(rust::Box<olm::Session> session)
    : m_session(std::move(session))
{}
