#!/bin/bash
set -e

# ============================================================================
# Unified Build Script for Linux/macOS (Bash)
# ============================================================================

# ----------------------------------------------------------------------------
# Color Definitions
# ----------------------------------------------------------------------------
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ----------------------------------------------------------------------------
# Default Configuration
# ----------------------------------------------------------------------------
BUILD_TYPE="Debug"
DO_CLEAN="false"
INCLUDE_SERVER="true"
INCLUDE_GUI="true"
ARCH="x86_64"
TARGET_WINDOWS="false"
TARGET_LINUX="false"
TARGET_MACOS="false"
OUTPUT_DIR=""

# Get project root directory (script is in deploy/ directory)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Set default output directory
if [ -z "$OUTPUT_DIR" ]; then
    OUTPUT_DIR="$PROJECT_ROOT/dist"
fi

BUILD_DIR="$PROJECT_ROOT/server/build"

# ----------------------------------------------------------------------------
# Helper Functions
# ----------------------------------------------------------------------------

# Print colored messages
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_section() {
    echo
    echo "----------------------------------------"
    echo -e "${GREEN}[BUILD]${NC} $1"
    echo "----------------------------------------"
}

# Show help message
show_help() {
    echo
    echo "Unified Build Script for Cross-Platform Project"
    echo
    echo "Usage: $0 [options]"
    echo
    echo "Build Options:"
    echo "  --release          Build in Release mode"
    echo "  --debug            Build in Debug mode (default)"
    echo "  --clean            Clean all build directories before starting"
    echo "  --no-server        Skip building the server component"
    echo "  --no-gui           Skip building the GUI component"
    echo "  --arch ARCH        Specify architecture (x86_64 or arm64). Default: x86_64"
    echo
    echo "Platform Options:"
    echo "  --windows          Build for Windows platform"
    echo "  --linux            Build for Linux platform"
    echo "  --macos            Build for macOS platform"
    echo "  --all              Build for all platforms"
    echo
    echo "Output Options:"
    echo "  --output DIR       Specify output directory (default: ./dist)"
    echo

    echo "Other Options:"
    echo "  -h, --help         Show this help message"
    echo
    echo "Examples:"
    echo "  $0 --release --macos --clean --arch x86_64 --no-gui"
    echo "  $0 --debug --windows --clean"
    echo "  $0 --release --no-gui --linux"
    echo
    exit 0
}

# Set default platform based on current OS
set_default_platform() {
    if [ "$TARGET_WINDOWS" == "false" ] && [ "$TARGET_LINUX" == "false" ] && [ "$TARGET_MACOS" == "false" ]; then
        case "$(uname -s)" in
            Darwin*)
                TARGET_MACOS="true"
                print_info "Auto-detected macOS platform"
                ;;
            Linux*)
                TARGET_LINUX="true"
                print_info "Auto-detected Linux platform"
                ;;
            CYGWIN*|MINGW*|MSYS*)
                TARGET_WINDOWS="true"
                print_info "Auto-detected Windows platform"
                ;;
            *)
                TARGET_LINUX="true"
                print_warning "Unknown platform, defaulting to Linux"
                ;;
        esac
    fi
}

# Create output directories
create_output_dir() {
    mkdir -p "$OUTPUT_DIR"
    print_info "Output directory: $OUTPUT_DIR"
}



# ----------------------------------------------------------------------------
# Argument Parsing
# ----------------------------------------------------------------------------
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --release) BUILD_TYPE="Release" ;;
        --debug) BUILD_TYPE="Debug" ;;
        --clean) DO_CLEAN="true" ;;
        --no-server) INCLUDE_SERVER="false" ;;
        --no-gui) INCLUDE_GUI="false" ;;
        --arch)
            ARCH="$2"
            shift
            ;;
        --windows) TARGET_WINDOWS="true" ;;
        --linux) TARGET_LINUX="true" ;;
        --macos) TARGET_MACOS="true" ;;
        --all)
            TARGET_WINDOWS="true"
            TARGET_LINUX="true"
            TARGET_MACOS="true"
            ;;
        --output)
            OUTPUT_DIR="$2"
            shift
            ;;

        -h|--help) show_help ;;
        *)
            print_error "Unknown parameter: $1"
            show_help
            ;;
    esac
    shift
