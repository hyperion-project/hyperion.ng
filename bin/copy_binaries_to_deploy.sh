#!/bin/sh

if [ "$#" -ne 2 ] || ! [ -d "$1" ] || ! [ -d "$2" ]; then
  echo "Usage: $0 <BUILD-DIR> <REPO-DIR>" >&2
  exit 1
fi

builddir="$1"
repodir="$2"
echo build directory = $builddir
echo repository root dirrectory = $repodir

echo Copying binaries
cp -v "$builddir"/bin/hyperiond       "$repodir"/deploy
cp -v "$builddir"/bin/hyperion-remote "$repodir"/deploy
cp -v "$builddir"/bin/gpio2spi        "$repodir"/deploy
