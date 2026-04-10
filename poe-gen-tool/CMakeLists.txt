# Minimum required CMake version (3.13+ for FetchContent and modern features)
cmake_minimum_required(VERSION 3.13)

# Define project name and languages
project(poe-gen-tool LANGUAGES CXX)

 # Find OpenSSL for cryptographic functionality
 find_package(OpenSSL REQUIRED)

# Set C++ standard to C++20 (required for modern features)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Disable compiler-specific extensions

# Define common compiler flags for all build types
set(COMMON_FLAGS
    -ffunction-sections
    -fdata-sections
    -Wall -Wextra -Winit-self -Wpointer-arith -Wreturn-type
    -Waddress -Wsequence-point -Wformat-security
    -Wmissing-include-dirs -Wfloat-equal -Wundef -Wshadow
    -Wcast-align -Wconversion -Wredundant-decls
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
    #BINARY_DIR     ${CMAKE_BINARY_DIR}/_deps/cxxopts-build
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
    -Wl,-z,relro,-z,now,-z,noexecstack
)

# Define source files for the executable
set(APP_CPP_FILES
    src/POETool.cpp
    src/utility.cpp
)

# Create the main executable target
add_executable(${PROJECT_NAME} ${APP_CPP_FILES})

# Add private include directories for the executable
target_include_directories(${PROJECT_NAME} PRIVATE
    "src/inc"
    # Note: cxxopts is already provided via INTERFACE library, no need to add manually
)

# Apply common compiler flags to the executable
target_compile_options(${PROJECT_NAME} PRIVATE ${COMMON_FLAGS})

# Apply linker flags and link required libraries
target_link_options(${PROJECT_NAME} PRIVATE ${LINK_FLAGS})
target_link_libraries(${PROJECT_NAME} PRIVATE 
    cxxopts
    OpenSSL::Crypto)  # Link OpenSSL crypto via CMake package, cxxopts

# Optional: Set output directory for the executable
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Inject version from build system (defaults to dev build if not specified)
if(NOT DEFINED POE_VERSION)
    set(POE_VERSION "9.9.9.9-dev")
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE POE_VERSION="${POE_VERSION}")

# Optional: Print build information for debugging
message(STATUS "Building ${PROJECT_NAME} v${POE_VERSION} with C++${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")