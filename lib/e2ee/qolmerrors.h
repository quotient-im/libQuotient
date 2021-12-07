// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>

namespace Quotient {
//! All errors that could be caused by an operation regarding Olm
//! Errors are named exactly like the ones in libolm.
enum QOlmError
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

QOlmError fromString(const std::string &error_raw);

} //namespace Quotient
