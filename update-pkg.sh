#!/bin/bash
# @file: update-package.sh
#
#   update and package all source files (自动清理并打包源代码).
#
# @author: master@pepstack.com
#
# @create: 2018-06-21 23:11:00
# @update: 2018-08-10
#
#######################################################################
# will cause error on macosx
_file=$(readlink -f $0)

_cdir=$(dirname $_file)
_name=$(basename $_file)

_proj=$(basename $_cdir)

_ver=$(cat $_cdir/VERSION)

author=$(cat $_cdir/AUTHOR)

#######################################################################

# Major_Version_Number.Minor_Version_Number[.Revision_Number[.Build_Number]]
# app-1.2.3_build201806221413.tar.gz

OLD_IFS="$IFS"
IFS="."
versegs=($_ver)
IFS="$OLD_IFS"

majorVer="${versegs[0]}"
minorVer="${versegs[1]}"
revisionVer="${versegs[2]}"

verno="$majorVer"."$minorVer"."$revisionVer"
buildno="build$(date +%Y%m%d%H%M)"

echo "[INFO] update date and version ..."
${_cdir}/revise.py \
    --path=$_cdir/src \
    --filter="c,cpp,bash" \
    --author="$author" \
    --updver="$verno" \
    --recursive

echo "[INFO] update server.mk client.mk version ..."
sed -i "s/^VERSION :=.*$/VERSION := `cat VERSION`/" src/server/server.mk
sed -i "s/^VERSION :=.*$/VERSION := `cat VERSION`/" src/client/client.mk

sed -i "s/^#define XSYNC_VERSION .*$/#define XSYNC_VERSION    \"`cat VERSION`\"/" src/xsync-config.h

echo "[INFO] clean all sources"
find ${_cdir} -name *.pyc | xargs rm -f
find ${_cdir} -name *.o | xargs rm -f
find ${_cdir}/utils -name *.pyc | xargs rm -f


pkgname="$_proj"-"$verno"_"$buildno".tar.gz

echo "[INFO] create package: $pkgname"
workdir=$(pwd)
outdir=$(dirname $_cdir)
cd $outdir
rm -rf "$pkgname"
tar -zvcf "$pkgname" --exclude="$_proj/.git" --exclude="$_proj/target" "$_proj/"
cd $workdir

if [ -f "$outdir/$pkgname" ]; then
    echo "[INFO] SUCCESS update-pkg: $outdir/$pkgname"
else
    echo "[ERROR] FAILED update-pkg: $outdir/$pkgname"
fi
