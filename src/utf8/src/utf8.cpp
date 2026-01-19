#include <utf8/utf8.hpp>

#include "validation.hpp"

namespace utf8 {
    auto is_valid(std::u8string_view sequence) noexcept -> bool {
        while(!sequence.empty()) {
            const auto decode_result = decode(sequence);
            if(!decode_result) {
                return false;
            }

            const auto [codepoint, length] = decode_result.value();
            sequence.remove_prefix(length);
        }

        return true;
    }

    auto length(std::u8string_view sequence) noexcept -> Utf8Expected<std::size_t> {
        std::size_t result = 0U;

        while(!sequence.empty()) {
            const auto decode_result = decode(sequence);
            if(!decode_result) {
                return std::unexpected{ decode_result.error() };
            }

            const auto [codepoint, length] = decode_result.value();
            sequence.remove_prefix(length);

            ++result;
        }

        return result;
    }
}
