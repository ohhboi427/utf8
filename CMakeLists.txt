cmake_minimum_required(VERSION 3.29...4.0)

project(Utf8
    LANGUAGES CXX
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

include(FetchContent)

add_subdirectory("src")
add_subdirectory("examples")
