// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later


#include "qolmerrors.h"
#include "util.h"
#include <QtCore/QLatin1String>

Quotient::QOlmError Quotient::fromString(const char* error_raw) {
    const QLatin1String error { error_raw };
    if (error_raw == "BAD_ACCOUNT_KEY"_ls) {
        return QOlmError::BadAccountKey;
    } else if (error_raw == "BAD_MESSAGE_KEY_ID"_ls) {
        return QOlmError::BadMessageKeyId;
    } else if (error_raw == "INVALID_BASE64"_ls) {
        return QOlmError::InvalidBase64;
    } else if (error_raw == "NOT_ENOUGH_RANDOM"_ls) {
        return QOlmError::NotEnoughRandom;
    } else if (error_raw == "OUTPUT_BUFFER_TOO_SMALL"_ls) {
        return QOlmError::OutputBufferTooSmall;
    } else {
        return QOlmError::Unknown;
    }
}
