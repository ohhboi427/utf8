#pragma once

#include "error.hpp"

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <tuple>
#include <utility>

namespace utf8 {
    namespace detail {
        inline constexpr std::array SEQUENCE_LAST = {
            U'\u007F',
            U'\u07FF',
            U'\uFFFF',
            U'\U0010FFFF',
        };
    }

    [[nodiscard]] constexpr auto is_invalid(const char32_t codepoint) noexcept -> bool {
        constexpr char32_t SURROGATE_FIRST = 0xD800U;
        constexpr char32_t SURROGATE_LAST  = 0xDFFFU;

        return
            (codepoint >= SURROGATE_FIRST && codepoint <= SURROGATE_LAST) ||
            codepoint > detail::SEQUENCE_LAST.back();
    }

    namespace detail {
        inline constexpr char8_t CONTINUATION_UNIT_HEADER = 0x80U;
        inline constexpr char8_t CONTINUATION_UNIT_MASK   = 0x3FU;

        inline constexpr std::array<char8_t, 4U> LEADER_UNIT_HEADERS = {
            0x00U,
            0xC0U,
            0xE0U,
            0xF0U,
        };

        inline constexpr std::array<char8_t, 4U> LEADER_UNIT_MASKS = {
            0x7FU,
            0x1FU,
            0x0FU,
            0x07U,
        };

        [[nodiscard]] constexpr auto decoded_length(const char8_t unit) noexcept -> Expected<std::uint8_t> {
            const std::uint8_t length = std::countl_one(std::bit_cast<std::uint8_t>(unit));
            if(length == 0U) {
                return 1U;
            }

            if(length < 2U || length > 4U) {
                return Unexpected{ Error::InvalidByteSequence };
            }

            return length;
        }

        [[nodiscard]] constexpr auto make_leading(const char8_t unit, const std::uint8_t length) noexcept -> char8_t {
            return LEADER_UNIT_HEADERS[length - 1U] | unit & LEADER_UNIT_MASKS[length - 1U];
        }

        [[nodiscard]] constexpr auto read_leading(
            const char8_t unit
        ) noexcept -> Expected<std::pair<char8_t, std::uint8_t>> {
            const auto length = decoded_length(unit);
            if(!length) {
                return Unexpected{ length.error() };
            }

            return std::make_pair(unit & LEADER_UNIT_MASKS[*length - 1U], *length);
        }

        [[nodiscard]] constexpr auto make_continuation(const char8_t unit) noexcept -> char8_t {
            return CONTINUATION_UNIT_HEADER | unit & CONTINUATION_UNIT_MASK;
        }

        [[nodiscard]] constexpr auto read_continuation(const char8_t unit) noexcept -> Expected<char8_t> {
            if((unit & ~CONTINUATION_UNIT_MASK) != CONTINUATION_UNIT_HEADER) {
                return Unexpected{ Error::InvalidByteSequence };
            }

            return unit & CONTINUATION_UNIT_MASK;
        }

        [[nodiscard]] constexpr auto encoded_length(const char32_t codepoint) noexcept -> Expected<std::uint8_t> {
            if(is_invalid(codepoint)) {
                return Unexpected{ Error::InvalidCodepoint };
            }

            for(std::uint8_t i = 0U; i < static_cast<std::uint8_t>(SEQUENCE_LAST.size()); ++i) {
                if(codepoint <= SEQUENCE_LAST[i]) {
                    return i + 1U;
                }
            }

            return Unexpected{ Error::InvalidCodepoint };
        }

        [[nodiscard]] constexpr auto is_overlong(const char32_t codepoint, const std::uint8_t length) noexcept -> bool {
            if(length < 2U) {
                return false;
            }

            return codepoint <= SEQUENCE_LAST[length - 2U];
        }
    }

    using Decode = char32_t;

