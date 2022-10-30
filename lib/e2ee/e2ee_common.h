// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "converters.h"

#include <QtCore/QMetaType>
#include <QtCore/QStringBuilder>

#include <array>

#ifdef Quotient_E2EE_ENABLED
#    include "expected.h"

#    include <olm/error.h>
#    include <variant>
#endif

namespace Quotient {

constexpr auto AlgorithmKeyL = "algorithm"_ls;
constexpr auto RotationPeriodMsKeyL = "rotation_period_ms"_ls;
constexpr auto RotationPeriodMsgsKeyL = "rotation_period_msgs"_ls;

constexpr auto AlgorithmKey = "algorithm"_ls;
constexpr auto RotationPeriodMsKey = "rotation_period_ms"_ls;
constexpr auto RotationPeriodMsgsKey = "rotation_period_msgs"_ls;

constexpr auto Ed25519Key = "ed25519"_ls;
constexpr auto Curve25519Key = "curve25519"_ls;
constexpr auto SignedCurve25519Key = "signed_curve25519"_ls;

constexpr auto OlmV1Curve25519AesSha2AlgoKey = "m.olm.v1.curve25519-aes-sha2"_ls;
constexpr auto MegolmV1AesSha2AlgoKey = "m.megolm.v1.aes-sha2"_ls;

constexpr std::array SupportedAlgorithms { OlmV1Curve25519AesSha2AlgoKey,
                                           MegolmV1AesSha2AlgoKey };

inline bool isSupportedAlgorithm(const QString& algorithm)
{
    return std::find(SupportedAlgorithms.cbegin(), SupportedAlgorithms.cend(),
                     algorithm)
           != SupportedAlgorithms.cend();
}

#ifdef Quotient_E2EE_ENABLED

#define QOLM_INTERNAL_ERROR_X(Message_, LastError_) \
    qFatal("%s, internal error: %s", Message_, LastError_)

#define QOLM_INTERNAL_ERROR(Message_) \
    QOLM_INTERNAL_ERROR_X(Message_, lastError())

#define QOLM_FAIL_OR_LOG_X(InternalCondition_, Message_, LastErrorText_)   \
    do {                                                                   \
        const QString errorMsg{ (Message_) };                              \
        if (InternalCondition_)                                            \
            QOLM_INTERNAL_ERROR_X(qPrintable(errorMsg), (LastErrorText_)); \
        qWarning(E2EE).nospace() << errorMsg << ": " << (LastErrorText_);  \
    } while (false) /* End of macro */

#define QOLM_FAIL_OR_LOG(InternalFailureValue_, Message_)                      \
    QOLM_FAIL_OR_LOG_X(lastErrorCode() == (InternalFailureValue_), (Message_), \
                       lastError())

struct Unencrypted {};
struct Encrypted {
    QByteArray key;
};

using PicklingMode = std::variant<Unencrypted, Encrypted>;

template <typename T>
using QOlmExpected = Expected<T, OlmErrorCode>;

// Convert PicklingMode to key
QUOTIENT_API QByteArray toKey(const PicklingMode &mode);

//! \brief Initialise a buffer object for use with Olm calls
//!
//! Qt and Olm use different size types; this causes the warning noise
QUOTIENT_API QByteArray bufferForOlm(size_t bufferSize);

//! \brief Get a size of Qt container coerced to size_t
//!
//! It's a safe cast since size_t can easily accommodate the range between
//! 0 and INTMAX - 1 that Qt containers support; yet compilers complain...
inline size_t unsignedSize(auto qtBuffer)
{
    // The buffer size cannot be negative by definition, so this is deemed safe
    return static_cast<size_t>(qtBuffer.size());
}

class QUOTIENT_API RandomBuffer : public QByteArray {
public:
    explicit RandomBuffer(size_t size);
    ~RandomBuffer() { clear(); }

    // NOLINTNEXTLINE(google-explicit-constructor)
    QUO_IMPLICIT operator void*() { return data(); }
    char* chars() { return data(); }
    uint8_t* bytes() { return reinterpret_cast<uint8_t*>(data()); }

    Q_DISABLE_COPY(RandomBuffer)
    RandomBuffer(RandomBuffer&&) = default;
    void operator=(RandomBuffer&&) = delete;
};

[[deprecated("Create RandomBuffer directly")]] //
inline RandomBuffer getRandom(size_t bufferSize)
{
    return RandomBuffer(bufferSize);
}

#endif // Quotient_E2EE_ENABLED

struct IdentityKeys
{
    QByteArray curve25519;
    QByteArray ed25519;
};

//! Struct representing the one-time keys.
struct UnsignedOneTimeKeys
{
    QHash<QString, QHash<QString, QString>> keys;

    //! Get the HashMap containing the curve25519 one-time keys.
    QHash<QString, QString> curve25519() const { return keys[Curve25519Key]; }
};

class SignedOneTimeKey {
public:
    explicit SignedOneTimeKey(const QString& unsignedKey, const QString& userId,
                              const QString& deviceId,
                              const QByteArray& signature)
        : payload { { "key"_ls, unsignedKey },
                    { "signatures"_ls,
                      QJsonObject {
                          { userId, QJsonObject { { "ed25519:"_ls % deviceId,
                                                    QString(signature) } } } } } }
    {}
    explicit SignedOneTimeKey(const QJsonObject& jo = {})
        : payload(jo)
    {}

    //! Unpadded Base64-encoded 32-byte Curve25519 public key
    QByteArray key() const { return payload["key"_ls].toString().toLatin1(); }

    //! \brief Signatures of the key object
    //!
    //! The signature is calculated using the process described at
    //! https://spec.matrix.org/v1.3/appendices/#signing-json
    auto signatures() const
    {
        return fromJson<QHash<QString, QHash<QString, QString>>>(
            payload["signatures"_ls]);
    }

    QByteArray signature(QStringView userId, QStringView deviceId) const
    {
        return payload["signatures"_ls][userId]["ed25519:"_ls % deviceId]
            .toString()
            .toLatin1();
    }

    //! Whether the key is a fallback key
    bool isFallback() const { return payload["fallback"_ls].toBool(); }
    auto toJson() const { return payload; }
    auto toJsonForVerification() const
    {
        auto json = payload;
        json.remove("signatures"_ls);
        json.remove("unsigned"_ls);
        return QJsonDocument(json).toJson(QJsonDocument::Compact);
    }

private:
    QJsonObject payload;
};

using OneTimeKeys = QHash<QString, std::variant<QString, SignedOneTimeKey>>;

} // namespace Quotient

Q_DECLARE_METATYPE(Quotient::SignedOneTimeKey)
