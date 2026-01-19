add_library(utf8)

target_sources(utf8
    PRIVATE
    "src/utf8.cpp"
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "include"
    FILES
    "include/utf8/error.hpp"
    "include/utf8/string.hpp"
    "include/utf8/utf8.hpp"
)

target_compile_features(utf8
    PUBLIC
    cxx_std_23
)

add_library(Utf8::Utf8 ALIAS utf8)
