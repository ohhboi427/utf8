#pragma once

#include "error.hpp"
#include "validation.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <iterator>
#include <ranges>
#include <utility>

namespace utf8 {
    template<std::input_iterator I, std::sentinel_for<I> S>
        requires std::same_as<std::iter_value_t<I>, char8_t>
    [[nodiscard]] constexpr auto is_valid(I it, S end) noexcept -> bool {
        while(it != end) {
            const auto decode_result = decode(it, end);
            if(!decode_result) {
                return false;
            }

            const std::uint8_t step = std::max(decode_result->length, static_cast<std::uint8_t>(1U));

            std::ranges::advance(it, step, end);
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
    [[nodiscard]] constexpr auto length(I it, S end) noexcept -> Utf8Expected<std::size_t> {
        std::size_t result = 0U;

        while(it != end) {
            const auto decode_result = decode(it, end);
            if(!decode_result) {
                return std::unexpected{ decode_result.error() };
            }

            const std::uint8_t step = std::max(decode_result->length, static_cast<std::uint8_t>(1U));

            std::ranges::advance(it, step, end);
            ++result;
        }

        return result;
    }

    template<std::ranges::input_range R>
        requires std::same_as<std::ranges::range_value_t<R>, char8_t>
    [[nodiscard]] constexpr auto length(R&& range) noexcept -> Utf8Expected<std::size_t> {
        return length(std::ranges::begin(range), std::ranges::end(range));
    }

    template<std::forward_iterator I, std::sentinel_for<I> S>
    class Iterator {
    public:
        using iterator_concept  = std::forward_iterator_tag;
        using iterator_category = std::input_iterator_tag;
        using value_type        = char32_t;
        using reference         = char32_t;
        using pointer           = void;
        using difference_type   = std::iter_difference_t<I>;

        Iterator() = default;

        explicit constexpr Iterator(I it, S end) noexcept
            : m_it{ std::move(it) }, m_end{ std::move(end) } {}

        [[nodiscard]] constexpr auto operator*() const noexcept -> reference {
            const auto result = decode(m_it, m_end);
            if(!result) {
                return REPLACEMENT;
            }

            return result->codepoint;
        }

        constexpr auto operator++() noexcept -> Iterator& {
            advance();

            return *this;
        }

        constexpr auto operator++(int) noexcept -> Iterator {
            const auto copy = *this;

            advance();

            return copy;
        }

        [[nodiscard]] friend constexpr auto operator==(const Iterator lhs, const Iterator rhs) noexcept -> bool {
            return lhs.m_it == rhs.m_it;
        }

        [[nodiscard]] friend constexpr auto operator==(const Iterator lhs, std::default_sentinel_t) noexcept -> bool {
            return lhs.m_it == lhs.m_end;
        }

    private:
        I m_it{};
        S m_end{};

        auto advance() noexcept -> void {
            if(m_it == m_end) {
                return;
            }

            const auto step = decoded_length(*m_it).value_or(1U);

            std::ranges::advance(m_it, step, m_end);
        }
    };

    template<std::ranges::view V>
        requires std::same_as<std::ranges::range_value_t<V>, char8_t>
    class View : public std::ranges::view_interface<View<V>> {
    public:
        View() = default;

        explicit constexpr View(V view) noexcept
            : m_view{ std::move(view) } {}

        [[nodiscard]] constexpr auto begin() const noexcept {
            return Iterator{ std::ranges::begin(m_view), std::ranges::end(m_view) };
        }

        [[nodiscard]] static constexpr auto end() noexcept {
            return std::default_sentinel_t{};
        }

    private:
        V m_view{};
    };

    template<std::ranges::viewable_range R>
        requires std::same_as<std::ranges::range_value_t<R>, char8_t>
    View(R&&) -> View<std::views::all_t<R>>;

    struct AsUtf8Fn : std::ranges::range_adaptor_closure<AsUtf8Fn> {
        template<std::ranges::viewable_range R>
            requires std::same_as<std::ranges::range_value_t<R>, char8_t>
        [[nodiscard]] static constexpr auto operator()(R&& range) noexcept {
            return View{ std::forward<R>(range) };
        }
    };

    inline constexpr AsUtf8Fn as_utf8{};
}
