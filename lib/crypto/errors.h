// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#ifdef Quotient_E2EE_ENABLED
#include <string>

namespace Quotient {
//! All errors that could be caused by an operation regarding Olm
//! Errors are named exactly like the ones in libolm.
enum OlmError
{
    BadAccountKey,
    BadMessageFormat,
    BadMessageKeyId,
    BadMessageMac,
    BadMessageVersion,
    InvalidBase64,
    NotEnoughRandom,
    OutputBufferTooSmall,
    UnknownMessageIndex,
    Unknown,
};

OlmError fromString(const std::string &error_raw);

} //namespace Quotient

#endif
