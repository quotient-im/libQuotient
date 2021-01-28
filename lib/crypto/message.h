// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#ifdef Quotient_E2EE_ENABLED

#include <QObject>
#include <QByteArray>

namespace Quotient {

/*! \brief A wrapper around an olm encrypted message
 *
 * This class encapsulates a Matrix olm encrypted message,
 * passed in either of 2 forms: a general message or a pre-key message.
 *
 * The class provides functions to get a type and the ciphertext.
 */
class Message : public QByteArray {
    Q_GADGET
public:
    enum Type {
        General,
        PreKey,
    };
    Q_ENUM(Type)

    Message() = default;
    explicit Message(const QByteArray &ciphertext, Type type = General);
    explicit Message(const Message &message);

    static Message fromCiphertext(const QByteArray &ciphertext);

    Q_INVOKABLE Type type() const;
    Q_INVOKABLE QByteArray toCiphertext() const;

private:
    Type m_messageType = General;
};


} //namespace Quotient

#endif // Quotient_E2EE_ENABLED
