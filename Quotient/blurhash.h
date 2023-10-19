// SPDX-FileCopyrightText: 2024 Joshua Goins <josh@redstrate.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "quotient_export.h"

#include <QImage>

class TestBlurHash;

namespace Quotient {
/**
 * @brief Encodes and decodes image to and from the BlurHash format. See https://blurha.sh/.
 *
 * @note This class has been adapted from https://github.com/redstrate/QtBlurHash.
 */
class QUOTIENT_API BlurHash
{
public:
    /** Decodes the @p blurhash string creating an image of @p size.
     * @note Returns a null image if decoding failed.
     */
    static QUOTIENT_API QImage decode(const QString &blurhash, const QSize &size);

    /** Encodes the @p image and returns a blurhash string.
     * @param image A non-null image.
     * @param componentsX the number of components X-wise. Must be between 1 and 9.
     * @param componentsY the number of components Y-wise. Must be between 1 and 9.
     * @note Returns an empty string if it failed to encode the image.
     */
    static QUOTIENT_API QString encode(const QImage &image, int componentsX = 4, int componentsY = 4);

protected:
    struct Components {
        int x, y;

        bool operator==(const Components &other) const
        {
            return x == other.x && y == other.y;
        }
    };

    /**
     * @brief Decodes a base 83 string to it's integer value. Returns std::nullopt if there's an invalid character in the blurhash.
     */
    static QUOTIENT_API std::optional<int> decode83(const QString &encodedString);

    /**
     * @brief Encodes an integer to it's base 83 representation.
     */
    static QUOTIENT_API QString encode83(int value);

    /**
     * @brief Unpacks an integer to it's @c Components value.
     */
    static QUOTIENT_API Components unpackComponents(int packedComponents);

    /**
     * @brief Packs @c Components to it's integer representation.
     */
    static QUOTIENT_API int packComponents(const Components &components);

    /**
     * @brief Decodes a encoded max AC component value.
     */
    static QUOTIENT_API  float decodeMaxAC(int value);

    /**
     * @brief Encodes the maximum AC component value to an integer repsentation.
     */
    static QUOTIENT_API int encodeMaxAC(float value);

    /**
     * @brief Decodes the average color from the encoded RGB value.
     * @note This returns the color as SRGB.
     */
    static QUOTIENT_API QColor decodeAverageColor(int encodedValue);

    /**
     * @brief Encodes the average color into it's integer representation.
     */
    static QUOTIENT_API int encodeAverageColor(const QColor &averageColor);

    /**
     * @brief Calls pow() with @p exp on @p value, while keeping the sign.
     */
    static QUOTIENT_API float signPow(float value, float exp);

    /**
     * @brief Decodes a encoded AC component value.
     */
    static QUOTIENT_API QColor decodeAC(int value, float maxAC);

    /**
     * @brief Encodes the AC component into it's integer representation.
     */
    static QUOTIENT_API int encodeAC(QColor value, float maxAC);

    /**
     * @brief Calculates the weighted sum for @p dimension across @p components.
     */
    static QUOTIENT_API QList<float> calculateWeights(qsizetype dimension, qsizetype components);

    friend class ::TestBlurHash;
};
} // namespace Quotient