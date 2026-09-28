cmake_minimum_required(VERSION 3.12)
project(libmo VERSION 1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LIBMO_BUILD_TESTS "Build libmo tests" ON)

add_library(mo STATIC
    src/mo.cpp
    src/moalloc.cpp
    src/mohook.cpp
)

target_include_directories(mo
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(mo PRIVATE /W4)
else()
    target_compile_options(mo PRIVATE -Wall -Wextra -Werror)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(mo PRIVATE NDEBUG)
endif()

set_target_properties(mo PROPERTIES
    OUTPUT_NAME mo
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

if(LIBMO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
