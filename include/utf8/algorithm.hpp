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

    template<std::ranges::input_range R>
        requires std::same_as<std::ranges::range_value_t<R>, char8_t>
    [[nodiscard]] constexpr auto is_valid(R&& range) noexcept -> bool {
        return is_valid(std::ranges::begin(range), std::ranges::end(range));
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

    template<std::ranges::input_range R>
        requires std::same_as<std::ranges::range_value_t<R>, char8_t>
    [[nodiscard]] constexpr auto length(R&& range) noexcept -> Expected<std::size_t> {
        return length(std::ranges::begin(range), std::ranges::end(range));
    }

    template<std::input_iterator I, std::sentinel_for<I> S, std::output_iterator<char8_t> O>
        requires std::same_as<std::iter_value_t<I>, char8_t>
    constexpr auto repair(I it, S end, O out) noexcept -> O {
        while(it != end) {
            auto [new_it, codepoint] = decode(std::move(it), end);

            const auto [units, length]     = *encode(codepoint.value_or(REPLACEMENT));
            auto       [units_it, new_out] = std::ranges::copy(units.begin(), out);

            it  = std::move(new_it);
            out = std::move(new_out);
        }

        return out;
    }

    template<std::ranges::input_range R, std::output_iterator<char8_t> O>
        requires std::same_as<std::ranges::range_value_t<R>, char8_t>
    constexpr auto repair(R&& range, O out) noexcept -> O {
        return repair(std::ranges::begin(range), std::ranges::end(range), std::move(out));
    }
}
