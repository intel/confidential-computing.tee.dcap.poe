# Minimum required CMake version (3.18+ for FetchContent and modern features)
cmake_minimum_required(VERSION 3.18)

# Define project name and languages
project(poe-gen-tool LANGUAGES CXX)

option(BUILD_TESTS "Build poe-gen-tool unit tests" ON)

include(ExternalProject)
include(ProcessorCount)

find_program(MAKE_EXECUTABLE NAMES make gmake REQUIRED)

ProcessorCount(OPENSSL_BUILD_JOBS)
if(NOT OPENSSL_BUILD_JOBS OR OPENSSL_BUILD_JOBS EQUAL 0)
    set(OPENSSL_BUILD_JOBS 4)
endif()

set(OPENSSL_VERSION     "3.5.6")
set(OPENSSL_URL_HASH    "SHA256=deae7c80cba99c4b4f940ecadb3c3338b13cb77418409238e57d7f31f2a3b736")
set(OPENSSL_INSTALL_DIR "${CMAKE_BINARY_DIR}/openssl-install")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    set(OPENSSL_PLATFORM "linux-x86_64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(OPENSSL_PLATFORM "linux-aarch64")
else()
    set(OPENSSL_PLATFORM "linux-generic64")
endif()

ExternalProject_Add(openssl_ext
    URL             "https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz"
    URL_HASH        ${OPENSSL_URL_HASH}
    CONFIGURE_COMMAND <SOURCE_DIR>/Configure
        ${OPENSSL_PLATFORM}
        no-shared
        no-tests
        --prefix=${OPENSSL_INSTALL_DIR}
        --libdir=lib
    BUILD_COMMAND    ${MAKE_EXECUTABLE} -j${OPENSSL_BUILD_JOBS} build_sw
    INSTALL_COMMAND  ${MAKE_EXECUTABLE} install_sw
    LOG_DOWNLOAD     TRUE
    LOG_CONFIGURE    TRUE
    LOG_BUILD        TRUE
    LOG_INSTALL      TRUE
)

file(MAKE_DIRECTORY "${OPENSSL_INSTALL_DIR}/include")

add_library(OpenSSL::Crypto STATIC IMPORTED GLOBAL)
set_target_properties(OpenSSL::Crypto PROPERTIES
    IMPORTED_LOCATION             "${OPENSSL_INSTALL_DIR}/lib/libcrypto.a"
    INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INSTALL_DIR}/include"
)
add_dependencies(OpenSSL::Crypto openssl_ext)

find_package(Threads REQUIRED)

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
    ${CMAKE_DL_LIBS}
    Threads::Threads
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
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

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
message(STATUS "Building ${PROJECT_NAME} v${POE_VERSION} with C++${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")