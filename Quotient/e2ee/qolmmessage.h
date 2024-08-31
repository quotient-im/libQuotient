// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Quotient/quotient_export.h>

#include <QtCore/QByteArray>
#include <QtCore/QObject>

#include <vodozemac.h>

namespace Quotient {

/*! \brief A wrapper around an olm encrypted message
 *
 * This class encapsulates a Matrix olm encrypted message,
 * passed in either of 2 forms: a general message or a pre-key message.
 *
 * The class provides functions to get a type and the ciphertext.
 */
class QUOTIENT_API QOlmMessage : public QByteArray {
    Q_GADGET
public:
    enum Type {
        PreKey = 0,
        General = 1,
    };
    Q_ENUM(Type)

    explicit QOlmMessage(QByteArray ciphertext, size_t type);

    static QOlmMessage fromCiphertext(const QByteArray &ciphertext);

    Q_INVOKABLE size_t type() const;
    Q_INVOKABLE QByteArray toCiphertext() const;
    olm::OlmMessage message() const;

private:
    size_t m_messageType;
};

} //namespace Quotient
