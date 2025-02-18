#!/bin/sh

#
# $1 - application version
#

#
# Prepare folders structure
#
APP_IMAGE_DIR="appdir"
mkdir $APP_IMAGE_DIR/lib
mkdir $APP_IMAGE_DIR/plugins
ls -l $APP_IMAGE_DIR

#
# Copy application binaries
#
APP_BIN_DIR="../../src/_build"
ls -l $APP_BIN_DIR
cp $APP_BIN_DIR/starcapp $APP_IMAGE_DIR/starc
cp $APP_BIN_DIR/libcorelib.so.1 $APP_IMAGE_DIR/lib/
cp $APP_BIN_DIR/plugins/*.so $APP_IMAGE_DIR/plugins/

#
# Copy flathub required files
#
FLATHUB_DIR="flathub/share"
ls -l $FLATHUB_DIR
cp -R $FLATHUB_DIR $APP_IMAGE_DIR

#
# Copy qgnomeplatform library
#
cp /usr/lib/x86_64-linux-gnu/libqgnomeplatform.so $APP_IMAGE_DIR/lib/

#
# Make installer
#
wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage" -O linuxdeployqt
chmod a+x linuxdeployqt
./linuxdeployqt appdir/starc -no-copy-copyright-files -no-translations -always-overwrite -extra-plugins=platforms,wayland-decoration-client,wayland-graphics-integration-client,wayland-shell-integration -executable=appdir/plugins/libcoreplugin.so -appimage
mv *.AppImage starc-setup.AppImage
