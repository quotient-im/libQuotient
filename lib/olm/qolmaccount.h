// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include "olm/e2ee.h"
#include "olm/errors.h"
#include "olm/olm.h"
#include <QObject>

struct OlmAccount;

namespace Quotient {

//! An olm account manages all cryptographic keys used on a device.
//! \code{.cpp}
//! const auto olmAccount = new QOlmAccount(this);
//! \endcode
class QOlmAccount
{
public:
    ~QOlmAccount();

    //! Creates a new instance of OlmAccount. During the instantiation
    //! the Ed25519 fingerprint key pair and the Curve25519 identity key
    //! pair are generated. For more information see <a
    //! href="https://matrix.org/docs/guides/e2e_implementation.html#keys-used-in-end-to-end-encryption">here</a>.
    static std::optional<QOlmAccount> create();
    static std::variant<QOlmAccount, OlmError> unpickle(QByteArray &picked, const PicklingMode &mode);

    //! Serialises an OlmAccount to encrypted Base64.
    std::variant<QByteArray, OlmError> pickle(const PicklingMode &mode);
    std::variant<IdentityKeys, OlmError> identityKeys();

    //! Returns the signature of the supplied message.
    std::variant<QString, OlmError> sign(const QString &message) const;

    //! Maximum number of one time keys that this OlmAccount can
    //! currently hold.
    size_t maxNumberOfOneTimeKeys() const;

    //! Generates the supplied number of one time keys.
    std::optional<OlmError> generateOneTimeKeys(size_t numberOfKeys) const;

    //! Gets the OlmAccount's one time keys formatted as JSON.
    std::variant<OneTimeKeys, OlmError> oneTimeKeys() const;

    // HACK do not use directly
    QOlmAccount(OlmAccount *account);
private:
    OlmAccount *m_account;
};

} // namespace Quotient
