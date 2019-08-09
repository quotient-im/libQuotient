#pragma once

#include "util.h"

#include <QtCore/QStringList>

namespace Quotient {
static const auto CiphertextKeyL = "ciphertext"_ls;
static const auto SenderKeyKeyL = "sender_key"_ls;
static const auto DeviceIdKeyL = "device_id"_ls;
static const auto SessionIdKeyL = "session_id"_ls;

static const auto AlgorithmKeyL = "algorithm"_ls;
static const auto RotationPeriodMsKeyL = "rotation_period_ms"_ls;
static const auto RotationPeriodMsgsKeyL = "rotation_period_msgs"_ls;

static const auto AlgorithmKey = QStringLiteral("algorithm");
static const auto RotationPeriodMsKey = QStringLiteral("rotation_period_ms");
static const auto RotationPeriodMsgsKey =
    QStringLiteral("rotation_period_msgs");

static const auto Ed25519Key = QStringLiteral("ed25519");
static const auto Curve25519Key = QStringLiteral("curve25519");
static const auto SignedCurve25519Key = QStringLiteral("signed_curve25519");
static const auto OlmV1Curve25519AesSha2AlgoKey =
    QStringLiteral("m.olm.v1.curve25519-aes-sha2");
static const auto MegolmV1AesSha2AlgoKey =
    QStringLiteral("m.megolm.v1.aes-sha2");
static const QStringList SupportedAlgorithms = { OlmV1Curve25519AesSha2AlgoKey,
                                                 MegolmV1AesSha2AlgoKey };
} // namespace Quotient