done

# Validate architecture
if [[ "$ARCH" != "x86_64" && "$ARCH" != "arm64" ]]; then
    print_error "Invalid architecture specified: $ARCH"
    print_info "Supported architectures are: x86_64, arm64"
    exit 1
fi



# Set default platform if none specified
set_default_platform

# Create output directory
create_output_dir

# ----------------------------------------------------------------------------
# Build Functions
# ----------------------------------------------------------------------------

# Build server component
build_server() {
    local platform=$1
    local arch=$2
    local output_name=$3
    
    print_section "Building Server for $platform ($arch)"
    
    local server_build_dir="$BUILD_DIR/$platform-$arch"
    mkdir -p "$server_build_dir"
    cd "$server_build_dir"
    
    print_info "Configuring CMake for server..."
    
    # Select appropriate toolchain file
    local toolchain_file=""
    case "$platform" in
        windows)
            if [[ "$arch" == "arm64" ]]; then
                toolchain_file="$PROJECT_ROOT/server/toolchains/windows-on-linux-arm64.cmake"
            else
                toolchain_file="$PROJECT_ROOT/server/toolchains/windows-on-linux-x86.cmake"
            fi
            ;;
        linux)
            if [[ "$arch" == "arm64" ]]; then
                toolchain_file="$PROJECT_ROOT/server/toolchains/linux-arm64.cmake"
            else
                toolchain_file="$PROJECT_ROOT/server/toolchains/linux-x86.cmake"
            fi
            ;;
        macos)
            if [[ "$arch" == "arm64" ]]; then
                toolchain_file="$PROJECT_ROOT/server/toolchains/macos-arm64.cmake"
            else
                toolchain_file="$PROJECT_ROOT/server/toolchains/macos-x86.cmake"
            fi
            ;;
        *)
            print_error "Unsupported platform: $platform"
            return 1
            ;;
    esac
    
    # Configure with CMake
    if [ -f "$toolchain_file" ]; then
        cmake ../.. -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DCMAKE_TOOLCHAIN_FILE="$toolchain_file" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    else
        cmake ../.. -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    fi
    
    if [ $? -ne 0 ]; then
        print_error "CMake configuration failed"
        return 1
    fi
    
    print_info "Building server with CMake..."
    cmake --build . --config "$BUILD_TYPE"
    
    if [ $? -ne 0 ]; then
        print_error "Server build failed"
        return 1
    fi
    
    print_info "Copying server artifacts..."
    local dest_path="$OUTPUT_DIR/$platform-$arch"
    mkdir -p "$dest_path"
    
    # Find the built binary
    local binary_path=""
    case "$platform" in
        macos)
            # macOS builds an .app bundle which is a directory
            if [ -d "bin/$BUILD_TYPE/$output_name" ]; then
                binary_path="bin/$BUILD_TYPE/$output_name"
            elif [ -d "bin/$output_name" ]; then
                binary_path="bin/$output_name"
            else
                print_error "Server app bundle not found (.app)"
                return 1
            fi
            ;;
        *)
            # Linux build a regular executable file
            if [ -f "bin/$BUILD_TYPE/$output_name" ]; then
                binary_path="bin/$BUILD_TYPE/$output_name"
            elif [ -f "bin/$output_name" ]; then
                binary_path="bin/$output_name"
            else
                print_error "Server executable not found"
                return 1
            fi
            ;;
    esac
    
    cp -r "$binary_path" "$dest_path/$output_name"
    cp "$PROJECT_ROOT/server/server.conf" "$dest_path/"
    # Copy power control scripts for the current platform only
    if [ -d "$PROJECT_ROOT/server/scripts" ]; then
        print_info "Packaging server scripts (platform: $platform) into scripts/ ..."
        case "$platform" in
            macos)
                if [ -d "$PROJECT_ROOT/server/scripts/macos" ]; then
                    mkdir -p "$dest_path/scripts"
                    cp -R "$PROJECT_ROOT/server/scripts/macos/"* "$dest_path/scripts/" || true
                    # Make shell scripts executable (portable across BSD/macOS/Linux)
                    chmod +x "$dest_path/scripts"/*.sh 2>/dev/null || true
                fi
                ;;
            linux)
                if [ -d "$PROJECT_ROOT/server/scripts/linux" ]; then
                    mkdir -p "$dest_path/scripts"
                    cp -R "$PROJECT_ROOT/server/scripts/linux/"* "$dest_path/scripts/" || true
                    # Make shell scripts executable (portable across BSD/macOS/Linux)
                    chmod +x "$dest_path/scripts"/*.sh 2>/dev/null || true
                fi
                ;;
            windows)
                if [ -d "$PROJECT_ROOT/server/scripts/windows" ]; then
                    mkdir -p "$dest_path/scripts"
                    cp -R "$PROJECT_ROOT/server/scripts/windows/"* "$dest_path/scripts/" || true
                fi
                ;;
        esac
    fi
    
    print_success "Server build completed for $platform"
    return 0
}

# Build GUI component
build_gui() {
    local platform=$1
    local arch=$2
    
    print_section "Building GUI for $platform ($arch)"
    
    local gui_dir="$PROJECT_ROOT/server_gui"
    cd "$gui_dir"
    
    print_info "Checking for Flutter..."
    if ! command -v flutter &> /dev/null; then
        print_error "'flutter' command not found. Please ensure Flutter SDK is in your PATH."
        return 1
    fi
    
    print_info "Installing Flutter dependencies..."
    flutter pub get
    if [ $? -ne 0 ]; then
        print_error "Flutter pub get failed"
        return 1
    fi
    
    print_info "Building Flutter for $platform..."
    local flutter_build_mode=$(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')
    local flutter_artifact_subdir=""
    
    case "$platform" in
        windows)
            flutter_artifact_subdir="x64"
            if [ "$arch" == "arm64" ]; then
                flutter_artifact_subdir="arm64"
            fi
            flutter build windows --$flutter_build_mode
            ;;
        linux)
            flutter_artifact_subdir="x64"
            if [ "$arch" == "arm64" ]; then
                flutter_artifact_subdir="arm64"
            fi
            flutter build linux --$flutter_build_mode
            ;;
        macos)
            flutter build macos --$flutter_build_mode
            ;;
        *)
            print_error "Unsupported GUI platform: $platform"
            return 1
            ;;
    esac
    
    if [ $? -ne 0 ]; then
        print_error "Flutter build failed"
        return 1
    fi
    
    print_info "Copying GUI artifacts..."
    local dest_path="$OUTPUT_DIR/$platform-$arch"
    mkdir -p "$dest_path"
    
    # Copy Flutter build artifacts
    case "$platform" in
        windows)
            local subdir="${flutter_artifact_subdir:-x64}"
            local source_path="build/windows/$subdir/runner/$BUILD_TYPE"
            if [ -d "$source_path" ]; then
                cp -r "$source_path"/* "$dest_path/"
            else
                print_error "Flutter build artifacts not found at $source_path"
                return 1
            fi
            ;;
        linux)
            local subdir="${flutter_artifact_subdir:-x64}"
            local source_path="build/linux/$subdir/$flutter_build_mode/bundle"
            if [ -d "$source_path" ]; then
                cp -r "$source_path"/* "$dest_path/"
                # Also stage server config and linux scripts next to the GUI exec
                if [ -f "$PROJECT_ROOT/server/server.conf" ]; then
                    cp -f "$PROJECT_ROOT/server/server.conf" "$dest_path/"
                    print_info "Staged server.conf into GUI dist folder"
                fi
                if [ -d "$PROJECT_ROOT/server/scripts/linux" ]; then
                    mkdir -p "$dest_path/scripts"
                    cp -R "$PROJECT_ROOT/server/scripts/linux/"* "$dest_path/scripts/" || true
                    chmod +x "$dest_path/scripts"/*.sh 2>/dev/null || true
                    print_info "Staged server linux scripts into GUI dist folder"
                fi
            else
                print_error "Flutter build artifacts not found at $source_path"
                return 1
            fi
            ;;
        macos)
            local source_path="build/macos/Build/Products/$BUILD_TYPE"
            if [ -d "$source_path" ]; then
                cp -R "$source_path/MouseHero.app" "$dest_path/"
                
                # Embed server binary and config into the .app
                local app_path="$dest_path/MouseHero.app"
                local app_macos_dir="$app_path/Contents/MacOS"
                local app_resources_dir="$app_path/Contents/Resources"
                mkdir -p "$app_macos_dir" "$app_resources_dir"
                
                # Ensure the destination is clean before moving
                rm -rf "$app_resources_dir/server.app"
                # Move server.app bundle into the main app's Resources directory
                mv "$dest_path/server.app" "$app_resources_dir/"
                print_info "Embedded server.app into app: Contents/Resources/server.app"
                
                # Move server.conf if present in dest_path (use as template resource)
                mv -f "$dest_path/server.conf" "$app_resources_dir/server.conf"
                print_info "Embedded server.conf template into app: Contents/Resources/server.conf"
                
                # Move server scripts into app Resources if present
                mkdir -p "$app_resources_dir/scripts"
                cp -R "$dest_path/scripts/"* "$app_resources_dir/scripts/" || true
                # ensure executable bits for shell scripts
                chmod +x "$app_resources_dir/scripts"/*.sh 2>/dev/null || true
                # cleanup source scripts directory in dest_path
                rm -rf "$dest_path/scripts"
                print_info "Embedded server scripts into app: Contents/Resources/scripts"

                local sign_identity="${SIGN_IDENTITY:-${SIGN_ID:-}}"
                if [ -z "$sign_identity" ]; then
                    # Try to auto-detect an identity (prefer Developer ID Application), extract only SHA-1 hash
                    sign_identity=$(security find-identity -p codesigning -v 2>/dev/null \
                        | awk '/Developer ID Application/{print}' \
                        | sed -n 's/^[[:space:]]*[0-9]*)[[:space:]]\([A-F0-9]\{40\}\).*/\1/p' \
                        | head -n1)
                    if [ -z "$sign_identity" ]; then
                        sign_identity=$(security find-identity -p codesigning -v 2>/dev/null \
                            | awk '/Apple Distribution/{print}' \
                            | sed -n 's/^[[:space:]]*[0-9]*)[[:space:]]\([A-F0-9]\{40\}\).*/\1/p' \
                            | head -n1)
                    fi
                    if [ -z "$sign_identity" ]; then
                        sign_identity=$(security find-identity -p codesigning -v 2>/dev/null \
                            | awk '/Apple Development/{print}' \
                            | sed -n 's/^[[:space:]]*[0-9]*)[[:space:]]\([A-F0-9]\{40\}\).*/\1/p' \
                            | head -n1)
                    fi
                fi
                # Trim whitespace just in case
                sign_identity=$(echo -n "$sign_identity" | tr -d '[:space:]')
                local sign_preview=""
                if [ -n "$sign_identity" ]; then
                    sign_preview="${sign_identity:0:8}…"
                    local release_entitlements="$PROJECT_ROOT/server_gui/macos/Runner/Release.entitlements"
                    if [ ! -f "$release_entitlements" ]; then
                        print_warning "Release entitlements not found at $release_entitlements; proceeding without custom entitlements"
                        release_entitlements=""
                    fi
                    local server_executable_path="$app_resources_dir/server.app"
                    # Sign embedded server executable first
                    print_info "Codesigning embedded server with identity (SHA-1): $sign_preview"
                    codesign --force --options runtime --timestamp --sign "$sign_identity" "$server_executable_path" || print_warning "codesign server failed"

                    # Sign embedded frameworks and dylibs with Developer ID cert
                    local frameworks_dir="$app_path/Contents/Frameworks"
                    if [ -d "$frameworks_dir" ]; then
                        print_info "Codesigning frameworks under $frameworks_dir"
                        while IFS= read -r framework; do
                            print_info "  ↳ $(basename "$framework")"
                            codesign --force --options runtime --timestamp --sign "$sign_identity" "$framework" || print_warning "codesign framework failed: $framework"
                        done < <(find "$frameworks_dir" -mindepth 1 -maxdepth 1 -type d -name "*.framework")
                        while IFS= read -r dylib; do
                            print_info "  ↳ $(basename "$dylib")"
                            codesign --force --options runtime --timestamp --sign "$sign_identity" "$dylib" || print_warning "codesign dylib failed: $dylib"
                        done < <(find "$frameworks_dir" -type f \( -name "*.dylib" -o -name "*.so" \))
                    fi

                    # Sign login helper apps if present
                    local login_items_dir="$app_path/Contents/Library/LoginItems"
                    if [ -d "$login_items_dir" ]; then
                        print_info "Codesigning login helper apps"
                        while IFS= read -r helper_app; do
                            print_info "  ↳ $(basename "$helper_app")"
                            if [ -n "$release_entitlements" ]; then
                                codesign --force --options runtime --timestamp --entitlements "$release_entitlements" --sign "$sign_identity" "$helper_app" || print_warning "codesign login helper failed: $helper_app"
                            else
                                codesign --force --options runtime --timestamp --sign "$sign_identity" "$helper_app" || print_warning "codesign login helper failed: $helper_app"
                            fi
                        done < <(find "$login_items_dir" -mindepth 1 -maxdepth 1 -type d -name "*.app")
                    fi

                    # Then sign the .app bundle
                    print_info "Codesigning app with identity (SHA-1): $sign_preview"
                    if [ -n "$release_entitlements" ]; then
                        codesign --force --options runtime --timestamp --entitlements "$release_entitlements" --sign "$sign_identity" "$app_path" || print_warning "codesign app failed"
                    else
                        codesign --force --options runtime --timestamp --sign "$sign_identity" "$app_path" || print_warning "codesign app failed"
                    fi
                else
                    print_warning "No signing identity found; app will remain unsigned."
                fi

                if [ "$BUILD_TYPE" == "Release" ]; then
                    local dmg_name="server_gui-${ARCH}-${BUILD_TYPE}.dmg"
                    local dmg_path="$dest_path/$dmg_name"
                    print_info "Creating DMG: $dmg_name"
                    # Prepare staging directory to include Applications symlink
                    local dmg_staging
                    dmg_staging="$(mktemp -d)"
                    mkdir -p "$dmg_staging"
                    cp -R "$app_path" "$dmg_staging/"
                    ln -s /Applications "$dmg_staging/Applications"
                    # Create compressed read-only DMG with the app and Applications alias
                    hdiutil create -volname "server_gui" -srcfolder "$dmg_staging" -ov -format UDZO "$dmg_path" >/dev/null 2>&1 || {
                        print_warning "Failed to create DMG via hdiutil; continuing with .app only"
                    }
                    # Cleanup staging
                    rm -rf "$dmg_staging"
                    if [ -f "$dmg_path" ]; then
                        if [ -n "$sign_identity" ]; then
                            print_info "Codesigning DMG with identity (SHA-1): $sign_preview"
                            codesign --force --timestamp --sign "$sign_identity" "$dmg_path" || print_warning "codesign dmg failed"
                        else
                            print_warning "No signing identity available; DMG will remain unsigned."
                        fi
                    fi
                fi
            else
                print_error "Flutter build artifacts not found at $source_path"
                return 1
            fi
            ;;
    esac
    
    print_success "GUI build completed for $platform"
    return 0
}

