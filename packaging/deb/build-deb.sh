#!/usr/bin/env bash
set -e
ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
VERSION="${1:-1.0.0}"
ARCH=$(dpkg --print-architecture 2>/dev/null || echo "amd64")
PKG_DIR="$ROOT_DIR/packaging/deb/android-control_${VERSION}_${ARCH}"

echo "Building .deb package $VERSION $ARCH"

mkdir -p "$PKG_DIR/DEBIAN"
mkdir -p "$PKG_DIR/usr/bin"
mkdir -p "$PKG_DIR/usr/share/applications"
mkdir -p "$PKG_DIR/usr/share/icons/hicolor/scalable/apps"
mkdir -p "$PKG_DIR/usr/share/doc/android-control"

# Binary
if [ -f "$BUILD_DIR/android-control" ]; then
    cp "$BUILD_DIR/android-control" "$PKG_DIR/usr/bin/"
else
    echo "Binary not found, building..."
    "$ROOT_DIR/scripts/build.sh"
    cp "$BUILD_DIR/android-control" "$PKG_DIR/usr/bin/"
fi

cp "$ROOT_DIR/packaging/desktop-entry/com.github.androidcontrol.desktop" "$PKG_DIR/usr/share/applications/"
cp "$ROOT_DIR/desktop/resources/icons/android-control.svg" "$PKG_DIR/usr/share/icons/hicolor/scalable/apps/"

cat > "$PKG_DIR/DEBIAN/control" <<EOF
Package: android-control
Version: $VERSION
Section: utils
Priority: optional
Architecture: $ARCH
Depends: android-sdk-platform-tools, scrcpy, libqt6widgets6, libqt6gui6, libqt6core6t64, libspdlog1.15, libfmt10
Maintainer: Android Control <sam858y@gmail.com>
Description: Modern Android screen mirroring and control for Ubuntu
 Mirror and control Android devices via USB using ADB and scrcpy.
 Features real-time mirroring, input control, file transfer, clipboard
 sync, screenshots and recording with Qt6 GUI.
EOF

cat > "$PKG_DIR/DEBIAN/postinst" <<'EOS'
#!/bin/sh
set -e
update-desktop-database || true
gtk-update-icon-cache -f /usr/share/icons/hicolor || true
echo "Android Control installed. Launch via 'android-control' or app menu."
EOS
chmod 755 "$PKG_DIR/DEBIAN/postinst"

dpkg-deb --build "$PKG_DIR"
echo "Built: $PKG_DIR.deb"
ls -lh "$PKG_DIR.deb"
