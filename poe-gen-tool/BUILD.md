Intel(R) Software Guard Extensions Data Center Attestation Primitives (Intel(R) SGX DCAP): Platform Ownership Endorsement Tool
===============================================


## Prerequisites
- CMake 3.18 or later
- C++20 compatible compiler (GCC 10+ or Clang 10+)
- OpenSSL development headers (`libssl-dev` on Debian/Ubuntu, `openssl-devel` on RHEL/Fedora)

The following packages must be installed on the build host before building:

- **OpenSSL** (libssl-dev / openssl-devel) — used for X.509 certificate parsing.
  - Ubuntu/Debian: `sudo apt-get install libssl-dev`
  - RHEL/Fedora:   `sudo dnf install openssl-devel`

All other dependencies (cxxopts, GoogleTest) are fetched automatically by CMake
via FetchContent when not already present on the system.

## How to build this tool
For Linux version:
- Just run the command: make 
- For debug version, you can run command: make DEBUG=1

If you use cmake:
- please run the command: 
     mkdir -p build && cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
- For debug version, you can run command:
     mkdir -p build && cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug .. && cmake --build .
