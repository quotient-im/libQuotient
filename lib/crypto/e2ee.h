// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <optional>
#include <string>
#include "converters.h"
#include <variant>

#include <QMap>
#include <QHash>
#include <QStringList>
#include <QMetaType>

#include "util.h"

namespace Quotient {

inline const auto CiphertextKeyL = "ciphertext"_ls;
inline const auto SenderKeyKeyL = "sender_key"_ls;
inline const auto DeviceIdKeyL = "device_id"_ls;
inline const auto SessionIdKeyL = "session_id"_ls;

inline const auto AlgorithmKeyL = "algorithm"_ls;
inline const auto RotationPeriodMsKeyL = "rotation_period_ms"_ls;
inline const auto RotationPeriodMsgsKeyL = "rotation_period_msgs"_ls;

inline const auto AlgorithmKey = QStringLiteral("algorithm");
inline const auto RotationPeriodMsKey = QStringLiteral("rotation_period_ms");
inline const auto RotationPeriodMsgsKey =
    QStringLiteral("rotation_period_msgs");

inline const auto Ed25519Key = QStringLiteral("ed25519");
inline const auto Curve25519Key = QStringLiteral("curve25519");
inline const auto SignedCurve25519Key = QStringLiteral("signed_curve25519");
inline const auto OlmV1Curve25519AesSha2AlgoKey =
    QStringLiteral("m.olm.v1.curve25519-aes-sha2");
inline const auto MegolmV1AesSha2AlgoKey =
    QStringLiteral("m.megolm.v1.aes-sha2");
inline const QStringList SupportedAlgorithms = { OlmV1Curve25519AesSha2AlgoKey,
                                                 MegolmV1AesSha2AlgoKey };
struct Unencrypted {};
struct Encrypted {
    QByteArray key;
};

using PicklingMode = std::variant<Unencrypted, Encrypted>;

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

struct IdentityKeys
{
    QByteArray curve25519;
    QByteArray ed25519;
};

//! Struct representing the one-time keys.
struct OneTimeKeys
{
    QMap<QString, QMap<QString, QString>> keys;

    //! Get the HashMap containing the curve25519 one-time keys.
    QMap<QString, QString> curve25519() const;

    //! Get a reference to the hashmap corresponding to given key type.
    std::optional<QMap<QString, QString>> get(QString keyType) const;
};

//! Struct representing the signed one-time keys.
class SignedOneTimeKey
{
public:
    SignedOneTimeKey() = default;
    SignedOneTimeKey(const SignedOneTimeKey &) = default;
    SignedOneTimeKey &operator=(const SignedOneTimeKey &) = default;
    //! Required. The unpadded Base64-encoded 32-byte Curve25519 public key.
    QString key;

    //! Required. Signatures of the key object.
    //! The signature is calculated using the process described at Signing JSON.
    QHash<QString, QHash<QString, QString>> signatures;
};


template <>
struct JsonObjectConverter<SignedOneTimeKey> {
    static void fillFrom(const QJsonObject& jo,
                         SignedOneTimeKey& result)
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

bool operator==(const IdentityKeys& lhs, const IdentityKeys& rhs);

template <typename T>
class asKeyValueRange
{
public:
    asKeyValueRange(T &data)
        : m_data{data}
    {
    }

    auto begin() { return m_data.keyValueBegin(); }

    auto end() { return m_data.keyValueEnd(); }

private:
    T &m_data;
};

} // namespace Quotient

Q_DECLARE_METATYPE(Quotient::SignedOneTimeKey)