    template<std::input_iterator I, std::sentinel_for<I> S, std::output_iterator<char8_t> O>
        requires std::same_as<std::iter_value_t<I>, char8_t>
    [[nodiscard]] constexpr auto decode_into(I it, S end, O out) noexcept -> std::tuple<I, O, Expected<char32_t>> {
        if(it == end) {
            return { std::move(it), std::move(out), Unexpected{ Error::InvalidByteSequence } };
        }

        const auto leading_length = detail::read_leading(*it);

        *out = *it;
        ++out;

        std::ranges::advance(it, 1U, end);

        if(!leading_length) {
            return { std::move(it), std::move(out), Unexpected{ leading_length.error() } };
        }

        const auto [leading, length] = *leading_length;

        auto codepoint = static_cast<char32_t>(leading);
        for(std::size_t i = 1U; i < length; ++i) {
            if(it == end) {
                return { std::move(it), std::move(out), Unexpected{ Error::InvalidByteSequence } };
            }

            const auto continuation = detail::read_continuation(*it);
            if(!continuation) {
                return { std::move(it), std::move(out), Unexpected{ continuation.error() } };
            }

            codepoint <<= 6U;
            codepoint |= static_cast<char32_t>(*continuation);

            *out = *it;
            ++out;

            std::ranges::advance(it, 1U, end);
        }

        if(detail::is_overlong(codepoint, length)) {
            return { std::move(it), std::move(out), Unexpected{ Error::OverlongEncoding } };
        }

        if(is_invalid(codepoint)) {
            return { std::move(it), std::move(out), Unexpected{ Error::InvalidCodepoint } };
        }

        return { std::move(it), std::move(out), codepoint };
    }

    template<std::input_iterator I, std::sentinel_for<I> S>
        requires std::same_as<std::iter_value_t<I>, char8_t>
    [[nodiscard]] constexpr auto decode(I it, S end) noexcept -> std::pair<I, Expected<Decode>> {
        struct DiscardIterator {
            using iterator_category = std::output_iterator_tag;
            using iterator_concept  = std::output_iterator_tag;
            using value_type        = void;
            using reference         = void;
            using pointer           = void;
            using difference_type   = std::iter_difference_t<I>;

            [[nodiscard]] constexpr auto operator*() const noexcept {
                return std::ignore;
            }

            constexpr auto operator++() noexcept -> DiscardIterator& {
                return *this;
            }

            constexpr auto operator++(int) noexcept -> DiscardIterator {
                return *this;
            }
        };

        auto [new_it, new_out, codepoint] = decode_into(std::move(it), end, DiscardIterator{});

        return { std::move(new_it), std::move(codepoint) };
    }

    struct Encode {
        std::array<char8_t, 4U> units;
        std::uint8_t            length;

        [[nodiscard]] constexpr auto size() const noexcept -> std::size_t {
            return length;
        }

        [[nodiscard]] constexpr auto begin(this auto&& self) noexcept {
            return self.units.begin();
        }

        [[nodiscard]] constexpr auto end(this auto&& self) noexcept {
            return self.units.begin() + self.length;
        }
    };

    [[nodiscard]] constexpr auto encode(char32_t codepoint) noexcept -> Expected<Encode> {
        const auto length = detail::encoded_length(codepoint);
        if(!length) {
            return Unexpected{ length.error() };
        }

        Encode result{ .units = {}, .length = *length };

        if(result.length == 1U) {
            result.units[0U] = static_cast<char8_t>(codepoint);

            return result;
        }

        for(std::size_t i = result.length - 1U; i > 0U; --i) {
            result.units[i] = detail::make_continuation(static_cast<char8_t>(codepoint));

            codepoint >>= 6U;
        }

        result.units[0U] = detail::make_leading(static_cast<char8_t>(codepoint), result.length);

        return result;
    }

    inline constexpr char32_t BOM         = U'\uFEFF';
    inline constexpr char32_t REPLACEMENT = U'\uFFFD';

    inline constexpr auto BOM_UNITS         = *encode(BOM);
    inline constexpr auto REPLACEMENT_UNITS = *encode(REPLACEMENT);
}
