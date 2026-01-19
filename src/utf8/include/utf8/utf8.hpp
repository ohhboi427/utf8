#pragma once

#include "error.hpp"
#include "detail/validation.hpp"

#include <algorithm>
#include <compare>
#include <cstddef>
#include <iterator>
#include <string_view>

namespace utf8 {
    [[nodiscard]] constexpr auto is_valid(std::u8string_view sequence) noexcept -> bool {
        while(!sequence.empty()) {
            const auto decode_result = detail::decode(sequence);
            if(!decode_result) {
                return false;
            }

            const auto [codepoint, length] = decode_result.value();
            sequence.remove_prefix(length);
        }

        return true;
    }

    [[nodiscard]] constexpr auto length(std::u8string_view sequence) noexcept -> Utf8Expected<std::size_t> {
        std::size_t result = 0U;

        while(!sequence.empty()) {
            const auto decode_result = detail::decode(sequence);
            if(!decode_result) {
                return std::unexpected{ decode_result.error() };
            }

            const auto [codepoint, length] = decode_result.value();
            sequence.remove_prefix(length);

            ++result;
        }

        return result;
    }

    class Iterator {
    public:
        using iterator_concept  = std::forward_iterator_tag;
        using iterator_category = std::input_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = char32_t;
        using pointer           = void;
        using reference         = char32_t;

        explicit constexpr Iterator(const char8_t* const ptr) noexcept
            : m_ptr{ ptr } {}

        [[nodiscard]] constexpr auto operator*() const noexcept -> reference {
            const auto result = detail::decode(std::u8string_view{ m_ptr, 4U });
            if(!result) {
                return detail::REPLACEMENT;
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
            return lhs.m_ptr == rhs.m_ptr;
        }

        [[nodiscard]] friend constexpr auto operator<=>(
            const Iterator lhs,
            const Iterator rhs
        ) noexcept -> std::strong_ordering {
            return lhs.m_ptr <=> rhs.m_ptr;
        }

    private:
        const char8_t* m_ptr{};

        auto advance() noexcept -> void {
            const auto length = detail::decoded_length(*m_ptr);

            m_ptr += std::max(length, static_cast<std::uint8_t>(1U));
        }
    };

    class View {
    public:
        using value_type      = char32_t;
        using reference       = char32_t;
        using const_reference = char32_t;
        using iterator        = Iterator;
        using const_iterator  = Iterator;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;

        explicit constexpr View(const std::u8string_view str) noexcept
            : m_str{ str } {}

        [[nodiscard]] constexpr auto begin() const noexcept -> iterator {
            return iterator{ m_str.data() };
        }

        [[nodiscard]] constexpr auto end() const noexcept -> iterator {
            return iterator{ m_str.data() + m_str.size() };
        }

    private:
        std::u8string_view m_str{};
    };
}
