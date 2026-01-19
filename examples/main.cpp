#include <utf8/utf8.hpp>

#include <print>
#include <string_view>

using namespace std::string_view_literals;

auto main() -> int {
    constexpr auto str = u8"Helló, Világ!"sv;
    if(const auto length = utf8::length(str); length) {
        std::println("{}", length.value());
    }
}
