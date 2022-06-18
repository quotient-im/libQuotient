// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "converters.h"
#include "expected.h"
#include "qolmerrors.h"

#include <QtCore/QMetaType>
#include <variant>

namespace Quotient {

constexpr auto CiphertextKeyL = "ciphertext"_ls;
constexpr auto SenderKeyKeyL = "sender_key"_ls;
constexpr auto DeviceIdKeyL = "device_id"_ls;
constexpr auto SessionIdKeyL = "session_id"_ls;

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

inline bool isSupportedAlgorithm(const QString& algorithm)
{
    static constexpr std::array SupportedAlgorithms {
        OlmV1Curve25519AesSha2AlgoKey, MegolmV1AesSha2AlgoKey
    };
    return std::find(SupportedAlgorithms.cbegin(), SupportedAlgorithms.cend(),
                     algorithm)
           != SupportedAlgorithms.cend();
}

struct Unencrypted {};
struct Encrypted {
    QByteArray key;
};

using PicklingMode = std::variant<Unencrypted, Encrypted>;

class QOlmSession;
using QOlmSessionPtr = std::unique_ptr<QOlmSession>;

class QOlmInboundGroupSession;
using QOlmInboundGroupSessionPtr = std::unique_ptr<QOlmInboundGroupSession>;

class QOlmOutboundGroupSession;
using QOlmOutboundGroupSessionPtr = std::unique_ptr<QOlmOutboundGroupSession>;

template <typename T>
using QOlmExpected = Expected<T, QOlmError>;

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

//! Struct representing the signed one-time keys.
class SignedOneTimeKey
{
public:
    //! Required. The unpadded Base64-encoded 32-byte Curve25519 public key.
    QString key;

    //! Required. Signatures of the key object.
    //! The signature is calculated using the process described at Signing JSON.
    QHash<QString, QHash<QString, QString>> signatures;
};

template <>
struct JsonObjectConverter<SignedOneTimeKey> {
    static void fillFrom(const QJsonObject& jo, SignedOneTimeKey& result)
    {
        fromJson(jo.value("key"_ls), result.key);
        fromJson(jo.value("signatures"_ls), result.signatures);
    }

    static void dumpTo(QJsonObject &jo, const SignedOneTimeKey &result)
    {
        addParam<>(jo, QStringLiteral("key"), result.key);
        addParam<>(jo, QStringLiteral("signatures"), result.signatures);
    }
};

using OneTimeKeys = QHash<QString, std::variant<QString, SignedOneTimeKey>>;

template <typename T>
class asKeyValueRange
{
public:
    asKeyValueRange(T& data)
        : m_data { data }
    {}

    auto begin() { return m_data.keyValueBegin(); }
    auto end() { return m_data.keyValueEnd(); }

private:
    T &m_data;
};
template <typename T>
asKeyValueRange(T&) -> asKeyValueRange<T>;

} // namespace Quotient

Q_DECLARE_METATYPE(Quotient::SignedOneTimeKey)
