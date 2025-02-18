#!/bin/sh

# $1 - application version

# Prepare folders structure
APP_DIR="appdir"
BIN_DIR="$APP_DIR/usr/bin"
LIB_DIR="$APP_DIR/usr/lib"
PLUGINS_DIR="$APP_DIR/usr/lib/plugins"
mkdir -p $APP_DIR/DEBIAN $BIN_DIR $LIB_DIR $PLUGINS_DIR

# Create control file
echo "Package: starc
Version: $1
Section: base
Priority: optional
Architecture: x86_64
Maintainer: StarcAppTeam <team@starc.app>
Description: Starc Application" > $APP_DIR/DEBIAN/control

# Copy application binaries
APP_BIN_DIR="../../src/_build"
cp $APP_BIN_DIR/starcapp $BIN_DIR/starc
cp $APP_BIN_DIR/libcorelib.so.1 $LIB_DIR/
cp $APP_BIN_DIR/plugins/*.so $PLUGINS_DIR/

# Copy flathub required files
FLATHUB_DIR="flathub/share"
cp -R $FLATHUB_DIR $APP_DIR

# Copy qgnomeplatform library
cp /usr/lib/x86_64-linux-gnu/libqgnomeplatform.so $LIB_DIR/

# Create DEB package
DEB_PACKAGE="starc-setup.deb"
dpkg-deb --build $APP_DIR $DEB_PACKAGE