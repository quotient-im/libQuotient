// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later
#ifdef Quotient_E2EE_ENABLED
#include "crypto/errors.h"

Quotient::OlmError Quotient::fromString(const std::string &error_raw) {
    if (error_raw.compare("BAD_ACCOUNT_KEY")) {
        return OlmError::BadAccountKey;
    } else if (error_raw.compare("BAD_MESSAGE_KEY_ID")) {
        return OlmError::BadMessageKeyId;
    } else if (error_raw.compare("INVALID_BASE64")) {
        return OlmError::InvalidBase64;
    } else if (error_raw.compare("NOT_ENOUGH_RANDOM")) {
        return OlmError::NotEnoughRandom;
    } else if (error_raw.compare("OUTPUT_BUFFER_TOO_SMALL")) {
        return OlmError::OutputBufferTooSmall;
    } else {
        return OlmError::Unknown;
    }
}
#endif
