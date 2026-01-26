#pragma once

#include "error.hpp"
#include "validation.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <utility>

namespace utf8 {
    template<std::input_iterator I, std::sentinel_for<I> S>
        requires std::same_as<std::iter_value_t<I>, char8_t>
    [[nodiscard]] constexpr auto is_valid(I it, S end) noexcept -> bool {
        while(it != end) {
            auto [new_it, codepoint] = decode(std::move(it), end);
            if(!codepoint) {
                return false;
            }

            it = std::move(new_it);
        }

        return true;
    }

    template<std::input_iterator I, std::sentinel_for<I> S>
        requires std::same_as<std::iter_value_t<I>, char8_t>
    [[nodiscard]] constexpr auto length(I it, S end) noexcept -> Expected<std::size_t> {
        std::size_t result = 0U;

        while(it != end) {
            auto [new_it, codepoint] = decode(std::move(it), end);
            if(!codepoint) {
                return Unexpected{ codepoint.error() };
            }

            it = std::move(new_it);

            ++result;
        }

        return result;
    }

    template<std::input_iterator I, std::sentinel_for<I> S, std::output_iterator<char8_t> O>
        requires std::same_as<std::iter_value_t<I>, char8_t>
    constexpr auto repair(I it, S end, O out) noexcept -> O {
        while(it != end) {
            auto [new_it, codepoint] = decode(std::move(it), end);

            const auto units = *encode(codepoint.value_or(REPLACEMENT));

            out = std::ranges::copy(units, std::move(out)).out;

            it = std::move(new_it);
        }

        return out;
    }

    template<std::input_iterator I, std::sentinel_for<I> S, std::output_iterator<char32_t> O>
        requires std::same_as<std::iter_value_t<I>, char8_t>
    constexpr auto decode_all(I it, S end, O out) noexcept -> O {
        while(it != end) {
            auto [new_it, codepoint] = decode(std::move(it), end);

            *out = codepoint.value_or(REPLACEMENT);
            ++out;

            it = std::move(new_it);
        }

        return out;
    }

    template<std::input_iterator I, std::sentinel_for<I> S, std::output_iterator<char32_t> O>
        requires std::same_as<std::iter_value_t<I>, char8_t>
    [[nodiscard]] constexpr auto decode_strict(I it, S end, O out) noexcept -> std::pair<O, Expected<void>> {
        while(it != end) {
            auto [new_it, codepoint] = decode(std::move(it), end);
            if(!codepoint) {
                return { std::move(out), Unexpected{ codepoint.error() } };
            }

            *out = *codepoint;
            ++out;

            it = std::move(new_it);
        }

        return { std::move(out), {} };
    }

    template<std::input_iterator I, std::sentinel_for<I> S, std::output_iterator<char8_t> O>
        requires std::same_as<std::iter_value_t<I>, char32_t>
    constexpr auto encode_all(I it, S end, O out) noexcept -> O {
        while(it != end) {
            char32_t codepoint = *it;
            if(is_invalid(codepoint)) {
                codepoint = REPLACEMENT;
            }

            const auto units = *encode(codepoint);

            out = std::ranges::copy(units, std::move(out)).out;

            std::ranges::advance(it, 1U, end);
        }

        return out;
    }

    template<std::input_iterator I, std::sentinel_for<I> S, std::output_iterator<char8_t> O>
        requires std::same_as<std::iter_value_t<I>, char32_t>
    [[nodiscard]] constexpr auto encode_strict(I it, S end, O out) noexcept -> std::pair<O, Expected<void>> {
        while(it != end) {
            const auto units = encode(*it);
            if(!units) {
                return { std::move(out), Unexpected{ units.error() } };
            }

            out = std::ranges::copy(*units, std::move(out)).out;

            std::ranges::advance(it, 1U, end);
        }

        return { std::move(out), {} };
    }

    namespace ranges {
        template<std::ranges::input_range R>
            requires std::same_as<std::ranges::range_value_t<R>, char8_t>
        [[nodiscard]] constexpr auto is_valid(R&& range) noexcept -> bool {
            return utf8::is_valid(std::ranges::begin(range), std::ranges::end(range));
        }

        template<std::ranges::input_range R>
            requires std::same_as<std::ranges::range_value_t<R>, char8_t>
        [[nodiscard]] constexpr auto length(R&& range) noexcept -> Expected<std::size_t> {
            return utf8::length(std::ranges::begin(range), std::ranges::end(range));
        }

        template<std::ranges::input_range R, std::output_iterator<char8_t> O>
            requires std::same_as<std::ranges::range_value_t<R>, char8_t>
        constexpr auto repair(R&& range, O out) noexcept -> O {
            return utf8::repair(std::ranges::begin(range), std::ranges::end(range), std::move(out));
        }

        template<std::ranges::input_range R, std::output_iterator<char32_t> O>
            requires std::same_as<std::ranges::range_value_t<R>, char8_t>
        constexpr auto decode_all(R&& range, O out) noexcept -> O {
            return utf8::decode_all(std::ranges::begin(range), std::ranges::end(range), std::move(out));
        }

        template<std::ranges::input_range R, std::output_iterator<char32_t> O>
            requires std::same_as<std::ranges::range_value_t<R>, char8_t>
        [[nodiscard]] constexpr auto decode_strict(R&& range, O out) noexcept -> std::pair<O, Expected<void>> {
            return utf8::decode_strict(std::ranges::begin(range), std::ranges::end(range), std::move(out));
        }

        template<std::ranges::input_range R, std::output_iterator<char8_t> O>
            requires std::same_as<std::ranges::range_value_t<R>, char32_t>
        constexpr auto encode_all(R&& range, O out) noexcept -> O {
            return utf8::encode_all(std::ranges::begin(range), std::ranges::end(range), std::move(out));
        }

        template<std::ranges::input_range R, std::output_iterator<char8_t> O>
            requires std::same_as<std::ranges::range_value_t<R>, char32_t>
        [[nodiscard]] constexpr auto encode_strict(R&& range, O out) noexcept -> std::pair<O, Expected<void>> {
            return utf8::encode_strict(std::ranges::begin(range), std::ranges::end(range), std::move(out));
        }
    }
}
