#include <utf8/utf8.hpp>

#include <bit>
#include <print>
#include <string_view>

using namespace std::string_view_literals;

auto main() -> int {
    constexpr auto str = u8"Helló, Világ!"sv;

    if(const auto length = utf8::ranges::length(str); length) {
        std::println("{}", length.value());
    }

    for(const auto codepoint : str | utf8::views::as_utf8) {
        std::println("U+{:04X}", std::bit_cast<std::uint32_t>(codepoint));
    }
}
