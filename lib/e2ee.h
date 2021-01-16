// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "util.h"

#include <QtCore/QStringList>

namespace Quotient {
inline const auto CiphertextKeyL = "ciphertext"_ls;
inline const auto SenderKeyKeyL = "sender_key"_ls;
inline const auto DeviceIdKeyL = "device_id"_ls;
inline const auto SessionIdKeyL = "session_id"_ls;

inline const auto AlgorithmKeyL = "algorithm"_ls;
inline const auto RotationPeriodMsKeyL = "rotation_period_ms"_ls;
inline const auto RotationPeriodMsgsKeyL = "rotation_period_msgs"_ls;

inline const auto AlgorithmKey = QStringLiteral("algorithm");
inline const auto RotationPeriodMsKey = QStringLiteral("rotation_period_ms");
inline const auto RotationPeriodMsgsKey =
    QStringLiteral("rotation_period_msgs");

inline const auto Ed25519Key = QStringLiteral("ed25519");
inline const auto Curve25519Key = QStringLiteral("curve25519");
inline const auto SignedCurve25519Key = QStringLiteral("signed_curve25519");
inline const auto OlmV1Curve25519AesSha2AlgoKey =
    QStringLiteral("m.olm.v1.curve25519-aes-sha2");
inline const auto MegolmV1AesSha2AlgoKey =
    QStringLiteral("m.megolm.v1.aes-sha2");
inline const QStringList SupportedAlgorithms = { OlmV1Curve25519AesSha2AlgoKey,
                                                 MegolmV1AesSha2AlgoKey };
} // namespace Quotient
