#pragma once

#include "error.hpp"

#include <cstddef>
#include <string_view>

namespace utf8 {
    [[nodiscard]] auto is_valid(std::u8string_view sequence) noexcept -> bool;
    [[nodiscard]] auto length(std::u8string_view sequence) noexcept -> Utf8Expected<std::size_t>;
}