# ----------------------------------------------------------------------------
# Main Build Logic
# ----------------------------------------------------------------------------

print_info "Starting unified build..."
print_info "Build Type: $BUILD_TYPE"
print_info "Architecture: $ARCH"

# Clean build if requested
if [ "$DO_CLEAN" == "true" ]; then
    print_info "Cleaning previous build artifacts..."
    if [ -d "$OUTPUT_DIR" ]; then
        print_info "Removing $OUTPUT_DIR"
        rm -rf "$OUTPUT_DIR"
    fi
    if [ -d "$BUILD_DIR" ]; then
        print_info "Removing $BUILD_DIR"
        rm -rf "$BUILD_DIR"
    fi
    if [ "$INCLUDE_GUI" == "true" ] && [ -d "$PROJECT_ROOT/server_gui/build" ]; then
        print_info "Cleaning Flutter build directory"
        cd "$PROJECT_ROOT/server_gui"
        flutter clean
    fi
fi

# Build for each target platform
build_failed=false

if [ "$TARGET_WINDOWS" == "true" ]; then
    if [ "$INCLUDE_SERVER" == "true" ]; then
        build_server "windows" "$ARCH" "server.exe"
        if [ $? -ne 0 ]; then
            build_failed=true
        fi
    fi
    
    if [ "$INCLUDE_GUI" == "true" ] && [ "$build_failed" == "false" ]; then
        build_gui "windows" "$ARCH"
        if [ $? -ne 0 ]; then
            build_failed=true
        fi
    fi
