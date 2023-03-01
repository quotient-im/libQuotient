// SPDX-FileCopyrightText: 2021 Alexey Andreyev <aa13q@ya.ru>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "quotient_export.h"

#include <QtCore/QByteArray>
#include <qobjectdefs.h>
#include <olm/olm.h>

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
        PreKey = OLM_MESSAGE_TYPE_PRE_KEY,
        General = OLM_MESSAGE_TYPE_MESSAGE,
    };
    Q_ENUM(Type)

    explicit QOlmMessage(QByteArray ciphertext, Type type = General);

    static QOlmMessage fromCiphertext(const QByteArray &ciphertext);

    Q_INVOKABLE Type type() const;
    Q_INVOKABLE QByteArray toCiphertext() const;

private:
    Type m_messageType = General;
};

} //namespace Quotient
