#pragma once

#include "iterator.hpp"

#include <concepts>
#include <ranges>
#include <utility>

namespace utf8::ranges {
    template<std::ranges::view V>
        requires std::same_as<std::ranges::range_value_t<V>, char8_t>
    class View : public std::ranges::view_interface<View<V>> {
    public:
        View() = default;

        explicit constexpr View(V view) noexcept
            : m_view{ std::move(view) } {}

        [[nodiscard]] constexpr V base() const & noexcept
            requires std::copy_constructible<V> {
            return m_view;
        }

        [[nodiscard]] constexpr auto base() && noexcept -> V {
            return std::move(m_view);
        }

        [[nodiscard]] constexpr auto begin(this auto&& self) noexcept {
            return Iterator{ std::ranges::begin(self.m_view), std::ranges::end(self.m_view) };
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

    struct AsChars : std::ranges::range_adaptor_closure<AsChars> {
        template<std::ranges::viewable_range R>
            requires std::same_as<std::ranges::range_value_t<R>, char8_t>
        [[nodiscard]] static constexpr auto operator()(R&& range) noexcept {
            return std::forward<R>(range) | std::views::transform(
                [](const char8_t c) noexcept -> char {
                    return static_cast<char>(c);
                }
            );
        }
    };

    struct AsU8Chars : std::ranges::range_adaptor_closure<AsU8Chars> {
        template<std::ranges::viewable_range R>
            requires std::same_as<std::ranges::range_value_t<R>, char>
        [[nodiscard]] static constexpr auto operator()(R&& range) noexcept {
            return std::forward<R>(range) | std::views::transform(
                [](const char c) noexcept -> char8_t {
                    return static_cast<char8_t>(c);
                }
            );
        }
    };

    namespace views {
        inline constexpr AsUtf8Fn as_utf8{};

        inline constexpr AsChars   as_chars{};
        inline constexpr AsU8Chars as_u8chars{};
    }
}

namespace utf8 {
    namespace views = ranges::views;
}
