// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "e2ee/qolmutility.h"

#include <olm/olm.h>

using namespace Quotient;

OlmErrorCode QOlmUtility::lastErrorCode() const {
    return olm_utility_last_error_code(m_utility);
}

const char* QOlmUtility::lastError() const
{
    return olm_utility_last_error(m_utility);
}

QOlmUtility::QOlmUtility()
{
    auto utility = new uint8_t[olm_utility_size()];
    m_utility = olm_utility(utility);
}

QOlmUtility::~QOlmUtility()
{
    olm_clear_utility(m_utility);
    delete[](reinterpret_cast<uint8_t *>(m_utility));
}

QString QOlmUtility::sha256Bytes(const QByteArray &inputBuf) const
{
    const auto outputLen = olm_sha256_length(m_utility);
    QByteArray outputBuf(outputLen, '\0');
    olm_sha256(m_utility, inputBuf.data(), inputBuf.length(),
            outputBuf.data(), outputBuf.length());

    return QString::fromUtf8(outputBuf);
}

QString QOlmUtility::sha256Utf8Msg(const QString &message) const
{
    return sha256Bytes(message.toUtf8());
}

QOlmExpected<bool> QOlmUtility::ed25519Verify(const QByteArray& key,
                                              const QByteArray& message,
                                              const QByteArray& signature)
{
    QByteArray signatureBuf(signature.length(), '\0');
    std::copy(signature.begin(), signature.end(), signatureBuf.begin());

    if (olm_ed25519_verify(m_utility, key.data(), key.size(), message.data(),
                           message.size(), signatureBuf.data(),
                           signatureBuf.size())
        == 0)
        return true;

    auto error = lastErrorCode();
    if (error == OLM_BAD_MESSAGE_MAC)
        return false;
    return error;
}
