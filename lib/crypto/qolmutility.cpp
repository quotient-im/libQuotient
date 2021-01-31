// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifdef Quotient_E2EE_ENABLED
#include "crypto/qolmutility.h"
#include "olm/olm.h"
#include <QDebug>

using namespace Quotient;

// Convert olm error to enum
QOlmError lastError(OlmUtility *utility) {
    const std::string error_raw = olm_utility_last_error(utility);

    return fromString(error_raw);
}

QOlmUtility::QOlmUtility()
{
    auto utility = new uint8_t[olm_utility_size()];
    m_utility = olm_utility(utility);
    qDebug() << "created";
}

QOlmUtility::~QOlmUtility()
{
    qDebug() << "deleted";
    olm_clear_utility(m_utility);
    delete[](reinterpret_cast<uint8_t *>(m_utility));
}

QString QOlmUtility::sha256Bytes(const QByteArray &inputBuf) const
{
    const auto outputLen = olm_sha256_length(m_utility);
    QByteArray outputBuf(outputLen, '0');
    olm_sha256(m_utility, inputBuf.data(), inputBuf.length(),
            outputBuf.data(), outputBuf.length());

    return QString::fromUtf8(outputBuf);
}

QString QOlmUtility::sha256Utf8Msg(const QString &message) const
{
    return sha256Bytes(message.toUtf8());
}

std::variant<bool, QOlmError> QOlmUtility::ed25519Verify(const QByteArray &key,
        const QByteArray &message, const QByteArray &signature)
{
    QByteArray signatureBuf(signature.length(), '0');
    std::copy(signature.begin(), signature.end(), signatureBuf.begin());

    qDebug() << "3" << key << message << signature;

    const auto ret = olm_ed25519_verify(m_utility, key.data(), key.size(),
            message.data(), message.size(), (void *)signatureBuf.data(), signatureBuf.size());

    const auto error = ret;
    if (error == olm_error()) {
        qDebug() << QString(olm_utility_last_error(m_utility));
        return lastError(m_utility);
    }

    if (ret != 0) {
        qDebug() << "ed25519Verify" << ret;
        return false;
    }
    return true;
}


#endif
