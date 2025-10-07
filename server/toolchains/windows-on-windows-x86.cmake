# System information
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(MSYS2_ROOT "$ENV{MSYS2_ROOT}")
set(MSYS2_UCRT64_ROOT "${MSYS2_ROOT}/ucrt64")

# Verify UCRT64 exists
if(NOT EXISTS "${MSYS2_UCRT64_ROOT}")
  message(FATAL_ERROR "UCRT64 directory not found at ${MSYS2_UCRT64_ROOT}. Please ensure MSYS2 UCRT64 toolchain is installed.")
endif()

# Specify the compilers
set(CMAKE_C_COMPILER "${MSYS2_UCRT64_ROOT}/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "${MSYS2_UCRT64_ROOT}/bin/g++.exe")
set(CMAKE_MAKE_PROGRAM "${MSYS2_UCRT64_ROOT}/bin/ninja.exe" CACHE FILEPATH "Ninja build tool")
set(CMAKE_RC_COMPILER "${MSYS2_UCRT64_ROOT}/bin/windres.exe")

# Where to look for the target environment
set(CMAKE_FIND_ROOT_PATH "${MSYS2_UCRT64_ROOT}")

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Rust help:
# https://corrosion-rs.github.io/corrosion/usage.html#corrosion-options
# https://rust-lang.github.io/rustup/cross-compilation.html
set(Rust_CARGO_TARGET x86_64-pc-windows-gnu)


# for libevent disable ipv6
add_definitions(-D_WIN32_WINNT=0x0601)


