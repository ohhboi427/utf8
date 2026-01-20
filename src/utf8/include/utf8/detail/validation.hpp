#pragma once

#include <utf8/error.hpp>

#include <array>
#include <bit>
#include <cstdint>
#include <ranges>
#include <string_view>

namespace utf8::detail {
    constexpr char32_t REPLACEMENT = U'\uFFFD';

    constexpr char32_t SURROGATE_FIRST = 0xD800U;
    constexpr char32_t SURROGATE_LAST  = 0xDFFFU;

    constexpr std::array SEQUENCE_LAST = {
        U'\u007F',
        U'\u07FF',
        U'\uFFFF',
        U'\U0010FFFF',
    };

    constexpr char8_t CONTINUATION_UNIT_HEADER = 0x80U;
    constexpr char8_t CONTINUATION_UNIT_MASK   = 0x3FU;

    [[nodiscard]] constexpr auto leading_header(const std::uint8_t length) noexcept -> char8_t {
        return ((1U << length) - 1U) << (8U - length);
    }

    [[nodiscard]] constexpr auto leading_mask(const std::uint8_t length) noexcept -> char8_t {
        return (1U << (7U - length)) - 1U;
    }

    [[nodiscard]] constexpr auto is_continuation(const char8_t unit) noexcept -> bool {
        return (unit & 0xC0U) == 0x80U;
    }

    [[nodiscard]] constexpr auto decoded_length(const char8_t unit) noexcept -> std::uint8_t {
        if((unit & 0x80U) == 0U) {
            return 1U;
        }

        const std::uint8_t length = std::countl_one(std::bit_cast<std::uint8_t>(unit));
        if(length < 2U || length > 4U) {
            return 0U;
        }

        return length;
    }

    [[nodiscard]] constexpr auto encoded_length(const char32_t codepoint) noexcept -> std::uint8_t {
        for(std::uint8_t i = 0U; i < static_cast<std::uint8_t>(SEQUENCE_LAST.size()); ++i) {
            if(codepoint < SEQUENCE_LAST[i]) {
                return i + 1U;
            }
        }

        return 0U;
    }

    struct DecodeResult {
        char32_t     codepoint;
        std::uint8_t length;
    };

    struct EncodeResult {
        std::array<char8_t, 4U> sequence;
        std::uint8_t            length;
    };

    template<std::input_iterator I, std::sentinel_for<I> S>
        requires std::same_as<std::iter_value_t<I>, char8_t>
    [[nodiscard]] constexpr auto decode(I it, S end) noexcept -> Utf8Expected<DecodeResult> {
        if(it == end) {
            return std::unexpected{ Utf8Error::InvalidByteSequence };
        }

        const auto length = decoded_length(*it);
        if(length == 0U || std::ranges::distance(it, end) < length) {
            return std::unexpected{ Utf8Error::InvalidByteSequence };
        }

        if(length == 1U) {
            return DecodeResult{ .codepoint = static_cast<char32_t>(*it), .length = length };
        }

        char32_t codepoint = *it++ & leading_mask(length);
        for(std::size_t i = 1U; i < length; ++i) {
            if(!is_continuation(*it)) {
                return std::unexpected{ Utf8Error::InvalidByteSequence };
            }

            codepoint <<= 6U;
            codepoint |= *it & CONTINUATION_UNIT_MASK;

            ++it;
        }

        if(codepoint <= SEQUENCE_LAST[length - 2U]) {
            return std::unexpected{ Utf8Error::OverlongEncoding };
        }

        if((codepoint >= SURROGATE_FIRST && codepoint <= SURROGATE_LAST) || codepoint > SEQUENCE_LAST.back()) {
            return std::unexpected{ Utf8Error::InvalidCodepoint };
        }

        return DecodeResult{ .codepoint = codepoint, .length = length };
    }

    template<std::ranges::input_range R>
        requires std::same_as<std::ranges::range_value_t<R>, char8_t>
    [[nodiscard]] constexpr auto decode(R&& range) noexcept -> Utf8Expected<DecodeResult> {
        return decode(std::ranges::begin(range), std::ranges::end(range));
    }

    [[nodiscard]] constexpr auto encode(char32_t codepoint) noexcept -> Utf8Expected<EncodeResult> {
        if(codepoint >= SURROGATE_FIRST && codepoint <= SURROGATE_LAST) {
            return std::unexpected{ Utf8Error::InvalidCodepoint };
        }

        const auto length = encoded_length(codepoint);
        if(length == 0U) {
            return std::unexpected{ Utf8Error::InvalidCodepoint };
        }

        if(length == 1U) {
            return EncodeResult{ .sequence = { static_cast<char8_t>(codepoint), 0U, 0U, 0U }, .length = length };
        }

        std::array<char8_t, 4U> sequence{};
        for(std::size_t i = length - 1U; i > 0U; --i) {
            sequence[i] = CONTINUATION_UNIT_HEADER | (static_cast<char8_t>(codepoint) & CONTINUATION_UNIT_MASK);

            codepoint >>= 6U;
        }

        sequence[0U] = leading_header(length) | (static_cast<char8_t>(codepoint) & leading_mask(length));

        return EncodeResult{ .sequence = sequence, .length = length };
    }
}
