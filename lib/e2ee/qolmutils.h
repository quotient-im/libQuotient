// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QByteArray>

#include "e2ee/e2ee.h"

namespace Quotient {

// Convert PicklingMode to key
QUOTIENT_API QByteArray toKey(const PicklingMode &mode);

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

[[deprecated("Create RandomBuffer directly")]] inline auto getRandom(
    size_t bufferSize)
{
    return RandomBuffer(bufferSize);
}

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

} // namespace Quotient
