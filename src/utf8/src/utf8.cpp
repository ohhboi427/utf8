#include <utf8/utf8.hpp>

#include <bit>
#include <cstdint>

namespace utf8 {
    namespace {
        constexpr char32_t SURROGATE_FIRST = 0xD800U;
        constexpr char32_t SURROGATE_LAST  = 0xDFFFU;

        constexpr char32_t MB1_LAST = U'\u007F';
        constexpr char32_t MB2_LAST = U'\u07FF';
        constexpr char32_t MB3_LAST = U'\uFFFF';
        constexpr char32_t MB4_LAST = U'\U0010FFFF';

        constexpr std::uint8_t CONT_HEADER = 0x80U;
        constexpr std::uint8_t CONT_MASK   = 0x3FU;

        [[nodiscard]] constexpr auto leading_header(const std::uint8_t length) noexcept -> std::uint8_t {
            return ((1U << length) - 1U) << (8U - length);
        }

        [[nodiscard]] constexpr auto leading_mask(const std::uint8_t length) noexcept -> std::uint8_t {
            return (1U << (7U - length)) - 1U;
        }

        [[nodiscard]] constexpr auto is_cont_byte(const char8_t cont_byte) noexcept -> bool {
            return (cont_byte & 0xC0U) == 0x80U;
        }

        [[nodiscard]] constexpr auto mb_length(const char8_t leading) noexcept -> uint8_t {
            if((leading & 0x80U) == 0U) {
                return 1U;
            }

            const std::uint8_t leading_ones = std::countl_one(std::bit_cast<std::uint8_t>(leading));
            if(leading_ones < 2U || leading_ones > 4U) {
                return 0U;
            }

            return leading_ones;
        }

        [[nodiscard]] constexpr auto cp_length(const char32_t codepoint) noexcept -> std::uint8_t {
            if(codepoint <= MB1_LAST) {
                return 1U;
            }

            if(codepoint <= MB2_LAST) {
                return 2U;
            }

            if(codepoint <= MB3_LAST) {
                return 3U;
            }

            if(codepoint <= MB4_LAST) {
                return 4U;
            }

            return 0U;
        }
    }

    auto decode(const string_view str) noexcept -> std::expected<DecodeResult, Utf8Error> {
        if(str.empty()) {
            return std::unexpected{ Utf8Error::InvalidByteSequence };
        }

        const auto length = mb_length(str[0U]);
        if(length == 0U || str.size() < length) {
            return std::unexpected{ Utf8Error::InvalidByteSequence };
        }

        if(length == 1U) {
            return DecodeResult{ .codepoint = static_cast<char32_t>(str[0U]), .length = length };
        }

        char32_t codepoint = str[0U] & leading_mask(length);
        for(std::size_t i = 1U; i < length; ++i) {
            if(!is_cont_byte(str[i])) {
                return std::unexpected{ Utf8Error::InvalidByteSequence };
            }

            codepoint <<= 6U;
            codepoint |= str[i] & CONT_MASK;
        }

        if((length == 2U && codepoint <= MB1_LAST) ||
            (length == 3U && codepoint <= MB2_LAST) ||
            (length == 4U && codepoint <= MB3_LAST)) {
            return std::unexpected{ Utf8Error::OverlongEncoding };
        }

        if((codepoint >= SURROGATE_FIRST && codepoint <= SURROGATE_LAST) || codepoint > MB4_LAST) {
            return std::unexpected{ Utf8Error::InvalidCodepoint };
        }

        return DecodeResult{ .codepoint = codepoint, .length = length };
    }

    auto encode(char32_t codepoint) noexcept -> std::expected<EncodeResult, Utf8Error> {
        if(codepoint >= SURROGATE_FIRST && codepoint <= SURROGATE_LAST) {
            return std::unexpected{ Utf8Error::InvalidCodepoint };
        }

        const auto length = cp_length(codepoint);
        if(length == 0U) {
            return std::unexpected{ Utf8Error::InvalidCodepoint };
        }

        if(length == 1U) {
            return EncodeResult{ .bytes = { static_cast<char8_t>(codepoint), 0U, 0U, 0U }, .length = length };
        }

        std::array<char8_t, 4U> bytes{};
        for(std::size_t i = length - 1U; i > 0U; --i) {
            bytes[i]  = CONT_HEADER | (codepoint & CONT_MASK);
            codepoint >>= 6U;
        }

        bytes[0U] = leading_header(length) | (codepoint & leading_mask(length));

        return EncodeResult{ .bytes = bytes, .length = length };
    }
}
