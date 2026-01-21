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
#include <utility>

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

    [[nodiscard]] constexpr auto decoded_length(const char8_t unit) noexcept -> Expected<std::uint8_t> {
        if((unit & 0x80U) == 0U) {
            return 1U;
        }

        const std::uint8_t length = std::countl_one(std::bit_cast<std::uint8_t>(unit));
        if(length < 2U || length > 4U) {
            return std::unexpected{ Error::InvalidByteSequence };
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

    [[nodiscard]] constexpr auto read_continuation(const char8_t unit) noexcept -> Expected<char8_t> {
        if(!is_continuation(unit)) {
            return std::unexpected{ Error::InvalidCodepoint };
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

    [[nodiscard]] constexpr auto encoded_length(const char32_t codepoint) noexcept -> Expected<std::uint8_t> {
        if(is_invalid(codepoint)) {
            return std::unexpected{ Error::InvalidCodepoint };
        }

        for(std::uint8_t i = 0U; i < static_cast<std::uint8_t>(detail::SEQUENCE_LAST.size()); ++i) {
            if(codepoint < detail::SEQUENCE_LAST[i]) {
                return i + 1U;
            }
        }

        return std::unexpected{ Error::InvalidCodepoint };
    }

    struct EncodeResult {
        std::array<char8_t, 4U> sequence;
        std::uint8_t            length;
    };

    template<std::input_iterator I, std::sentinel_for<I> S>
        requires std::same_as<std::iter_value_t<I>, char8_t>
    [[nodiscard]] constexpr auto decode(I it, S end) noexcept -> std::pair<I, Expected<char32_t>> {
        if(it == end) {
            return { std::move(it), std::unexpected{ Error::InvalidByteSequence } };
        }

        const char8_t leading = *it;
        const auto    length  = decoded_length(leading);
        if(!length) {
            std::ranges::advance(it, 1U, end);

            return { std::move(it), std::unexpected{ length.error() } };
        }

        auto codepoint = static_cast<char32_t>(read_leading(leading, *length));
        std::ranges::advance(it, 1U, end);

        for(std::size_t i = 1U; i < *length; ++i) {
            if(it == end) {
                return { std::move(it), std::unexpected{ Error::InvalidByteSequence } };
            }

            const auto continuation = read_continuation(*it);
            if(!continuation) {
                return { std::move(it), std::unexpected{ continuation.error() } };
            }

            codepoint <<= 6U;
            codepoint |= static_cast<char32_t>(*continuation);

            std::ranges::advance(it, 1U, end);
        }

        if(is_overlong(codepoint, *length)) {
            return { std::move(it), std::unexpected{ Error::OverlongEncoding } };
        }

        if(is_invalid(codepoint)) {
            return { std::move(it), std::unexpected{ Error::InvalidCodepoint } };
        }

        return std::make_pair(std::move(it), codepoint);
    }

    [[nodiscard]] constexpr auto encode(char32_t codepoint) noexcept -> Expected<EncodeResult> {
        if(is_invalid(codepoint)) {
            return std::unexpected{ Error::InvalidCodepoint };
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
