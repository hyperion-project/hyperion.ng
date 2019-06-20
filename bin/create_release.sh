#!/bin/sh

if [ "$#" -ne 2 ] || ! [ -d "$1" ]; then
  echo "Usage: $0 <REPO-DIR> <BUILD-ID>" >&2
  exit 1
fi

repodir="$1"
buildid="$2"
builddir=$repodir/build-$buildid
echo build directory = $builddir
echo repository root dirrectory = $repodir
if ! [ -d "$builddir" ]; then
	echo "Could not find build director"
	exit 1
fi
	
outfile="$repodir/deploy/hyperion_$buildid.tar.gz"
echo create $outfile

tar --create --gzip --absolute-names --show-transformed-names --ignore-failed-read\
	--file "$outfile" \
	--transform "s:$builddir/bin/:hyperion/bin/:" \
	--transform "s:$repodir/config/:hyperion/config/:" \
	--transform "s:$repodir/bin/service/hyperion.init.sh:hyperion/services/hyperion.init.sh:" \
	--transform "s:$repodir/bin/service/hyperion.systemd.sh:hyperion/services/hyperion.systemd.sh:" \
	--transform "s:$repodir/bin/service/hyperion.initctl.sh:hyperion/services/hyperion.initctl.sh:" \
	--transform "s://:/:g" \
	"$builddir/bin/hyperion"* \
	"$repodir/bin/service/hyperion.init" \
	"$repodir/bin/service/hyperion.systemd" \
	"$repodir/bin/service/hyperion.initctl" \
	"$repodir/config/hyperion.config.json.default"

