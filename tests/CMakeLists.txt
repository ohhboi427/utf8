find_package(GTest)
if(NOT GTest_FOUND)
    FetchContent_Declare(
        googletest
        URL "https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz"
    )

    FetchContent_MakeAvailable(googletest)
endif()

add_executable(utf8_tests
    "unit/validation.cpp"
)

target_link_libraries(utf8_tests PRIVATE
    Utf8::Utf8
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(utf8_tests)
