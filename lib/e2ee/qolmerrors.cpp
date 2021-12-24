// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later


#include "qolmerrors.h"
#include <cstring>

Quotient::QOlmError Quotient::fromString(const char* error_raw) {
    if (!strncmp(error_raw, "BAD_ACCOUNT_KEY", 15)) {
        return QOlmError::BadAccountKey;
    } else if (!strncmp(error_raw, "BAD_MESSAGE_KEY_ID", 18)) {
        return QOlmError::BadMessageKeyId;
    } else if (!strncmp(error_raw, "INVALID_BASE64", 14)) {
        return QOlmError::InvalidBase64;
    } else if (!strncmp(error_raw, "NOT_ENOUGH_RANDOM", 17)) {
        return QOlmError::NotEnoughRandom;
    } else if (!strncmp(error_raw, "OUTPUT_BUFFER_TOO_SMALL", 23)) {
        return QOlmError::OutputBufferTooSmall;
    } else {
        return QOlmError::Unknown;
    }
}
