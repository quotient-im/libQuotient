// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QByteArray>

#include "e2ee/e2ee.h"

namespace Quotient {

// Convert PicklingMode to key
QUOTIENT_API QByteArray toKey(const PicklingMode &mode);

//! \brief Initialise a buffer object for use with Olm calls
//!
//! Qt and Olm use different size types; this causes the warning noise
QUOTIENT_API QByteArray bufferForOlm(size_t bufferSize);

//! \brief Get a size of Qt container coerced to size_t
//!
//! It's a safe cast since size_t can easily accommodate the range between
//! 0 and INTMAX - 1 that Qt containers support; yet compilers complain...
inline size_t unsignedSize(auto qtBuffer)
{
    // The buffer size cannot be negative by definition, so this is deemed safe
    return static_cast<size_t>(qtBuffer.size());
}

QUOTIENT_API void fillWithRandom(std::span<char, std::dynamic_extent> buffer);

inline void fillWithRandom(auto buffer)
    requires(!std::is_const_v<decltype(buffer.begin())>)
{
    fillWithRandom({ buffer.begin(), buffer.end() });
}

class QUOTIENT_API RandomBuffer : public QByteArray {
public:
    explicit RandomBuffer(size_t size) : QByteArray(bufferForOlm(size))
    {
        fillWithRandom({ begin(), end() });
    }
    ~RandomBuffer() { clear(); }

    // NOLINTNEXTLINE(google-explicit-constructor)
    QUO_IMPLICIT operator void*() { return data(); }
    char* chars() { return data(); }
    uint8_t* bytes() { return reinterpret_cast<uint8_t*>(data()); }

    Q_DISABLE_COPY(RandomBuffer)
    RandomBuffer(RandomBuffer&&) = default;
    void operator=(RandomBuffer&&) = delete;
};

[[deprecated("Create RandomBuffer directly")]] //
inline RandomBuffer getRandom(size_t bufferSize)
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
