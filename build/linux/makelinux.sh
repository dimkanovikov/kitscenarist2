#!/bin/sh
set -e

echo "pwd $(pwd)"
echo "parameters $1 $2"

export PATH=$PATH:$GITHUB_WORKSPACE/Qt/$1/gcc_64/bin

if [ -z "$MAKEFLAGS" ] ; then
if [ "$(uname)"  = "Linux" ]; then par=$(nproc); else par=$(sysctl -n hw.ncpu); fi
export MAKEFLAGS=-j$par
fi
echo "MAKEFLAGS=$MAKEFLAGS"

maketar() {
 cd $1
 tar -czvf ../"$1".tar.gz *
 cd -
}

run() {
QM="${QM:=/usr/bin/qmake}"
hash $QM &> /dev/null
if [ $? -eq 1 ]; then
  echo 'use qmake-qt5' >&2
  QM=qmake-qt5
fi

rm -rf "$1"
$QM && make
cd src
$QM && make
cd -

mv bin/linux-"$(uname -m)"/release $1
maketar $1
mkdir -p output
mv "$1".tar.gz output/.

ls -l "$1" || true
rm -rf "$1"
}


sudo mkdir -p /opt/anaconda3/bin && sudo ln -s /usr/bin/qmake /opt/anaconda3/bin/qmake

run linux-"$2" "$2"

if [ -d Qt ] ; then
tar -czf "$2"-Qt.tar.gz Qt
fi