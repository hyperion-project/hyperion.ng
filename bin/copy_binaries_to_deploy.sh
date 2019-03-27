#!/bin/sh

if [ "$#" -ne 2 ] || ! [ -d "$1" ] || ! [ -d "$2" ]; then
	echo "Usage: $0 <BUILD-DIR> <REPO-DIR>" >&2
	exit 1
fi

builddir="$1"
repodir="$2"
echo build directory = $builddir
echo repository root dirrectory = $repodir

outfile="$repodir/deploy/hyperion.tar.gz"
echo create $outfile

tar --create --verbose --gzip --absolute-names --show-transformed-names \
	--file "$outfile" \
	--transform "s:$builddir/bin/:hyperion/bin/:" \
	--transform "s:$repodir/bin/hyperion.init.sh:hyperion/init.d/hyperion.init.sh:" \
	--transform "s://:/:g" \
	"$builddir/bin/hyperiond" \
	"$builddir/bin/hyperion-remote" \
	"$builddir/bin/hyperion-v4l2" \
	"$builddir/bin/gpio2spi" \
	"$builddir/bin/dispmanx2png" \
	"$repodir/bin/hyperion.init.sh" \
