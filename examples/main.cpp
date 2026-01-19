#include <utf8/utf8.hpp>

#include <bit>
#include <cstdint>
#include <print>

using namespace utf8::string_view_literals;

auto main() -> int {
    auto str = u8"你好，世界！"sv;

    while(!str.empty()) {
        const auto decode = utf8::decode(str);
        if(!decode) {
            break;
        }

        const auto [codepoint, length] = decode.value();
        std::println("U+{:04X}", std::bit_cast<std::uint32_t>(codepoint));

        str.remove_prefix(length);
    }
}
