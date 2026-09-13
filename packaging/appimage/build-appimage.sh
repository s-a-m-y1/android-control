#!/usr/bin/env bash
set -e
ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
APPDIR="$ROOT_DIR/packaging/appimage/AndroidControl.AppDir"

echo "Building AppImage AppDir at $APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/scalable/apps"

if [ -f "$BUILD_DIR/android-control" ]; then
    cp "$BUILD_DIR/android-control" "$APPDIR/usr/bin/"
else
    echo "Binary not found, building..."
    "$ROOT_DIR/scripts/build.sh"
    cp "$BUILD_DIR/android-control" "$APPDIR/usr/bin/"
fi

cp "$ROOT_DIR/packaging/desktop-entry/com.github.androidcontrol.desktop" "$APPDIR/usr/share/applications/"
cp "$ROOT_DIR/desktop/resources/icons/android-control.svg" "$APPDIR/usr/share/icons/hicolor/scalable/apps/"
cp "$ROOT_DIR/packaging/desktop-entry/com.github.androidcontrol.desktop" "$APPDIR/android-control.desktop"
cp "$ROOT_DIR/desktop/resources/icons/android-control.svg" "$APPDIR/android-control.svg"

cat > "$APPDIR/AppRun" <<'EOS'
#!/bin/sh
SELF=$(readlink -f "$0")
HERE=${SELF%/*}
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"
exec "$HERE/usr/bin/android-control" "$@"
EOS
chmod +x "$APPDIR/AppRun"

echo "AppDir ready. To create AppImage, install appimagetool:"
echo "  wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
echo "  chmod +x appimagetool-x86_64.AppImage && ./appimagetool-x86_64.AppImage $APPDIR"
