#!/bin/sh
set -e

# Обновление списка пакетов
sudo apt-get update -y

# Установка пакетов
sudo apt-get install --no-install-recommends -y git libgstreamer-plugins-base1.0-0 cmake make pkg-config libx11-dev xcb libx11-xcb-dev libxkbcommon-x11-0 libxkbcommon-dev libgtk-3-dev sassc qtbase5-dev qtchooser qtbase5-dev-tools

export CC=clang
mkdir -p output
git submodule update --init --recursive src/3rd_party/qbreakpad

echo "pwd $(pwd)"
echo "parameters $1 $2"

QT_BIN_PATH=$GITHUB_WORKSPACE/Qt/5.15.2/gcc_64/bin
export PATH=$PATH:$QT_BIN_PATH

# Установка MAKEFLAGS
if [ -z "$MAKEFLAGS" ] ; then
  par=$(nproc)
  export MAKEFLAGS=-j$par
fi
echo "MAKEFLAGS=$MAKEFLAGS"

# Функция для создания tar архива
maketar() {
  tar -czvf "$1.tar.gz" -C "$1" .
}

# Функция для сборки проекта
run() {
  QM="${QM:=/usr/bin/qmake}"
  hash $QM > /dev/null 2>&1 || QM=qmake-qt5

  $QM && make
  (cd src && $QM && make)

  mkdir -p $1
  mv bin/linux-"$(uname -m)"/release $1
  maketar $1
  mkdir -p output
  mv "$1".tar.gz output/.
}

ANA_BIN_PATH=/opt/anaconda3/bin
sudo mkdir -p $ANA_BIN_PATH && sudo ln -s /usr/bin/qmake $ANA_BIN_PATH/qmake

run linux-arm64 arm64

[ -d Qt ] && tar -czf arm64-Qt.tar.gz Qt

find output -type d -exec chmod a+rwx {} \;
find output -type f -exec chmod a+rw {} \;