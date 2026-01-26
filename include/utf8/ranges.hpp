#pragma once

#include "iterator.hpp"

#include <concepts>
#include <ranges>
#include <utility>

namespace utf8::ranges {
    template<std::ranges::view V>
        requires std::same_as<std::ranges::range_value_t<V>, char8_t>
    class DecodeView : public std::ranges::view_interface<DecodeView<V>> {
    public:
        DecodeView() = default;

        explicit constexpr DecodeView(V view) noexcept
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
    DecodeView(R&&) -> DecodeView<std::views::all_t<R>>;

    struct Decode : std::ranges::range_adaptor_closure<Decode> {
        template<std::ranges::viewable_range R>
            requires std::same_as<std::ranges::range_value_t<R>, char8_t>
        [[nodiscard]] static constexpr auto operator()(R&& range) noexcept {
            return DecodeView{ std::forward<R>(range) };
        }
    };

    template<std::ranges::view V>
        requires std::same_as<std::ranges::range_value_t<V>, char8_t>
    class SanitizeView : public std::ranges::view_interface<DecodeView<V>> {
    public:
        SanitizeView() = default;

        explicit constexpr SanitizeView(V view) noexcept
            : m_view{ std::move(view) } {}

        [[nodiscard]] constexpr V base() const & noexcept
            requires std::copy_constructible<V> {
            return m_view;
        }

        [[nodiscard]] constexpr auto base() && noexcept -> V {
            return std::move(m_view);
        }

        [[nodiscard]] constexpr auto begin(this auto&& self) noexcept {
            return SanIterator{ std::ranges::begin(self.m_view), std::ranges::end(self.m_view) };
        }

        [[nodiscard]] static constexpr auto end() noexcept {
            return std::default_sentinel_t{};
        }

    private:
        V m_view{};
    };

    template<std::ranges::viewable_range R>
        requires std::same_as<std::ranges::range_value_t<R>, char8_t>
    SanitizeView(R&&) -> SanitizeView<std::views::all_t<R>>;

    struct Sanitize : std::ranges::range_adaptor_closure<Sanitize> {
        template<std::ranges::viewable_range R>
            requires std::same_as<std::ranges::range_value_t<R>, char8_t>
        [[nodiscard]] static constexpr auto operator()(R&& range) noexcept {
            return SanitizeView{ std::forward<R>(range) };
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
        inline constexpr Decode   decode{};
        inline constexpr Sanitize sanitize{};

        inline constexpr AsChars   as_chars{};
        inline constexpr AsU8Chars as_u8chars{};
    }
}

namespace utf8 {
    namespace views = ranges::views;
}
