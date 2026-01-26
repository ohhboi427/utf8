#include <utf8/utf8.hpp>

#include <bit>
#include <print>
#include <string_view>

using namespace std::string_view_literals;

auto main() -> int {
    constexpr auto str = u8"\uFEFFHelló, \xFF Világ!"sv;

    if(const auto length = utf8::ranges::length(str | utf8::views::sanitize); length) {
        std::println("{}", length.value());
    }

    for(const auto codepoint : str | utf8::views::decode) {
        std::println("U+{:04X}", std::bit_cast<std::uint32_t>(codepoint));
    }
}
