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
LIB_PATH=$(sudo find / -name libqgnomeplatform.so 2>/dev/null)
if [ -n "$LIB_PATH" ]
then
    cp $LIB_PATH $APP_IMAGE_DIR/lib/
else
    echo "libqgnomeplatform.so not found"
    exit 1
fi



#
# Make installer
#
# Install appimage-builder
sudo apt install python3-pip python3-setuptools patchelf desktop-file-utils libgdk-pixbuf2.0-dev fakeroot strace
pip3 install appimage-builder

# Create AppImage recipe file
cat > AppImageBuilder.yml <<EOF
version: 1
script: |
  mkdir -p AppDir/usr/bin
  cp $APP_IMAGE_DIR/starc AppDir/usr/bin/
  cp -r $APP_IMAGE_DIR/lib AppDir/usr/
  cp -r $APP_IMAGE_DIR/plugins AppDir/usr/
  cp -r $APP_IMAGE_DIR/share AppDir/usr/
  mkdir -p AppDir/usr/share/icons/hicolor/128x128/apps/
  cp $APP_IMAGE_DIR/starc.png AppDir/usr/share/icons/hicolor/128x128/apps/

AppDir:
  path: ./AppDir
  app_info:
    id: org.example.yourapp
    name: starc-setup
    icon: starc.png
    version: $1
    exec: usr/bin/starc
    exec_args: \$@
  runtime:
    env:
      APPDIR_LIBRARY_PATH: $APPDIR/usr/lib
  path: https://github.com/AppImage/AppImageKit/releases/download/continuous/runtime-aarch64
AppImage:
  arch: aarch64
  update-information: None
  sign-key: None
EOF

# Build AppImage
appimage-builder --recipe AppImageBuilder.yml --skip-tests