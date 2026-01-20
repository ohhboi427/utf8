#pragma once

#include "error.hpp"

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <iterator>
#include <ranges>

namespace utf8 {
    constexpr char32_t REPLACEMENT = U'\uFFFD';

    namespace detail {
        constexpr char8_t CONTINUATION_UNIT_HEADER = 0x80U;
        constexpr char8_t CONTINUATION_UNIT_MASK   = 0x3FU;

        constexpr std::array SEQUENCE_LAST = {
            U'\u007F',
            U'\u07FF',
            U'\uFFFF',
            U'\U0010FFFF',
        };

        [[nodiscard]] constexpr auto leading_header(const std::uint8_t length) noexcept -> char8_t {
            if(length == 1U) {
                return 0U;
            }

            return ((1U << length) - 1U) << (8U - length);
        }

        [[nodiscard]] constexpr auto leading_mask(const std::uint8_t length) noexcept -> char8_t {
            if(length == 1U) {
                return 0x7FU;
            }

            return (1U << (7U - length)) - 1U;
        }
    }

    [[nodiscard]] constexpr auto decoded_length(const char8_t unit) noexcept -> Utf8Expected<std::uint8_t> {
        if((unit & 0x80U) == 0U) {
            return 1U;
        }

        const std::uint8_t length = std::countl_one(std::bit_cast<std::uint8_t>(unit));
        if(length < 2U || length > 4U) {
            return std::unexpected{ Utf8Error::InvalidByteSequence };
        }

        return length;
    }

    [[nodiscard]] constexpr auto make_leading(const char8_t unit, const std::uint8_t length) noexcept -> char8_t {
        return detail::leading_header(length) | unit & detail::leading_mask(length);
    }

    [[nodiscard]] constexpr auto read_leading(const char8_t unit, const std::uint8_t length) noexcept -> char8_t {
        return unit & detail::leading_mask(length);
    }

    [[nodiscard]] constexpr auto is_continuation(const char8_t unit) noexcept -> bool {
        return (unit & ~detail::CONTINUATION_UNIT_MASK) == detail::CONTINUATION_UNIT_HEADER;
    }

    [[nodiscard]] constexpr auto make_continuation(const char8_t unit) noexcept -> char8_t {
        return detail::CONTINUATION_UNIT_HEADER | unit & detail::CONTINUATION_UNIT_MASK;
    }

    [[nodiscard]] constexpr auto read_continuation(const char8_t unit) noexcept -> Utf8Expected<char8_t> {
        if(!is_continuation(unit)) {
            return std::unexpected{ Utf8Error::InvalidCodepoint };
        }

        return unit & detail::CONTINUATION_UNIT_MASK;
    }

    [[nodiscard]] constexpr auto is_overlong(const char32_t codepoint, const std::uint8_t length) noexcept -> bool {
        if(length < 2U) {
            return false;
        }

        return codepoint <= detail::SEQUENCE_LAST[length - 2U];
    }

    [[nodiscard]] constexpr auto is_invalid(const char32_t codepoint) noexcept -> bool {
        constexpr char32_t SURROGATE_FIRST = 0xD800U;
        constexpr char32_t SURROGATE_LAST  = 0xDFFFU;

        return
            (codepoint >= SURROGATE_FIRST && codepoint <= SURROGATE_LAST) ||
            codepoint > detail::SEQUENCE_LAST.back();
    }

    [[nodiscard]] constexpr auto encoded_length(const char32_t codepoint) noexcept -> Utf8Expected<std::uint8_t> {
        if(is_invalid(codepoint)) {
            return std::unexpected{ Utf8Error::InvalidCodepoint };
        }

        for(std::uint8_t i = 0U; i < static_cast<std::uint8_t>(detail::SEQUENCE_LAST.size()); ++i) {
            if(codepoint < detail::SEQUENCE_LAST[i]) {
                return i + 1U;
            }
        }

        return std::unexpected{ Utf8Error::InvalidCodepoint };
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
        if(!length) {
            return std::unexpected{ length.error() };
        }

        if(std::ranges::distance(it, end) < *length) {
            return std::unexpected{ Utf8Error::InvalidByteSequence };
        }

        DecodeResult result{
            .codepoint = static_cast<char32_t>(read_leading(*it++, *length)),
            .length    = *length,
        };

        for(std::size_t i = 1U; i < result.length; ++i) {
            const auto continuation = read_continuation(*it);
            if(!continuation) {
                return std::unexpected{ continuation.error() };
            }

            result.codepoint <<= 6U;
            result.codepoint |= static_cast<char32_t>(*continuation);

            ++it;
        }

        if(is_overlong(result.codepoint, result.length)) {
            return std::unexpected{ Utf8Error::OverlongEncoding };
        }

        if(is_invalid(result.codepoint)) {
            return std::unexpected{ Utf8Error::InvalidCodepoint };
        }

        return result;
    }

    template<std::ranges::input_range R>
        requires std::same_as<std::ranges::range_value_t<R>, char8_t>
    [[nodiscard]] constexpr auto decode(R&& range) noexcept -> Utf8Expected<DecodeResult> {
        return decode(std::ranges::begin(range), std::ranges::end(range));
    }

    [[nodiscard]] constexpr auto encode(char32_t codepoint) noexcept -> Utf8Expected<EncodeResult> {
        if(is_invalid(codepoint)) {
            return std::unexpected{ Utf8Error::InvalidCodepoint };
        }

        const auto length = encoded_length(codepoint);
        if(!length) {
            return std::unexpected{ length.error() };
        }

        EncodeResult result{ .sequence = {}, .length = *length };
        for(std::size_t i = result.length - 1U; i > 0U; --i) {
            result.sequence[i] = make_continuation(static_cast<char8_t>(codepoint));

            codepoint >>= 6U;
        }

        result.sequence[0U] = make_leading(static_cast<char8_t>(codepoint), result.length);

        return result;
    }
}
