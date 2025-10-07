# System information
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR ARM64)

set(MSYS2_ROOT "$ENV{MSYS2_ROOT}")
set(MSYS2_CLANGARM64_ROOT "${MSYS2_ROOT}/clangarm64")

# Verify CLANGARM64 exists
if(NOT EXISTS "${MSYS2_CLANGARM64_ROOT}")
  message(FATAL_ERROR "CLANGARM64 directory not found at ${MSYS2_CLANGARM64_ROOT}. Please ensure MSYS2 CLANGARM64 toolchain is installed.")
endif()

# Specify the compilers for ARM64 cross-compilation
set(CMAKE_C_COMPILER "${MSYS2_CLANGARM64_ROOT}/bin/clang.exe")
set(CMAKE_CXX_COMPILER "${MSYS2_CLANGARM64_ROOT}/bin/clang++.exe")
set(CMAKE_MAKE_PROGRAM "${MSYS2_CLANGARM64_ROOT}/bin/ninja.exe" CACHE FILEPATH "Ninja build tool")
set(CMAKE_RC_COMPILER "${MSYS2_CLANGARM64_ROOT}/bin/windres.exe")

# Where to look for the target environment
set(CMAKE_FIND_ROOT_PATH "${MSYS2_CLANGARM64_ROOT}")

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# For libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Rust target for Windows on ARM64
# https://corrosion-rs.github.io/corrosion/usage.html#corrosion-options
# https://rust-lang.github.io/rustup/cross-compilation.html
set(Rust_CARGO_TARGET aarch64-pc-windows-gnu)

# For libevent disable ipv6 (if needed for ARM as well)
add_definitions(-D_WIN32_WINNT=0x0A00) # Setting for Windows 10
