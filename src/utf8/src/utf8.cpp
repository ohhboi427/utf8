#include <utf8/utf8.hpp>

#include <bit>
#include <cstdint>

namespace utf8 {
    namespace {
        [[nodiscard]] constexpr auto char_length(const char8_t leading) noexcept -> uint8_t {
            if((leading & 0x80U) == 0U) {
                return 1U;
            }

            const std::uint8_t leading_ones = std::countl_one(std::bit_cast<std::uint8_t>(leading));
            if(leading_ones < 2U || leading_ones > 4U) {
                return 0U;
            }

            return leading_ones;
        }

        [[nodiscard]] constexpr auto is_cont_byte(const char8_t cont_byte) noexcept -> bool {
            return (cont_byte & 0xC0U) == 0x80U;
        }
    }

    auto decode(const string_view str) noexcept -> std::expected<DecodeResult, Utf8Error> {
        if(str.empty()) {
            return std::unexpected{ Utf8Error::InvalidByteSequence };
        }

        const auto length = char_length(str[0U]);
        if(length == 0U || str.size() < length) {
            return std::unexpected{ Utf8Error::InvalidByteSequence };
        }

        if(length == 1U) {
            return DecodeResult{ .codepoint = static_cast<char32_t>(str[0U]), .length = length };
        }

        char32_t codepoint = str[0U] & ((1U << (7U - length)) - 1U);
        for(std::size_t i = 1U; i < length; ++i) {
            if(!is_cont_byte(str[i])) {
                return std::unexpected{ Utf8Error::InvalidByteSequence };
            }

            codepoint <<= 6U;
            codepoint |= str[i] & 0x3FU;
        }

        if((length == 2U && codepoint < 0x80U) ||
            (length == 3U && codepoint < 0x800U) ||
            (length == 4U && codepoint < 0x10000U)) {
            return std::unexpected{ Utf8Error::OverlongEncoding };
        }

        if((codepoint >= 0xD800U && codepoint <= 0xDFFFU) || codepoint > 0x10FFFFU) {
            return std::unexpected{ Utf8Error::InvalidCodepoint };
        }

        return DecodeResult{ .codepoint = codepoint, .length = length };
    }

    auto encode([[maybe_unused]] const char32_t codepoint) noexcept -> std::array<EncodeResult, 4ZU> {
        return {};
    }
}
