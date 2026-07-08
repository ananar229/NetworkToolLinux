#!/usr/bin/env bash
# Builds dist/NetworkTool-x86_64.AppImage from a clean Release build.
#
# Downloads linuxdeploy, linuxdeploy-plugin-qt and appimagetool (continuous
# GitHub releases) into packaging/.tools/ on first run and reuses them after.
#
# Two workarounds baked in, both specific to running these tools on a very
# new host toolchain (verified necessary on Arch/CachyOS with binutils 2.46):
#   1. The `strip` binary bundled inside the linuxdeploy/-plugin-qt AppImages
#      is too old to understand the newer ELF `.relr.dyn` relocation section
#      and aborts the whole run. We replace it with a symlink to the host's
#      own /usr/bin/strip inside each extracted AppImage.
#   2. KDE's kimageformats package ships a few Qt imageformat plugins
#      (avif/heif/jxr) whose codec libraries aren't installed on this host,
#      which makes linuxdeploy-plugin-qt hard-fail trying to resolve them.
#      They're irrelevant to this app (only PNG/SVG icons are used), so
#      they're excluded from deployment.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build-release"
DIST_DIR="$ROOT_DIR/dist"
APPDIR="$DIST_DIR/AppDir"
TOOLS_DIR="$ROOT_DIR/packaging/.tools"

ARCH="$(uname -m)"
LINUXDEPLOY="$TOOLS_DIR/core/squashfs-root/AppRun"
LINUXDEPLOY_QT="$TOOLS_DIR/qt/squashfs-root/AppRun"
APPIMAGETOOL="$TOOLS_DIR/pack/squashfs-root/AppRun"

fetch_and_patch() {
    local name="$1" url="$2" dir="$TOOLS_DIR/$1"
    if [ -x "$dir/squashfs-root/AppRun" ]; then
        return
    fi
    mkdir -p "$dir"
    curl -fL -o "$dir/tool.AppImage" "$url"
    chmod +x "$dir/tool.AppImage"
    (cd "$dir" && ./tool.AppImage --appimage-extract >/dev/null)
    if [ -f "$dir/squashfs-root/usr/bin/strip" ]; then
        rm -f "$dir/squashfs-root/usr/bin/strip"
        ln -s /usr/bin/strip "$dir/squashfs-root/usr/bin/strip"
    fi
}

echo "==> Fetching packaging tools (cached in packaging/.tools/)"
fetch_and_patch core "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage"
fetch_and_patch qt "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${ARCH}.AppImage"
fetch_and_patch pack "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-${ARCH}.AppImage"

echo "==> Configuring + building Release"
CMAKE_BIN="${CMAKE_BIN:-cmake}"
"$CMAKE_BIN" -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
"$CMAKE_BIN" --build "$BUILD_DIR" --parallel

echo "==> Installing into AppDir"
rm -rf "$APPDIR"
"$CMAKE_BIN" --install "$BUILD_DIR" --prefix "$APPDIR/usr"

echo "==> Deploying base executable + desktop file + icon"
"$LINUXDEPLOY" --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/NetworkTool" \
    --desktop-file "$APPDIR/usr/share/applications/networktool.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/512x512/apps/networktool.png"

echo "==> Deploying Qt runtime (platform plugins, imageformats, translations)"
QMAKE="${QMAKE:-/usr/bin/qmake6}" "$LINUXDEPLOY_QT" --appdir "$APPDIR" \
    --exclude-library='*kimg_avif*' \
    --exclude-library='*kimg_heif*' \
    --exclude-library='*kimg_jxr*'

echo "==> Resolving dependencies of newly added Qt plugins + stripping"
"$LINUXDEPLOY" --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/NetworkTool" \
    --desktop-file "$APPDIR/usr/share/applications/networktool.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/512x512/apps/networktool.png" \
    --deploy-deps-only="$APPDIR/usr/plugins"

echo "==> Packaging AppImage"
ARCH="$ARCH" "$APPIMAGETOOL" "$APPDIR" "$DIST_DIR/NetworkTool-${ARCH}.AppImage"

echo "==> Done: $DIST_DIR/NetworkTool-${ARCH}.AppImage"
