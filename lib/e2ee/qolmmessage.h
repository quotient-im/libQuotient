// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "quotient_export.h"

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
class QUOTIENT_API QOlmMessage : public QByteArray {
    Q_GADGET
public:
    enum Type {
        General,
        PreKey,
    };
    Q_ENUM(Type)

    QOlmMessage() = default;
    explicit QOlmMessage(QByteArray ciphertext, Type type = General);
    explicit QOlmMessage(const QOlmMessage &message);
    ~QOlmMessage() = default;

    static QOlmMessage fromCiphertext(const QByteArray &ciphertext);

    Q_INVOKABLE Type type() const;
    Q_INVOKABLE QByteArray toCiphertext() const;

private:
    Type m_messageType = General;
};

} //namespace Quotient
