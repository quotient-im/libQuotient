// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

//! All errors that could be caused by an operation regarding an `QOlmAccount`.
//! Errors are named exactly like the ones in libolm.
enum OlmAccountError {
    BadAccountKey,
    BadMessageKeyId,
    InvalidBase64,
    NotEnoughRandom,
    OutputBufferTooSmall,
    Unknown,
};

//! All errors that could be caused by an operation regarding an `QOlmSession`.
//! Errors are named exactly like the ones in libolm.
enum OlmSessionError {
    BadAccountKey,
    BadMessageFormat,
    BadMessageKeyId,
    BadMessageMac,
    BadMessageVersion,
    InvalidBase64,
    NotEnoughRandom,
    OutputBufferTooSmall,
    Unknown,
};

//! All errors that could be caused by an operation
//! regarding QOlmOutboundGroupSession and QOlmInboundGroupSession.
//! Errors are named exactly like the ones in libolm.
enum OlmGroupSessionError {
    BadAccountKey,
    BadMessageFormat,
    BadMessageMac,
    BadMessageVersion,
    BadSessionKey,
    InvalidBase64,
    NotEnoughRandom,
    OutputBufferTooSmall,
    UnknownMessageIndex,
    Unknown,
};
