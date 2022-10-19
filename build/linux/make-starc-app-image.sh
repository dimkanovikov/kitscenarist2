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
mkdir $APP_IMAGE_DIR/share
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
FLATHUB_DIR="../../dist/share"
ls -l $FLATHUB_DIR
cp -R $FLATHUB_DIR $APP_IMAGE_DIR/share

#
# Make installer
#
wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage" -O linuxdeployqt
chmod a+x linuxdeployqt
./linuxdeployqt appdir/starc -no-copy-copyright-files -no-translations -always-overwrite -executable=appdir/plugins/libcoreplugin.so -appimage
mv *.AppImage starc-setup.AppImage
