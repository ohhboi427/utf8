#pragma once

#include <string>
#include <string_view>

namespace utf8 {
    using string      = std::basic_string<char8_t>;
    using string_view = std::basic_string_view<char8_t>;

    inline namespace literals {
        inline namespace string_literals {
            using std::literals::string_literals::operator ""s;
        }
    }

    inline namespace literals {
        inline namespace string_view_literals {
            using std::literals::string_view_literals::operator ""sv;
        }
    }
}
