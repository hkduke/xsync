#!/bin/bash
#
# @file: update-pkg.sh
#
#   update and package all source files (自动清理并打包源代码).
#
# @author: master@pepstack.com
#
# @create: 2018-06-21 23:11:00
# @update: 2018-08-29
#
#######################################################################
# will cause error on macosx
_file=$(readlink -f $0)
_cdir=$(dirname $_file)
_name=$(basename $_file)

. $_cdir/common.sh

# Set characters encodeing
#   LANG=en_US.UTF-8;export LANG
LANG=zh_CN.UTF-8;export LANG

# Treat unset variables as an error
set -o nounset

#######################################################################

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

function update_dist() {
    echoinfo "update xsync-client to dist: ${_cdir}/dist/xclient"
    olddir=$(pwd)
    cp ${_cdir}/conf/log4crc ${_cdir}/dist/xclient/conf/
    cp ${_cdir}/conf/xsync-client-conf.xml ${_cdir}/dist/xclient/conf/    
    cp ${_cdir}/bin/testlog.sh ${_cdir}/dist/xclient/bin/
    cp ${_cdir}/bin/common.sh ${_cdir}/dist/xclient/bin/
    cp ${_cdir}/bin/path-filter-*.sh ${_cdir}/dist/xclient/bin/
    cp ${_cdir}/bin/event-task-*.sh ${_cdir}/dist/xclient/bin/
    cp ${_cdir}/bin/xclient-status.sh ${_cdir}/dist/xclient/bin/
    cp ${_cdir}/target/xsync-client-$verno ${_cdir}/dist/xclient/sbin/
    cd ${_cdir}/dist/xclient/sbin/
    ln -sf ${_cdir}/dist/xclient/sbin/xsync-client-$verno xsync-client
    cd "$olddir"
}


if [ "$1" == "-d" ]; then
    update_dist
    exit 0;
fi

 
echoinfo "update date and version ..."
${_cdir}/revise.py \
    --path=$_cdir/src \
    --filter="c,cpp,bash" \
    --author="$author" \
    --updver="$verno" \
    --recursive

echoinfo "update server.mk client.mk version ..."
sed -i "s/^VERSION :=.*$/VERSION := `cat VERSION`/" src/server/server.mk
sed -i "s/^VERSION :=.*$/VERSION := `cat VERSION`/" src/client/client.mk

sed -i "s/^#define XSYNC_VERSION .*$/#define XSYNC_VERSION    \"`cat VERSION`\"/" src/xsync-config.h

echoinfo "clean all sources"
find ${_cdir} -name *.pyc | xargs rm -f
find ${_cdir} -name *.o | xargs rm -f
find ${_cdir}/utils -name *.pyc | xargs rm -f


pkgname="$_proj"-"$verno"_"$buildno".tar.gz

echoinfo "create package: $pkgname"
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
