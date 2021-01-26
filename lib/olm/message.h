// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#ifdef Quotient_E2EE_ENABLED

#include <QtCore/QObject>
#include <QtCore/QByteArray>

namespace Quotient {

/*! \brief A wrapper around an olm encrypted message
 *
 * This class encapsulates a Matrix olm encrypted message,
 * passed in either of 2 forms: a general message or a pre-key message.
 *
 * The class provides functions to get a type and the ciphertext.
 */
class Message : private QByteArray {
    Q_GADGET
public:
    enum Type {
        General,
        PreKey,
    };
    Q_ENUM(Type)

    Message() = default;
    explicit Message(const QByteArray& ciphertext, Type type = General);
    explicit Message(QByteArray ciphertext);

    static Message fromCiphertext(QByteArray ciphertext);

    Q_INVOKABLE Type type() const;
    Q_INVOKABLE QByteArray toCiphertext() const;

private:
    Type _messageType = General;
};


} //namespace Quotient

#endif // Quotient_E2EE_ENABLED
