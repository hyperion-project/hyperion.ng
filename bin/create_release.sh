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

#Create RPI .tar.gz
if [ "$buildid" = "rpi" ]; then
outfile="$repodir/deploy/hyperion_$buildid.tar.gz"
echo create $outfile

tar --create --verbose --gzip --absolute-names --show-transformed-names --ignore-failed-read\
	--file "$outfile" \
	--transform "s:$builddir/bin/:hyperion/bin/:" \
	--transform "s:$repodir/effects/:hyperion/effects/:" \
	--transform "s:$repodir/config/:hyperion/config/:" \
	--transform "s:$repodir/bin/service/hyperion.init.sh:hyperion/init.d/hyperion.init.sh:" \
	--transform "s:$repodir/bin/service/hyperion.systemd.sh:hyperion/init.d/hyperion.systemd.sh:" \
	--transform "s:$repodir/bin/service/hyperion.initctl.sh:hyperion/init.d/hyperion.initctl.sh:" \
	--transform "s://:/:g" \
	"$builddir/bin/hyperiond" \
	"$builddir/bin/hyperion-remote" \
	"$builddir/bin/hyperion-v4l2" \
	"$builddir/bin/hyperion-dispmanx" \
	"$repodir/effects/"* \
	"$repodir/bin/service/hyperion.init.sh" \
	"$repodir/bin/service/hyperion.systemd.sh" \
	"$repodir/bin/service/hyperion.initctl.sh" \
	"$repodir/config/hyperion.config.json"
fi
exit 0

#Create Wetek imx6 .tar.gz
if [ "$buildid" = "wetek" ] || [ "$buildid" = "imx6" ]; then
outfile="$repodir/deploy/hyperion_$buildid.tar.gz"
echo create $outfile

tar --create --verbose --gzip --absolute-names --show-transformed-names --ignore-failed-read\
	--file "$outfile" \
	--transform "s:$builddir/bin/:hyperion/bin/:" \
	--transform "s:$repodir/effects/:hyperion/effects/:" \
	--transform "s:$repodir/config/:hyperion/config/:" \
	--transform "s:$repodir/bin/service/hyperion.init.sh:hyperion/init.d/hyperion.init.sh:" \
	--transform "s:$repodir/bin/service/hyperion.systemd.sh:hyperion/init.d/hyperion.systemd.sh:" \
	--transform "s:$repodir/bin/service/hyperion.initctl.sh:hyperion/init.d/hyperion.initctl.sh:" \
	--transform "s://:/:g" \
	"$builddir/bin/hyperiond" \
	"$builddir/bin/hyperion-remote" \
	"$builddir/bin/hyperion-v4l2" \
	"$repodir/effects/"* \
	"$repodir/bin/service/hyperion.init.sh" \
	"$repodir/bin/service/hyperion.systemd.sh" \
	"$repodir/bin/service/hyperion.initctl.sh" \
	"$repodir/config/hyperion.config.json"
fi
exit 0

#Create x32 and x64 .tar.gz
if [ "$buildid" = "x32" ] || [ "$buildid" = "x64" ]; then
outfile="$repodir/deploy/hyperion_$buildid.tar.gz"
echo create $outfile

tar --create --verbose --gzip --absolute-names --show-transformed-names --ignore-failed-read\
	--file "$outfile" \
	--transform "s:$builddir/bin/:hyperion/bin/:" \
	--transform "s:$repodir/effects/:hyperion/effects/:" \
	--transform "s:$repodir/config/:hyperion/config/:" \
	--transform "s:$repodir/bin/service/hyperion.init.sh:hyperion/init.d/hyperion.init.sh:" \
	--transform "s:$repodir/bin/service/hyperion.systemd.sh:hyperion/init.d/hyperion.systemd.sh:" \
	--transform "s:$repodir/bin/service/hyperion.initctl.sh:hyperion/init.d/hyperion.initctl.sh:" \
	--transform "s://:/:g" \
	"$builddir/bin/hyperiond" \
	"$builddir/bin/hyperion-remote" \
	"$builddir/bin/hyperion-v4l2" \
	"$builddir/bin/hyperion-x11" \
	"$repodir/effects/"* \
	"$repodir/bin/service/hyperion.init.sh" \
	"$repodir/bin/service/hyperion.systemd.sh" \
	"$repodir/bin/service/hyperion.initctl.sh" \
	"$repodir/config/hyperion.config.json"
fi
exit 0