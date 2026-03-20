Intel(R) Software Guard Extensions Data Center Attestation Primitives (Intel(R) SGX DCAP): Platform Ownership Endorsement Tool
===============================================


## Prerequisites

## How to build this tool
For Linux version:
- Just run the command: make 
- For debug version, you can run command: make DEBUG=1

If you use cmake:
- please run the command: 
     mkdir -p build && cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
- For debug version, you can run command:
     mkdir -p build && cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug .. && cmake --build .