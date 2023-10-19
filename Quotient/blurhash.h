// SPDX-FileCopyrightText: 2022 DeepBlueV7.X <https://github.com/deepbluev7>
// SPDX-FileCopyrightText: 2023 Joshua Goins <josh@redstrate.com>
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "quotient_export.h"

namespace Quotient {

struct DecodedImage {
    size_t width, height;
    std::vector<unsigned char> image; // pixels rgb
};

// Decode a blurhash to an image with size width*height
QUOTIENT_API DecodedImage decode_blurhash(std::string_view blurhash, size_t width, size_t height,
             size_t bytesPerPixel = 3) noexcept;

// Encode an image of rgb pixels (without padding) with size width*height into a
// blurhash with x*y components
QUOTIENT_API std::string encode_blurhash(unsigned char* image, size_t width, size_t height, int x,
                   int y);

}