// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Quotient/converters.h>
#include <Quotient/expected.h>

#include <QtCore/QMetaType>
#include <QtCore/QStringBuilder>

#include <array>
#include <span>
#include <variant>

#include <olm/error.h>

namespace Quotient {

constexpr inline auto AlgorithmKeyL = "algorithm"_ls;
constexpr inline auto RotationPeriodMsKeyL = "rotation_period_ms"_ls;
constexpr inline auto RotationPeriodMsgsKeyL = "rotation_period_msgs"_ls;

constexpr inline auto AlgorithmKey = "algorithm"_ls;
constexpr inline auto RotationPeriodMsKey = "rotation_period_ms"_ls;
constexpr inline auto RotationPeriodMsgsKey = "rotation_period_msgs"_ls;

constexpr inline auto Ed25519Key = "ed25519"_ls;
constexpr inline auto Curve25519Key = "curve25519"_ls;
constexpr inline auto SignedCurve25519Key = "signed_curve25519"_ls;

constexpr inline auto OlmV1Curve25519AesSha2AlgoKey = "m.olm.v1.curve25519-aes-sha2"_ls;
constexpr inline auto MegolmV1AesSha2AlgoKey = "m.megolm.v1.aes-sha2"_ls;

constexpr std::array SupportedAlgorithms { OlmV1Curve25519AesSha2AlgoKey,
                                           MegolmV1AesSha2AlgoKey };

inline bool isSupportedAlgorithm(const QString& algorithm)
{
    return std::find(SupportedAlgorithms.cbegin(), SupportedAlgorithms.cend(),
                     algorithm)
           != SupportedAlgorithms.cend();
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

template <typename T>
using QOlmExpected = Expected<T, OlmErrorCode>;

//! \brief Initialise a buffer object for use with Olm calls
//!
//! Qt and Olm use different size types; this causes the warning noise
QUOTIENT_API QByteArray byteArrayForOlm(size_t bufferSize);

//! \brief Get a size of a container coerced to size_t
//!
//! This is mainly aimed at Qt containers because they have signed size; but it can also be called
//! on other containers or even C arrays, e.g. - to spare generic code from special-casing.
//! For Qt containers, it's a safe cast since size_t can always accommodate the range between 0 and
//! SIZE_MAX / 2 - 1 that they support; yet compilers complain...
inline size_t unsignedSize(const auto& buffer)
    requires (sizeof(std::size(buffer)) <= sizeof(size_t))
{
    return static_cast<size_t>(std::size(buffer));
}

// Can't use std::byte normally recommended for the purpose because both Olm
// and OpenSSL get uint8_t* pointers, and std::byte* is not implicitly
// convertible to uint8_t* (and adding explicit casts in each case kinda defeats
// the purpose of all the span machinery below meant to replace reinterpret_ or
// any other casts).

using byte_t = uint8_t;

template <size_t N = std::dynamic_extent>
using byte_view_t = std::span<const byte_t, N>;

template <size_t N = std::dynamic_extent>
using byte_span_t = std::span<byte_t, N>;

namespace _impl {
    QUOTIENT_API void checkForSpanShortfall(QByteArray::size_type inputSize, int neededSize);

    template <typename SpanT>
    inline auto spanFromBytes(auto& byteArray)
    {
        // OpenSSL only handles int sizes; Release builds will cut the tail off
        Q_ASSERT_X(std::in_range<int>(std::size(byteArray)), __func__, "Too long array for OpenSSL");
        if constexpr (SpanT::extent != std::dynamic_extent) {
            static_assert(std::in_range<int>(SpanT::extent));
            checkForSpanShortfall(std::size(byteArray), static_cast<int>(SpanT::extent));
        }
        return SpanT(reinterpret_cast<typename SpanT::pointer>(std::data(byteArray)),
                     std::min(SpanT::extent, unsignedSize(byteArray)));
    }
} // namespace _impl

//! \brief Obtain a std::span<const byte_t, N> looking into the passed buffer
//!
//! This function returns an adaptor object that is suitable for OpenSSL/Olm
//! invocations (via std::span<>::data() accessor) so that you don't have
//! to wrap your containers into ugly reinterpret_casts on every OpenSSL call.
//! \note The caller is responsible for making sure that bytes.size() is small
//!       enough to fit into an int (OpenSSL only handles int sizes atm) but
//!       also large enough to have at least N bytes if N is not `std::dynamic_extent`
//! \sa asWritableCBytes for the case when you need to pass a buffer for writing
template <size_t N = std::dynamic_extent>
inline auto asCBytes(const auto& buf)
{
    return _impl::spanFromBytes<byte_view_t<N>>(buf);
}

//! Obtain a std::span<byte_t, N> looking into the passed buffer
template <size_t N = std::dynamic_extent>
inline auto asWritableCBytes(auto& buf)
{
    return _impl::spanFromBytes<byte_span_t<N>>(buf);
}

inline auto viewAsByteArray(const auto& aRange) -> auto
    requires (sizeof(*aRange.data()) == sizeof(char))
{ // -> auto to activate SFINAE, it's always QByteArray when well-formed
    return QByteArray::fromRawData(reinterpret_cast<const char*>(
                                       std::data(aRange)),
                                   static_cast<int>(std::size(aRange)));
}

//! Non-template base for owning byte span classes
class QUOTIENT_API FixedBufferBase {
public:
    enum InitOptions { Uninitialized, FillWithZeros, FillWithRandom };

    using value_type = byte_t; // TODO, 0.9: uint8_t -> value_type below
    using size_type = size_t; // TODO, 0.9: size_t -> size_type below

    static constexpr auto TotalSecureHeapSize = 65'536;

    auto size() const { return data_ == nullptr ? 0 : size_; }
    auto empty() const { return data_ == nullptr || size_ == 0; }

    void clear();

    //! \brief Access the bytes of the fixed buffer via QByteArray interface
    //!
    //! This uses QByteArray::fromRawData() to create a QByteArray object that
    //! refers to the original fixed buffer, without copying.
    //! \warning the lifetime of the returned QByteArray should not exceed the
    //!          lifetime of the underlying buffer; in particular, you should
    //!          never try using the result of viewAsByteArray() as a return
    //!          value of your function
    //! \sa copyToByteArray
    QByteArray viewAsByteArray() const
    {
        static_assert(std::in_range<QByteArray::size_type>(TotalSecureHeapSize));
        return QByteArray::fromRawData(reinterpret_cast<const char*>(data_),
                                       static_cast<QByteArray::size_type>(size_));
    }

    //! \brief Copy the contents of the buffer to a QByteArray
    //!
    //! Unlike viewAsByteArray(), this function actually copies the buffer to
    //! non-secure memory.
    QByteArray copyToByteArray(QByteArray::size_type untilPos = -1) const
    {
        if (untilPos < 0 || static_cast<size_type>(untilPos) > size_)
            untilPos = static_cast<QByteArray::size_type>(size_);
        return { reinterpret_cast<const char*>(data_), untilPos };
    }

    // TODO, 0.9: merge the overloads

    QByteArray toBase64() const { return viewAsByteArray().toBase64(); }
    QByteArray toBase64(QByteArray::Base64Options options) const
    {
        return viewAsByteArray().toBase64(options);
    }

    Q_DISABLE_COPY(FixedBufferBase)
    FixedBufferBase& operator=(FixedBufferBase&&) = delete;

protected:
    FixedBufferBase(size_t bufferSize, InitOptions options);
    ~FixedBufferBase() { clear(); }

    FixedBufferBase(FixedBufferBase&& other)
        : data_(std::exchange(other.data_, nullptr)), size_(other.size_)
    {}

    void fillFrom(QByteArray&& source);

    uint8_t* dataForWriting() { return data_; }
    const uint8_t* data() const { return data_; }

private:
    uint8_t* data_ = nullptr;
    size_t size_ = 0;
};

template <size_t ExtentN = std::dynamic_extent, bool DataIsWriteable = true>
class QUOTIENT_API FixedBuffer : public FixedBufferBase {
public:
    static constexpr auto extent = ExtentN; // Matching std::span
    static_assert(extent == std::dynamic_extent
                  || (extent < TotalSecureHeapSize / 2 && extent % 4 == 0));

    FixedBuffer() // TODO, 0.9: merge with the next constructor
        requires(extent != std::dynamic_extent)
        : FixedBuffer(FillWithZeros)
    {}
    explicit FixedBuffer(InitOptions fillMode)
        requires(extent != std::dynamic_extent)
        : FixedBufferBase(ExtentN, fillMode)
    {}
    explicit FixedBuffer(size_t bufferSize)
        requires(extent == std::dynamic_extent)
        : FixedBuffer(bufferSize, FillWithZeros)
    {}
    explicit FixedBuffer(size_t bufferSize, InitOptions fillMode)
        requires(extent == std::dynamic_extent)
        : FixedBufferBase(bufferSize, fillMode)
    {}

    using FixedBufferBase::data;
    uint8_t* data() requires DataIsWriteable { return dataForWriting(); }

    // NOLINTNEXTLINE(google-explicit-constructor)
    QUO_IMPLICIT operator byte_view_t<ExtentN>() const
    {
        return byte_view_t<ExtentN>(data(), size());
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    QUO_IMPLICIT operator byte_span_t<ExtentN>()
        requires DataIsWriteable
    {
        return byte_span_t<ExtentN>(dataForWriting(), size());
    }
};

inline auto getRandom(size_t bytes)
{
    return FixedBuffer<>{ bytes, FixedBufferBase::FillWithRandom };
}

template <size_t SizeN>
inline auto getRandom()
{
    return FixedBuffer<SizeN>{ FixedBufferBase::FillWithRandom };
}

//! \brief Fill the buffer with the securely generated random bytes
//!
//! You should use this throughout Quotient where pseudo-random generators
//! are not enough (i.e. in crypto cases). Don't use it when proper randomness
//! is not critical; it tries to rely on system entropy that is in (somewhat)
//! limited supply.
//! There's no fancy stuff internally, it's just a way to unify secure RNG usage
//! in Quotient. See the function definition for details if you want/need.
QUOTIENT_API void fillFromSecureRng(std::span<byte_t> bytes);

class PicklingKey : public FixedBuffer<128, /*DataIsWriteable=*/false> {
private:
    // `using` would have exposed the constructor as it's public in the parent
    explicit PicklingKey(InitOptions options) : FixedBuffer(options)
    {
        Q_ASSERT(options != FillWithZeros);
    }

public:
    static PicklingKey generate() { return PicklingKey(FillWithRandom); }
    static PicklingKey fromByteArray(QByteArray&& keySource)
    {
        PicklingKey k(Uninitialized);
        k.fillFrom(std::move(keySource));
        return k;
    }
    static PicklingKey mock() { return PicklingKey(Uninitialized); }
};

struct IdentityKeys
{
    // Despite being Base64 payloads, these keys are stored in QStrings because
    // in the vast majority of cases they are used to read from or write to
    // QJsonObjects, and that effectively requires QStrings
    QString curve25519;
    QString ed25519;
};

//! Struct representing the one-time keys.
struct UnsignedOneTimeKeys
{
    QHash<QString, QHash<QString, QString>> keys;

    //! Get the HashMap containing the curve25519 one-time keys.
    QHash<QString, QString> curve25519() const { return keys[Curve25519Key]; }
};

class QUOTIENT_API SignedOneTimeKey {
public:
    explicit SignedOneTimeKey(const QString& unsignedKey, const QString& userId,
                              const QString& deviceId,
                              const QByteArray& signature)
        : payload{
            { "key"_ls, unsignedKey },
            { "signatures"_ls,
              QJsonObject{
                  { userId, QJsonObject{ { "ed25519:"_ls % deviceId,
                                           QString::fromUtf8(signature) } } } } }
        }
    {}
    explicit SignedOneTimeKey(const QJsonObject& jo = {})
        : payload(jo)
    {}

    //! Unpadded Base64-encoded 32-byte Curve25519 public key
    QByteArray key() const { return payload["key"_ls].toString().toLatin1(); }

    //! \brief Signatures of the key object
    //!
    //! The signature is calculated using the process described at
    //! https://spec.matrix.org/v1.3/appendices/#signing-json
    auto signatures() const
    {
        return fromJson<QHash<QString, QHash<QString, QString>>>(
            payload["signatures"_ls]);
    }

    QByteArray signature(QStringView userId, QStringView deviceId) const
    {
        return payload["signatures"_ls][userId]["ed25519:"_ls % deviceId]
            .toString()
            .toLatin1();
    }

    //! Whether the key is a fallback key
    bool isFallback() const { return payload["fallback"_ls].toBool(); }
    auto toJson() const { return payload; }
    auto toJsonForVerification() const
    {
        auto json = payload;
        json.remove("signatures"_ls);
        json.remove("unsigned"_ls);
        return QJsonDocument(json).toJson(QJsonDocument::Compact);
    }

private:
    QJsonObject payload;
};

using OneTimeKeys = QHash<QString, std::variant<QString, SignedOneTimeKey>>;

} // namespace Quotient

Q_DECLARE_METATYPE(Quotient::SignedOneTimeKey)
