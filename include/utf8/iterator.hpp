#pragma once

#include "validation.hpp"

#include <array>
#include <concepts>
#include <iterator>
#include <type_traits>
#include <utility>

namespace utf8 {
    template<std::input_iterator I, std::sentinel_for<I> S>
        requires std::same_as<std::iter_value_t<I>, char8_t>
    class Iterator {
        static constexpr char32_t END_OF_STREAM = 0xFFFFFFFFU;

    public:
        using iterator_category = std::input_iterator_tag;
        using iterator_concept  = std::conditional_t<
            std::forward_iterator<I>,
            std::forward_iterator_tag,
            std::input_iterator_tag
        >;

        using value_type      = char32_t;
        using reference       = char32_t;
        using pointer         = void;
        using difference_type = std::iter_difference_t<I>;

        Iterator() = default;

        explicit constexpr Iterator(I it, S end) noexcept
            : m_it{ std::move(it) }, m_end{ std::move(end) } {
            next();

            if(m_codepoint == BOM) {
                next();
            }
        }

        [[nodiscard]] constexpr auto operator*() const noexcept -> reference {
            return m_codepoint;
        }

        constexpr auto operator++() noexcept -> Iterator& {
            next();

            return *this;
        }

        constexpr auto operator++(int) noexcept {
            if constexpr(std::forward_iterator<I>) {
                const auto copy = *this;

                next();

                return copy;
            } else {
                class Proxy {
                public:
                    explicit constexpr Proxy(const value_type codepoint) noexcept
                        : m_codepoint{ codepoint } {}

                    [[nodiscard]] constexpr auto operator*() const noexcept -> reference {
                        return m_codepoint;
                    }

                private:
                    value_type m_codepoint;
                };

                const Proxy proxy{ m_codepoint };

                next();

                return proxy;
            }
        }

        [[nodiscard]] friend constexpr auto operator==(const Iterator lhs, const Iterator rhs) noexcept -> bool {
            return lhs.m_it == rhs.m_it && lhs.m_codepoint == rhs.m_codepoint;
        }

        [[nodiscard]] friend constexpr auto operator==(const Iterator it, std::default_sentinel_t) noexcept -> bool {
            return it.m_codepoint == END_OF_STREAM;
        }

    private:
        I          m_it{};
        S          m_end{};
        value_type m_codepoint{};

        auto next() noexcept -> void {
            if(m_it == m_end) {
                m_codepoint = END_OF_STREAM;
                return;
            }

            auto [it, codepoint] = decode(std::move(m_it), m_end);

            m_it        = std::move(it);
            m_codepoint = codepoint.value_or(REPLACEMENT);
        }
    };

    template<std::input_iterator I, std::sentinel_for<I> S>
        requires std::same_as<std::iter_value_t<I>, char8_t>
    class SanIterator {
        static constexpr std::array<char8_t, 4U> END_OF_STREAM_UNITS = { 0xFFU, 0xFFU, 0xFFU, 0xFFU };

    public:
        using iterator_category = std::input_iterator_tag;
        using iterator_concept  = std::conditional_t<
            std::forward_iterator<I>,
            std::forward_iterator_tag,
            std::input_iterator_tag
        >;

        using value_type      = char8_t;
        using reference       = char8_t;
        using pointer         = void;
        using difference_type = std::iter_difference_t<I>;

        SanIterator() = default;

        explicit constexpr SanIterator(I it, S end) noexcept
            : m_it{ std::move(it) }, m_end{ std::move(end) } {
            next();

            if(m_buffer == BOM_UNITS.units) {
                m_buffer_index = m_buffer.size();

                next();
            }
        }

        [[nodiscard]] constexpr auto operator*() const noexcept -> reference {
            return m_buffer[m_buffer_index];
        }

        constexpr auto operator++() noexcept -> SanIterator& {
            next();

            return *this;
        }

        constexpr auto operator++(int) noexcept {
            if constexpr(std::forward_iterator<I>) {
                const auto copy = *this;

                next();

                return copy;
            } else {
                class Proxy {
                public:
                    explicit constexpr Proxy(const value_type unit) noexcept
                        : m_unit{ unit } {}

                    [[nodiscard]] constexpr auto operator*() const noexcept -> reference {
                        return m_unit;
                    }

                private:
                    value_type m_unit;
                };

                const Proxy proxy{ m_buffer[m_buffer_index] };

                next();

                return proxy;
            }
        }

        [[nodiscard]] friend constexpr auto operator==(const SanIterator lhs, const SanIterator rhs) noexcept -> bool {
            return
                lhs.m_it == rhs.m_it &&
                lhs.m_buffer == rhs.m_buffer &&
                lhs.m_buffer_index == rhs.m_buffer_index &&
                lhs.m_buffer_size == rhs.m_buffer_size;
        }

        [[nodiscard]] friend constexpr auto operator==(const SanIterator it, std::default_sentinel_t) noexcept -> bool {
            return it.m_buffer == END_OF_STREAM_UNITS;
        }

    private:
        I                       m_it{};
        S                       m_end{};
        std::array<char8_t, 4U> m_buffer{};
        std::size_t             m_buffer_size{};
        std::size_t             m_buffer_index{};

        auto next() noexcept -> void {
            if(++m_buffer_index < m_buffer_size) {
                return;
            }

            m_buffer_index = 0U;

            if(m_it == m_end) {
                m_buffer      = END_OF_STREAM_UNITS;
                m_buffer_size = 4U;
                return;
            }

            auto [it, out, codepoint] = decode_into(std::move(m_it), m_end, m_buffer.begin());

            m_it = std::move(it);

            if(codepoint) {
                std::ranges::fill(out, m_buffer.end(), 0U);

                m_buffer_size = std::ranges::distance(m_buffer.begin(), out);
            } else {
                m_buffer      = REPLACEMENT_UNITS.units;
                m_buffer_size = REPLACEMENT_UNITS.length;
            }
        }
    };
}
