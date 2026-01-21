add_library(utf8)
add_library(Utf8::Utf8 ALIAS utf8)

target_sources(utf8
    PRIVATE
    "src/utf8.cpp"
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "include"
    FILES
    "include/utf8/utf8.hpp"
    "include/utf8/error.hpp"
    "include/utf8/validation.hpp"
)

target_compile_features(utf8
    PUBLIC
    cxx_std_23
)

if(MSVC)
    target_compile_options(utf8
        PUBLIC
        /utf-8
    )
endif()
