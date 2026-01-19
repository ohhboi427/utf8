#pragma once

#include "error.hpp"
#include "string.hpp"

#include <array>
#include <cstdint>

namespace utf8 {
    struct DecodeResult {
        char32_t     codepoint;
        std::uint8_t length;
    };

    struct EncodeResult {
        std::array<char8_t, 4U> sequence;
        std::uint8_t            length;
    };

    [[nodiscard]] auto decode(string_view sequence) noexcept -> Utf8Expected<DecodeResult>;
    [[nodiscard]] auto encode(char32_t codepoint) noexcept -> Utf8Expected<EncodeResult>;
}
