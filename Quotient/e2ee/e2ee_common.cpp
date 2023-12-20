// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "e2ee_common.h"

#include "../logging_categories_p.h"

#include <QtCore/QRandomGenerator>

#include <openssl/crypto.h>

using namespace Quotient;

QByteArray Quotient::byteArrayForOlm(size_t bufferSize)
{
    if (bufferSize < std::numeric_limits<QByteArray::size_type>::max())
        return { static_cast<QByteArray::size_type>(bufferSize), '\0' };

    qCritical(E2EE) << "Too large buffer size:" << bufferSize;
    // Zero-length QByteArray is an almost guaranteed way to cause
    // an internal error in QOlm* classes, unless checked
    return {};
}

void Quotient::_impl::reportSpanShortfall(QByteArray::size_type inputSize,
                                          size_t neededSize)
{
    qCCritical(E2EE) << "Not enough bytes to create a valid span: " << inputSize
                     << '<' << neededSize << "- undefined behaviour imminent";
}

void Quotient::fillFromSecureRng(std::span<byte_t> bytes)
{
    // Discussion of QRandomGenerator::system() vs. OpenSSL's RAND_bytes
    //
    // It is a rather close call between the two; to be honest, TL;DR from the
    // below is "it doesn't really matter". Going through the source code of
    // both libraries, along with reading others' investigations like
    // https://github.com/golang/go/issues/33542, left me (@kitsune) with
    // the following:
    // 1. Both ultimately use more or less the same stuff internally - which is
    //    is not surprising and is reassuring in TL;DR. Contrary to occasional
    //    claims on interwebs though, QRandomGenerator does not rely on OpenSSL,
    //    calling system primitives directly instead.
    // 2. Qt's code is massively simpler - again, effectively it's a rather
    //    thin wrapper around what I know for a reasonably good practice across
    //    the platforms supported by Quotient.
    // 3. OpenSSL is more flexible in that you can specify which RNG engine
    //    you actually want: you can, e.g., explicitly use the hardware-based
    //    (aka "true") RNG on modern Intel CPUs. QRandomGenerator "simply"
    //    offers the nice Qt interface to whatever best the operating system
    //    provides (and at least Linux kernel happens to also use true RNGs
    //    where it can, so ¯\(ツ)/¯)
    // 3. QRandomGenerator produces stuff in uint32_t (or uint64_t in case of
    //    QRandomGenerator64) chunks; OpenSSL generates bytes, which is sorta
    //    more convenient. Quotient never needs quantities of randomness that
    //    are not multiples of 16 _bytes_; the Q_UNLIKELY branch below is merely
    //    to cover an edge case that should never happen in the first place.
    //    So, ¯\(ツ)/¯ again.
    //
    // Based on this, QRandomGenerator::system() wins by a tiny non-functional
    // margin; my somewhat educated opinion for now is that both APIs are
    // equally good from the functionality and security point of view.

    // QRandomGenerator::fillRange works in terms of 32-bit words,
    // and FixedBuffer happens to deal with sizes that are multiples
    // of those (16, 32, etc.)
    const qsizetype wordsCount = bytes.size() / 4;
    QRandomGenerator::system()->fillRange(
        reinterpret_cast<uint32_t*>(bytes.data()), wordsCount);
    if (const int remainder = bytes.size() % 4; Q_UNLIKELY(remainder != 0)) {
        // Not normal; but if it happens, apply best effort
        QRandomGenerator::system()->generate(bytes.end() - remainder,
                                             bytes.end());
    }
}

auto initializeSecureHeap()
{
#if !defined(LIBRESSL_VERSION_NUMBER)
    const auto result =
        CRYPTO_secure_malloc_init(FixedBufferBase::TotalSecureHeapSize, 16);
    if (result > 0) {
        qDebug(E2EE) << FixedBufferBase::TotalSecureHeapSize
                     << "bytes of secure heap initialised";
        if (std::atexit([] {
                CRYPTO_secure_malloc_done();
                qDebug(E2EE) << "Dismantled secure heap";
            })
            != 0)
            qWarning(E2EE)
                << "Could not register a cleanup function for secure heap!";
    } else
        qCritical(E2EE) << "Secure heap could not be initialised, sensitive "
                           "data will remain in common dynamic memory";
#else
    const auto result = 0;
    qCritical(E2EE) << "Secure heap is not available in LibreSSL";
#endif
    return result;
}

uint8_t* allocate(size_t bytes, bool initWithZeros = false)
{
#if !defined(LIBRESSL_VERSION_NUMBER)
    static auto secureHeapInitialised [[maybe_unused]] = initializeSecureHeap();

    const auto p = static_cast<uint8_t*>(initWithZeros
                                             ? OPENSSL_secure_zalloc(bytes)
                                             : OPENSSL_secure_malloc(bytes));
    Q_ASSERT(CRYPTO_secure_allocated(p));
    qDebug(E2EE) << "Allocated" << CRYPTO_secure_actual_size(p)
                 << "bytes of secure heap (requested" << bytes << "bytes),"
                 << CRYPTO_secure_used()
                 << "/ 65536 bytes of secure heap used in total";
#else
    const auto p = static_cast<uint8_t*>(initWithZeros
                                             ? calloc(bytes, 1)
                                             : malloc(bytes));
#endif
    return p;
}

FixedBufferBase::FixedBufferBase(size_t bufferSize, InitOptions options)
    : size_(bufferSize)
{
    if (bufferSize >= TotalSecureHeapSize) {
        qCritical(E2EE) << "Too large buffer size:" << bufferSize;
        return;
    }
    if (options == Uninitialized)
        return;

    data_ = allocate(bufferSize, options == FillWithZeros);
    if (options == FillWithRandom)
        fillFromSecureRng({ data_, bufferSize });
}

void FixedBufferBase::fillFrom(QByteArray&& source)
{
    if (unsignedSize(source) != size_) {
        qCritical(E2EE) << "Can't load a fixed buffer of length" << size_
                        << "from a string with length" << source.size();
        Q_ASSERT(unsignedSize(source) == size_); // Always false
        return;
    }
    if (data_ != nullptr) {
        qWarning(E2EE) << "Overwriting the fixed buffer with another string";
        clear();
    }

    data_ = allocate(size_);
    std::copy(source.cbegin(), source.cend(), reinterpret_cast<char*>(data_));
    if (source.isDetached())
        source.clear();
    else
        qWarning(E2EE)
            << "The fixed buffer source is shared; assuming that the caller is "
               "responsible for securely clearing other copies";
}

void FixedBufferBase::clear()
{
    if (empty())
        return;

#if !defined(LIBRESSL_VERSION_NUMBER)
    Q_ASSERT(CRYPTO_secure_allocated(data_));
    const auto actualSize = OPENSSL_secure_actual_size(data_);
    OPENSSL_secure_clear_free(data_, size_);
    qDebug(E2EE) << "Deallocated" << actualSize << "bytes,"
                 << CRYPTO_secure_used() << "/ 65536 bytes of secure heap used";
#else
    free(data_);
#endif
    data_ = nullptr;
}
