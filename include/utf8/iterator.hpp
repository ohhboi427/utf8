#pragma once

#include "validation.hpp"

#include <iterator>
#include <type_traits>
#include <utility>

namespace utf8 {
    template<std::input_iterator I, std::sentinel_for<I> S>
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

        [[nodiscard]] friend constexpr auto operator==(const Iterator lhs, std::default_sentinel_t) noexcept -> bool {
            return lhs.m_codepoint == END_OF_STREAM;
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
}
