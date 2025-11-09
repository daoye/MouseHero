#!/usr/bin/env bash
set -euo pipefail

# Build Server GUI as an AppImage on Linux.
# Source: dist/linux-$ARCH (must exist; produced by scripts/build_unified.sh)
# Output: dist/server_gui-$ARCH-$BUILD_TYPE.AppImage

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
GUI_ROOT="$PROJECT_ROOT/server_gui"
DIST_DIR="$PROJECT_ROOT/dist"
ARCH_ENV="${ARCH:-}"
if [[ -z "$ARCH_ENV" ]]; then
  case "$(uname -m)" in
    x86_64|amd64) ARCH_ENV="x86_64" ;;
    aarch64|arm64) ARCH_ENV="arm64" ;;
    *) ARCH_ENV="x86_64" ;;
  esac
fi
UNIFIED_SOURCE_DIR="$DIST_DIR/linux-$ARCH_ENV"
APPDIR_ROOT="$GUI_ROOT/build/AppDir"
APP_NAME="Mouse Hero"
APP_ID="com.aprilzz.mousehero.server_gui"
MODE_ENV="${BUILD_TYPE:-Release}"
APPIMAGE_NAME="server_gui-${ARCH_ENV}-${MODE_ENV}.AppImage"
OUT_DIR="$DIST_DIR"

mkdir -p "$OUT_DIR"
if [[ ! -d "$UNIFIED_SOURCE_DIR" ]]; then
  echo "[ERROR] Unified output not found: $UNIFIED_SOURCE_DIR" >&2
  echo "        Run scripts/build_unified.sh --release --linux first." >&2
  exit 1
fi
SRC="$UNIFIED_SOURCE_DIR"
echo "[INFO] Packaging from: $SRC (ARCH=$ARCH_ENV, MODE=$MODE_ENV)"

# 2) Prepare AppDir structure (clean everything first)
rm -rf "$APPDIR_ROOT"
mkdir -p "$APPDIR_ROOT/usr/bin" "$APPDIR_ROOT/usr/share/applications" "$APPDIR_ROOT/usr/share/icons/hicolor/256x256/apps"

# Copy the entire GUI bundle into usr/bin (exec + lib + data next to each other)
cp -a "$SRC/." "$APPDIR_ROOT/usr/bin/"

echo "[INFO] AppDir structure prepared at: $APPDIR_ROOT"

# 3) Custom AppRun wrapper - ensure cwd is usr/bin so Flutter finds data/
cat > "$APPDIR_ROOT/AppRun.wrapped" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
cd "$HERE/usr/bin"
# Prefer portals on Wayland for file dialogs, etc.
: "${GTK_USE_PORTAL:=1}"; export GTK_USE_PORTAL
exec ./server_gui "$@"
EOF
chmod +x "$APPDIR_ROOT/AppRun.wrapped"

# 4) Desktop file (in usr/share/applications for linuxdeploy)
cat > "$APPDIR_ROOT/usr/share/applications/$APP_ID.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=$APP_NAME
Comment=Mouse Hero Server
Exec=server_gui
Icon=server_gui
Categories=Utility;
StartupWMClass=server_gui
EOF

# 5) Icon (resize to 256x256 for linuxdeploy compatibility)
ICON_PATH="$SRC/data/flutter_assets/assets/app_icon.png"
ICON_DIR="$APPDIR_ROOT/usr/share/icons/hicolor/256x256/apps"
ICON_OUTPUT="$ICON_DIR/server_gui.png"

if [[ ! -f "$ICON_PATH" ]]; then
  echo "[ERROR] Icon not found: $ICON_PATH" >&2
  exit 1
fi

# Check if ImageMagick is available to resize the icon
if command -v convert >/dev/null 2>&1; then
  echo "[INFO] Resizing icon from $(identify -format "%wx%h" "$ICON_PATH" 2>/dev/null || echo "unknown") to 256x256..."
  convert "$ICON_PATH" -resize 256x256! "$ICON_OUTPUT"
  
  # Verify the resize worked
  if command -v identify >/dev/null 2>&1; then
    ACTUAL_SIZE=$(identify -format "%wx%h" "$ICON_OUTPUT" 2>/dev/null || echo "unknown")
    echo "[INFO] Icon resized to: $ACTUAL_SIZE"
    if [[ "$ACTUAL_SIZE" != "256x256" ]]; then
      echo "[ERROR] Icon resize failed, got $ACTUAL_SIZE instead of 256x256" >&2
      exit 1
    fi
  fi