fi

if [ "$TARGET_LINUX" == "true" ] && [ "$build_failed" == "false" ]; then
    if [ "$INCLUDE_SERVER" == "true" ]; then
        build_server "linux" "$ARCH" "server"
        if [ $? -ne 0 ]; then
            build_failed=true
        fi
    fi
    
    if [ "$INCLUDE_GUI" == "true" ] && [ "$build_failed" == "false" ]; then
        build_gui "linux" "$ARCH"
        if [ $? -ne 0 ]; then
            build_failed=true
        fi
        # Auto-build AppImage when in Release mode
        if [ "$build_failed" == "false" ] && [ "$BUILD_TYPE" == "Release" ]; then
            print_section "Packaging GUI as AppImage (Linux)"
            APPIMAGE_SCRIPT="$PROJECT_ROOT/scripts/build_appimage.sh"
            if [ -f "$APPIMAGE_SCRIPT" ]; then
                # Export vars so sub-script names and paths align
                export ARCH BUILD_TYPE
                bash "$APPIMAGE_SCRIPT" || {
                    print_warning "AppImage packaging failed; continuing without AppImage."
                }
            else
                print_warning "AppImage script not found: $APPIMAGE_SCRIPT"
            fi
        fi
    fi
fi

if [ "$TARGET_MACOS" == "true" ] && [ "$build_failed" == "false" ]; then
    if [ "$INCLUDE_SERVER" == "true" ]; then
        build_server "macos" "$ARCH" "server.app"
        if [ $? -ne 0 ]; then
            build_failed=true
        fi
    fi
    
    if [ "$INCLUDE_GUI" == "true" ] && [ "$build_failed" == "false" ]; then
        build_gui "macos" "$ARCH"
        if [ $? -ne 0 ]; then
            build_failed=true
        fi
    fi
fi

# Final result
if [ "$build_failed" == "true" ]; then
    print_error "Build failed. Please check the error messages above."
    exit 1
else
    echo
    print_success "Build finished successfully!"
    print_info "Output directory: $OUTPUT_DIR"
    echo
fi
