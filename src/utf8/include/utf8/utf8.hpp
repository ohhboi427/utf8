#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace utf8 {
    using string      = std::basic_string<char8_t>;
    using string_view = std::basic_string_view<char8_t>;

    inline namespace literals {
        inline namespace string_literals {
            using std::literals::string_literals::operator ""s;
        }
    }

    inline namespace literals {
        inline namespace string_view_literals {
            using std::literals::string_view_literals::operator ""sv;
        }
    }

    enum class Utf8Error {
        InvalidByteSequence,
        InvalidCodepoint,
        OverlongEncoding,
    };

    struct DecodeResult {
        char32_t     codepoint;
        std::uint8_t length;
    };

    struct EncodeResult {
        std::array<char8_t, 4U> bytes;
        std::uint8_t            length;
    };

    [[nodiscard]] auto decode(string_view str) noexcept -> std::expected<DecodeResult, Utf8Error>;
    [[nodiscard]] auto encode(char32_t codepoint) noexcept -> std::expected<EncodeResult, Utf8Error>;
}