else
  echo "[ERROR] ImageMagick (convert command) is required but not found" >&2
  echo "        Please install: sudo pacman -S imagemagick" >&2
  exit 1
fi

# 6) Download appimagetool
APPIMAGETOOL="$GUI_ROOT/build/appimagetool-x86_64.AppImage"
if [[ ! -x "$APPIMAGETOOL" ]]; then
  echo "[INFO] Downloading appimagetool..."
  curl -L -o "$APPIMAGETOOL" https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
  chmod +x "$APPIMAGETOOL"
fi

# 7) Bundle dependencies using ldd and manual collection
echo "[INFO] Bundling dependencies..."
mkdir -p "$APPDIR_ROOT/usr/lib"

# Determine system library directory
SYS_LIB_DIR="/usr/lib/x86_64-linux-gnu"
if [[ "$ARCH_ENV" == "arm64" ]]; then
  SYS_LIB_DIR="/usr/lib/aarch64-linux-gnu"
fi

# Function to copy a library and its dependencies
copy_lib_recursive() {
  local lib="$1"
  local lib_basename=$(basename "$lib")
  
  # Skip if already copied
  if [[ -f "$APPDIR_ROOT/usr/lib/$lib_basename" ]]; then
    return
  fi
  
  # Copy the library
  if [[ -f "$lib" ]]; then
    cp -L "$lib" "$APPDIR_ROOT/usr/lib/"
    echo "[INFO] Bundled: $lib_basename"
  fi
}

# Get all dependencies of server_gui
echo "[INFO] Analyzing dependencies of server_gui..."
LDD_OUTPUT=$(ldd "$APPDIR_ROOT/usr/bin/server_gui" 2>/dev/null || true)

