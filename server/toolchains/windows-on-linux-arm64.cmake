set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER aarch64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER aarch64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/aarch64-w64-mingw32)

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Rust help:
# https://corrosion-rs.github.io/corrosion/usage.html#corrosion-options
# https://rust-lang.github.io/rustup/cross-compilation.html
set(Rust_CARGO_TARGET aarch64-pc-windows-gnu)


# 目标 Windows 版本：Win7+
add_definitions(-D_WIN32_WINNT=0x0601)
