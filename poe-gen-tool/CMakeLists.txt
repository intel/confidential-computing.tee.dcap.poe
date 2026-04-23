# Minimum required CMake version (3.13+ for FetchContent and modern features)
cmake_minimum_required(VERSION 3.13)

# Define project name and languages
project(poe-gen-tool LANGUAGES CXX)

option(BUILD_TESTS "Build poe-gen-tool unit tests" ON)

# Find OpenSSL for cryptographic functionality
# Request static libraries so the binary is self-contained and does not depend
# on the target system's OpenSSL shared library version.
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

# Set C++ standard to C++20 (required for modern features)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Disable compiler-specific extensions

# Define common compiler flags for all build types
set(COMMON_FLAGS
    -ffunction-sections
    -fdata-sections
    -fPIE
    -Wall -Wextra -Winit-self -Wpointer-arith -Wreturn-type
    -Waddress -Wsequence-point -Wformat -Wformat-security -Werror=format-security
    -Wmissing-include-dirs -Wfloat-equal -Wundef -Wshadow
    -Wcast-align -Wconversion -Wredundant-decls
    -fno-delete-null-pointer-checks
    -fno-strict-overflow
)

# Enable stack protection (strong if compiler supports it)
if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9")
    list(APPEND COMMON_FLAGS -fstack-protector)
else()
    list(APPEND COMMON_FLAGS -fstack-protector-strong)
endif()

# Enable Control-flow Enforcement Technology (CET) for GCC >= 8
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "8")
    list(APPEND COMMON_FLAGS -fcf-protection)
endif()

# Set _FORTIFY_SOURCE level (default to 2 if not specified)
if(NOT DEFINED FORTIFY_SOURCE_VAL)
    set(FORTIFY_SOURCE_VAL 2)
endif()
list(APPEND COMMON_FLAGS -D_FORTIFY_SOURCE=${FORTIFY_SOURCE_VAL})

# Enable FetchContent module to manage external dependencies (CMake 3.11+)
include(FetchContent)

# Declare cxxopts as an external dependency (header-only library)
FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    # Use this commit in 'master' for latest fixes (e.g., -t=pm bug fix)
    GIT_TAG        11f26227ef8e951bdad48aeaa6537f1c5d8e9e0a
    SOURCE_DIR     ${CMAKE_BINARY_DIR}/_deps/cxxopts-src
)

# Check if cxxopts has already been downloaded and populated
FetchContent_GetProperties(cxxopts)
if(NOT cxxopts_POPULATED)
    FetchContent_Populate(cxxopts)  # Download and configure if not yet done
endif()

# Create an INTERFACE library for cxxopts (header-only, no linking needed)
add_library(cxxopts INTERFACE)
target_include_directories(cxxopts INTERFACE ${cxxopts_SOURCE_DIR}/include)

# Set compiler flags based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND COMMON_FLAGS -Og -ggdb -DDEBUG -UNDEBUG)
else()
    list(APPEND COMMON_FLAGS -O2 -UDEBUG -DNDEBUG)
endif()

# Define linker flags for security and optimization
set(LINK_FLAGS
    -pie
    -Wl,-z,relro,-z,now,-z,noexecstack
)

add_library(PoeGenToolUtility STATIC src/utility.cpp)

target_include_directories(PoeGenToolUtility PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/inc
)

target_compile_options(PoeGenToolUtility PRIVATE ${COMMON_FLAGS})

target_link_libraries(PoeGenToolUtility PUBLIC
    OpenSSL::Crypto
)

set_target_properties(PoeGenToolUtility PROPERTIES
    CXX_STANDARD          20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS        OFF
)

add_executable(${PROJECT_NAME} src/POETool.cpp)

target_compile_options(${PROJECT_NAME} PRIVATE ${COMMON_FLAGS})

target_link_options(${PROJECT_NAME} PRIVATE ${LINK_FLAGS})
target_link_libraries(${PROJECT_NAME} PRIVATE
    PoeGenToolUtility
    cxxopts
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Inject version from build system (defaults to dev build if not specified)
if(NOT DEFINED POE_VERSION)
    set(POE_VERSION "9.9.9.9-dev")
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE POE_VERSION="${POE_VERSION}")

message(STATUS "Building ${PROJECT_NAME} v${POE_VERSION} with C++${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")