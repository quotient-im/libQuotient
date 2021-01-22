// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QObject>
#include <QMap>
#include <optional>
#include <string>
#include <variant>
#include "olm/olm.h"

struct OlmAccount;

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

//! Struct representing the the one-time keys.
struct OneTimeKeys
{
    QMap<QString, QMap<QString, QString>> keys;

    //! Get the HashMap containing the curve25519 one-time keys.
    QMap<QString, QString> curve25519() const;

    //! Get a reference to the hashmap corresponding to given key type.
    std::optional<QMap<QString, QString>> get(QString keyType) const;
};

bool operator==(const IdentityKeys& lhs, const IdentityKeys& rhs);

//! An olm account manages all cryptographic keys used on a device.
//! \code{.cpp}
//! const auto olmAccount = new QOlmAccount(this);
//! \endcode
class QOlmAccount
{
public:
    enum OlmAccountError {
        BadAccountKey,
        BadMessageKeyId,
        InvalidBase64,
        NotEnoughRandom,
        OutputBufferTooSmall,
        Unknown,
    };

    //! Creates a new instance of OlmAccount. During the instantiation
    //! the Ed25519 fingerprint key pair and the Curve25519 identity key
    //! pair are generated. For more information see <a
    //! href="https://matrix.org/docs/guides/e2e_implementation.html#keys-used-in-end-to-end-encryption">here</a>.
    static std::optional<QOlmAccount> create();
    static std::variant<QOlmAccount, OlmAccountError> unpickle(QByteArray picked, PicklingMode mode);

    //! Serialises an OlmAccount to encrypted Base64.
    std::variant<QByteArray, OlmAccountError> pickle(PicklingMode mode);
    std::variant<IdentityKeys, OlmAccountError> identityKeys();

    //! Returns the signature of the supplied message.
    std::variant<QString, OlmAccountError> sign(QString message) const;

    //! Maximum number of one time keys that this OlmAccount can
    //! currently hold.
    size_t maxNumberOfOneTimeKeys() const;

    //! Generates the supplied number of one time keys.
    std::optional<OlmAccountError> generateOneTimeKeys(size_t numberOfKeys) const;

    //! Gets the OlmAccount's one time keys formatted as JSON.
    std::variant<OneTimeKeys, OlmAccountError> oneTimeKeys() const;

    // HACK do not use directly
    QOlmAccount(OlmAccount *account);
private:
    OlmAccount *m_account;
};