# Copy all non-system libraries
while IFS= read -r line; do
  if [[ "$line" =~ =\>.*\(0x ]]; then
    lib_path=$(echo "$line" | awk '{print $3}')
    if [[ -n "$lib_path" ]] && [[ "$lib_path" != "not" ]]; then
      # Skip system libraries that should be on every Linux system
      if [[ ! "$lib_path" =~ ^/lib/|^/lib64/ ]] && \
         [[ ! "$lib_path" =~ libc\.so|libm\.so|libdl\.so|libpthread\.so|librt\.so ]]; then
        copy_lib_recursive "$lib_path"
      fi
    fi
  fi
done <<< "$LDD_OUTPUT"

# Copy GTK and GDK resources
echo "[INFO] Copying GTK resources..."
if [[ -d "$SYS_LIB_DIR/gtk-3.0" ]]; then
  mkdir -p "$APPDIR_ROOT/usr/lib/gtk-3.0"
  cp -r "$SYS_LIB_DIR/gtk-3.0"/* "$APPDIR_ROOT/usr/lib/gtk-3.0/" 2>/dev/null || true
fi

if [[ -d "$SYS_LIB_DIR/gdk-pixbuf-2.0" ]]; then
  mkdir -p "$APPDIR_ROOT/usr/lib/gdk-pixbuf-2.0"
  cp -r "$SYS_LIB_DIR/gdk-pixbuf-2.0"/* "$APPDIR_ROOT/usr/lib/gdk-pixbuf-2.0/" 2>/dev/null || true
fi

# Copy GTK data files
if [[ -d "/usr/share/gtk-3.0" ]]; then
  mkdir -p "$APPDIR_ROOT/usr/share/gtk-3.0"
  cp -r /usr/share/gtk-3.0/* "$APPDIR_ROOT/usr/share/gtk-3.0/" 2>/dev/null || true
fi

if [[ -d "/usr/share/glib-2.0" ]]; then
  mkdir -p "$APPDIR_ROOT/usr/share/glib-2.0"
  cp -r /usr/share/glib-2.0/* "$APPDIR_ROOT/usr/share/glib-2.0/" 2>/dev/null || true
fi

# Copy icon themes (Adwaita)
echo "[INFO] Copying icon themes..."
if [[ -d "/usr/share/icons/Adwaita" ]]; then
  mkdir -p "$APPDIR_ROOT/usr/share/icons"
  cp -r /usr/share/icons/Adwaita "$APPDIR_ROOT/usr/share/icons/" 2>/dev/null || true
  echo "[INFO] Copied Adwaita icon theme"
fi

if [[ -d "/usr/share/icons/hicolor" ]]; then
  cp -r /usr/share/icons/hicolor "$APPDIR_ROOT/usr/share/icons/" 2>/dev/null || true
  echo "[INFO] Copied hicolor icon theme"
fi

# 8) Create AppRun script
cat > "$APPDIR_ROOT/AppRun" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"

# Set up library path
export LD_LIBRARY_PATH="$HERE/usr/lib:${LD_LIBRARY_PATH:-}"

# Set up GTK environment
export GTK_THEME=Adwaita
export GTK_DATA_PREFIX="$HERE/usr"
export XDG_DATA_DIRS="$HERE/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"
export GSETTINGS_SCHEMA_DIR="$HERE/usr/share/glib-2.0/schemas"
export GTK_PATH="$HERE/usr/lib/gtk-3.0"
export GDK_PIXBUF_MODULEDIR="$HERE/usr/lib/gdk-pixbuf-2.0/2.10.0/loaders"
export GDK_PIXBUF_MODULE_FILE="$HERE/usr/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"

# Fix icon theme - use system icons as fallback
export XCURSOR_PATH="$HERE/usr/share/icons:/usr/share/icons:${XCURSOR_PATH:-}"
export GTK_ICON_THEME="Adwaita"

# Auto-detect display server and configure accordingly
if [ -n "${WAYLAND_DISPLAY:-}" ]; then
  echo "[AppImage] Wayland detected, using native Wayland backend"
  # Use Wayland backend for GTK
  export GDK_BACKEND=wayland
  # Disable OpenGL rendering on Wayland (use Cairo software rendering)
  export GSK_RENDERER=cairo
  # Enable Wayland-specific features
  export MOZ_ENABLE_WAYLAND=1
  export QT_QPA_PLATFORM=wayland
else
  echo "[AppImage] X11 detected, using X11 backend"
  export GDK_BACKEND=x11
  export GSK_RENDERER=cairo
  export QT_QPA_PLATFORM=xcb
fi

# Disable HiDPI scaling (let Flutter handle it)
export GDK_SCALE=1
export GDK_DPI_SCALE=1
export QT_AUTO_SCREEN_SCALE_FACTOR=0
export QT_SCALE_FACTOR=1

# Change to usr/bin for Flutter
cd "$HERE/usr/bin"
: "${GTK_USE_PORTAL:=1}"; export GTK_USE_PORTAL

# Debug output
echo "[AppImage] GDK_BACKEND=$GDK_BACKEND"
echo "[AppImage] GSK_RENDERER=$GSK_RENDERER"
echo "[AppImage] GTK_THEME=$GTK_THEME"
echo "[AppImage] WAYLAND_DISPLAY=${WAYLAND_DISPLAY:-<not set>}"

exec ./server_gui "$@"
EOF
chmod +x "$APPDIR_ROOT/AppRun"

# 9) Create desktop file at AppDir root (required by AppImage spec)
cp "$APPDIR_ROOT/usr/share/applications/$APP_ID.desktop" "$APPDIR_ROOT/$APP_ID.desktop"

# 10) Create icon at AppDir root
cp "$ICON_OUTPUT" "$APPDIR_ROOT/server_gui.png"
ln -sf server_gui.png "$APPDIR_ROOT/.DirIcon"

# 11) Build AppImage using appimagetool
echo "[INFO] Building AppImage with appimagetool..."
export ARCH="$ARCH_ENV"
"$APPIMAGETOOL" "$APPDIR_ROOT" "$OUT_DIR/$APPIMAGE_NAME"

if [[ -f "$OUT_DIR/$APPIMAGE_NAME" ]]; then
  echo "[OK] AppImage created: $OUT_DIR/$APPIMAGE_NAME"
else
  echo "[ERROR] AppImage creation failed" >&2
  exit 1
fi
