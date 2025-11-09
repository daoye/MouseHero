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

# 2) Prepare AppDir structure
rm -rf "$APPDIR_ROOT"
mkdir -p "$APPDIR_ROOT/usr/bin" "$APPDIR_ROOT/usr/lib"

# Helper to copy shared libraries into the AppDir runtime lib folder.
copy_shared_lib() {
  local src="$1"
  if [[ -f "$src" ]]; then
    echo "[INFO] Bundling shared library: $src"
    cp -L "$src" "$APPDIR_ROOT/usr/lib/"
  else
    echo "[WARNING] Shared library not found (skipping): $src" >&2
  fi
}

# Resolve system library directory based on architecture
SYS_LIB_DIR="/usr/lib"
case "$ARCH_ENV" in
  x86_64|amd64)
    SYS_LIB_DIR="/usr/lib/x86_64-linux-gnu"
    ;;
  arm64|aarch64)
    SYS_LIB_DIR="/usr/lib/aarch64-linux-gnu"
    ;;
esac

# Copy the entire GUI bundle into usr/bin (exec + lib + data next to each other)
cp -a "$SRC/." "$APPDIR_ROOT/usr/bin/"

# 3) AppRun (entrypoint) - ensure cwd is usr/bin so Flutter finds data/
cat > "$APPDIR_ROOT/AppRun" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
cd "$HERE/usr/bin"
# Prefer portals on Wayland for file dialogs, etc.
: "${GTK_USE_PORTAL:=1}"; export GTK_USE_PORTAL
export LD_LIBRARY_PATH="$HERE/usr/lib:${LD_LIBRARY_PATH:-}"
exec ./server_gui "$@"
EOF
chmod +x "$APPDIR_ROOT/AppRun"

# 4) Desktop file (at AppDir root as per AppImage spec)
cat > "$APPDIR_ROOT/$APP_ID.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=$APP_NAME
Comment=Mouse Hero Server
Exec=server_gui
Icon=server_gui
Categories=Utility;
StartupWMClass=server_gui
EOF

# 5) Icon (at AppDir root name.png) — required by AppImage desktop file
ICON_PATH="$SRC/data/flutter_assets/assets/app_icon.png"
if [[ -f "$ICON_PATH" ]]; then
  cp -f "$ICON_PATH" "$APPDIR_ROOT/server_gui.png"
  # Ensure the AppImage file icon is set by providing .DirIcon
  ln -sf "server_gui.png" "$APPDIR_ROOT/.DirIcon"
else
  echo "[ERROR] Icon not found: $ICON_PATH" >&2
  exit 1
fi

# 6.1) Bundle runtime dependencies missing on many distros
copy_shared_lib "$SYS_LIB_DIR/libayatana-appindicator3.so.1"
copy_shared_lib "$SYS_LIB_DIR/libayatana-ido3-0.4.so.0"
copy_shared_lib "$SYS_LIB_DIR/libdbusmenu-glib.so.4"
copy_shared_lib "$SYS_LIB_DIR/libdbusmenu-gtk3.so.4"

# 6.2) Verify that no shared libraries are missing
if command -v ldd >/dev/null 2>&1; then
  MISSING_DEPS=$(LD_LIBRARY_PATH="$APPDIR_ROOT/usr/lib:${LD_LIBRARY_PATH:-}" ldd "$APPDIR_ROOT/usr/bin/server_gui" | grep "not found" || true)
  if [[ -n "$MISSING_DEPS" ]]; then
    echo "[ERROR] The following shared libraries are still unresolved:"
    echo "$MISSING_DEPS"
    exit 1
  fi
else
  echo "[WARNING] 'ldd' not available; skipping dependency verification" >&2
fi

# 6) Get appimagetool
APPIMG_TOOL="$GUI_ROOT/build/appimagetool-x86_64.AppImage"
if [[ ! -x "$APPIMG_TOOL" ]]; then
  echo "[INFO] Downloading appimagetool..."
  curl -L -o "$APPIMG_TOOL" https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
  chmod +x "$APPIMG_TOOL"
fi

# 7) Build AppImage
"$APPIMG_TOOL" "$APPDIR_ROOT" "$OUT_DIR/$APPIMAGE_NAME"
echo "[OK] AppImage created: $OUT_DIR/$APPIMAGE_NAME"
