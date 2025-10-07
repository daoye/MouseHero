# MouseHero - Turn your phone into a mouse

[![Build](https://github.com/daoye/MouseHero/actions/workflows/build.yml/badge.svg)](https://github.com/daoye/MouseHero/actions/workflows/build.yml)

## Overview

MouseHero is a cross-platform mouse and keyboard simulation suite that "Turn your phone into a mouse". 

Official site: https://mousehero.aprilzz.com

## Project
- **Server (`server/`)** A standalone core service implemented in C. It supports mouse, keyboard, and clipboard control.
- **Server GUI (`server_gui/`)** A Flutter desktop front-end. It surfaces status, configuration editing, and log viewing to simplify user operations, but the server can run without it.
- **Mobile App** A proprietary client distributed separately. Its source code is not part of this repository—please download the latest release from the official website: <https://mousehero.aprilzz.com>.

## Cross-Platform
Support for Windows, Linux (in active development), and macOS on both `x86_64` and `arm64` architectures.
For Mobile, Support iOS and Android.

## Build

Clone the repository:
```bash
git clone https://github.com/daoye/MouseHero.git
cd MouseHero
```

### Windows
- Prerequisites: Rust (`rustup`), Flutter SDK, and **MSYS2** (UCRT environment).
- **Rust targets**
  ```powershell
  rustup target add x86_64-pc-windows-gnu
  # Optional: add ARM64 when you have the toolchain installed
  # rustup target add aarch64-pc-windows-gnu
  ```
- **MSYS2 packages** (run inside an MSYS2 shell)
  ```bash
  pacman -Sy --noconfirm
  pacman -S --needed \
    mingw-w64-ucrt-x86_64-toolchain \
    mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-ninja
  # Optional (Windows ARM64 builds) – requires clangarm64 environment
  # pacman -S --needed mingw-w64-clang-aarch64-toolchain mingw-w64-clang-aarch64-cmake mingw-w64-clang-aarch64-ninja
  ```
- **Build**
  ```powershell
  scripts\build_windows.bat --release --clean [--arch arm64]
  ```

### macOS
- **Prerequisites**
  - Xcode Command Line Tools (`xcode-select --install`).
  - Flutter SDK (channel `stable`).
  - Rust via rustup.
  - Homebrew packages: `cmake` and `ninja`.
  ```bash
  brew install cmake ninja
  ```
- **Rust targets**
  ```bash
  rustup target add x86_64-apple-darwin
  rustup target add aarch64-apple-darwin   # for Apple Silicon builds
  ```
- **Build**
  ```bash
  ./scripts/build_unified.sh --release --macos --clean [--arch arm64]
  ```

### Linux
- **Prerequisites**
  - Flutter SDK (channel `stable`).
  - Rust via rustup.
  - System packages(Debian/Ubuntu example):
  ```bash
  sudo apt-get update
  sudo apt-get install -y \
    cmake ninja-build clang pkg-config \
    libgtk-3-dev libxdo-dev libxkbcommon-dev \
    libwayland-dev libayatana-appindicator3-dev libfuse2
  ```
  For other distributions, install the equivalent development packages (on Arch Linux, see `pacman -S gtk3 xdotool libxkbcommon wayland libayatana-appindicator cmake ninja clang` etc.).
- **Rust targets**
  ```bash
  rustup target add x86_64-unknown-linux-gnu
  # Optional: rustup target add aarch64-unknown-linux-gnu
  ```
- **Build**
  ```bash
  ./scripts/build_unified.sh --release --linux --clean [--arch arm64]
  ```

## License
- **GNU Affero General Public License v3.0** (`LICENSE.txt`)
- Using the software means you agree to the GNU AGPL v3 terms.
## Copyright
-  Copyright © 2025 **MouseHero** All rights reserved.
