#
# Copyright(c) 2026 Intel Corporation
#
# SPDX-License-Identifier: BSD-3-Clause
#
# NOTE: cmake_minimum_required() and project() are omitted — this subdirectory
# inherits version policy and project scope from the top-level CMakeLists.txt.

include(FetchContent)
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

# Single test binary containing all unit tests.
# POEToolUT.cpp includes POETool.cpp directly (with 'main' macro-renamed) so
# that parse_arg() and the internal types are testable without any source
# modifications.  cxxopts must be in scope for the included translation unit.
add_executable(PoeGenToolTests
    UtilityUT.cpp
    POEToolUT.cpp
)

target_link_libraries(PoeGenToolTests PRIVATE
    PoeGenToolUtility
    cxxopts
    GTest::gtest
    GTest::gtest_main
)

set_target_properties(PoeGenToolTests PROPERTIES
    CXX_STANDARD          20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS        OFF
)

gtest_discover_tests(PoeGenToolTests)
