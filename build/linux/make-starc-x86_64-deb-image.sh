#!/bin/sh

#
# $1 - application version
#

#
# Prepare folders structure
#
DEB_PACKAGE_DIR="debdir"
mkdir -p $DEB_PACKAGE_DIR/DEBIAN
mkdir -p $DEB_PACKAGE_DIR/usr/bin
mkdir -p $DEB_PACKAGE_DIR/usr/lib/starc/plugins
ls -l $DEB_PACKAGE_DIR

#
# Copy application binaries
#
APP_BIN_DIR="../../src/_build"
ls -l $APP_BIN_DIR
cp $APP_BIN_DIR/starcapp $DEB_PACKAGE_DIR/usr/bin/starc
cp $APP_BIN_DIR/libcorelib.so.1 $DEB_PACKAGE_DIR/usr/lib/starc/
cp $APP_BIN_DIR/plugins/*.so $DEB_PACKAGE_DIR/usr/lib/starc/plugins/

#
# Create DEBIAN/control file
#
cat << EOF > $DEB_PACKAGE_DIR/DEBIAN/control
Package: starc
Version: $1
Section: base
Priority: optional
Architecture: amd64
Depends: libqt5core5a (>= 5.15.2), libqt5gui5 (>= 5.15.2), libqt5widgets5 (>= 5.15.2)
Maintainer: Your Name <your.email@example.com>
Description: Starc Application
EOF

#
# Build .deb package
#
dpkg-deb --build $DEB_PACKAGE_DIR
mv debdir.deb starc-setup.deb
