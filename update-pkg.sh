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
    echoinfo "update xsync-client(xclient) to dist: ${_cdir}/dist/xclient"
    olddir=$(pwd)

    echoinfo "build xclient package"
    cd ${_cdir}/src/
    make clean && make
    cd ${_cdir}

    echoinfo "update xclient dist"
    mkdir -p ${_cdir}/dist/xclient/{bin,conf,sbin,watch}
    cp ${_cdir}/conf/log4crc ${_cdir}/dist/xclient/conf/
    cp ${_cdir}/conf/xsync-client-conf.xml ${_cdir}/dist/xclient/conf/    
    cp ${_cdir}/bin/testlog.sh ${_cdir}/dist/xclient/bin/
    cp ${_cdir}/bin/common.sh ${_cdir}/dist/xclient/bin/
    cp ${_cdir}/bin/path-filter-0.0.1.sh ${_cdir}/dist/xclient/bin/
    cp ${_cdir}/bin/event-task-0.0.1.sh ${_cdir}/dist/xclient/bin/
    cp ${_cdir}/bin/xclient-status.sh ${_cdir}/dist/xclient/bin/
    cp ${_cdir}/target/xsync-client-$verno ${_cdir}/dist/xclient/sbin/

    echoinfo "update xclient links"
    cd ${_cdir}/dist/xclient/sbin/
    ln -sf ${_cdir}/dist/xclient/sbin/xsync-client-$verno xsync-client

    cd ${_cdir}/dist/xclient/watch/
    ln -sf ${_cdir}/dist/xclient/bin/path-filter-0.0.1.sh path-filter.sh
    ln -sf ${_cdir}/dist/xclient/bin/event-task-0.0.1.sh event-task.sh

    echoinfo "generate xclient dist pkg: ${_cdir}/dist/xclient-dist-$verno.tar.gz"
    cd ${_cdir}/dist/
    tar -zcf xclient-dist-$verno.tar.gz ./xclient/

    echoinfo "clean all"
    cd ${_cdir}/src/
    make clean
    cd "$olddir"

    echoinfo "update xclient dist package ok."
}


if [ $# -eq 1 ]; then
    if [ "$1" == "-d" ]; then
        update_dist
        exit 0;
    fi
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
