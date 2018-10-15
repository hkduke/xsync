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

XCHOME=${_cdir}/dist/xclient-$verno

function update_dist() {
    echoinfo "update xsync-client(xclient) to dist: ${_cdir}/dist/xclient-$verno"
    olddir=$(pwd)

    echoinfo "build xclient package"

    cd ${_cdir}/src/kafkatools/
    make clean && make

    cd ${_cdir}/src/
    make clean && make

    cd ${_cdir}

    echoinfo "update all xclient packages dist"
    mkdir -p $XCHOME/{bin,conf,lib,sbin,watch-local}

    cd $XCHOME
    ln -sf watch-local watch

    mkdir -p /tmp/xclient-watch-test-stash
    cd $XCHOME/watch/
    ln -sf /tmp/xclient-watch-test-stash test-stash

    cp ${_cdir}/conf/log4crc $XCHOME/conf/
    cp ${_cdir}/conf/xsync-client-conf.xml $XCHOME/conf/    
    cp ${_cdir}/bin/test-stash.sh $XCHOME/bin/
    cp ${_cdir}/bin/common.sh $XCHOME/bin/
    cp ${_cdir}/bin/watch-filters.lua $XCHOME/bin/
    cp ${_cdir}/bin/xclient-status.sh $XCHOME/bin/
    cp ${_cdir}/target/xsync-client-$verno $XCHOME/sbin/
    cp ${_cdir}/target/libkafkatools.so.1.0.0 $XCHOME/sbin/
    cp ${_cdir}/libs/lib/librdkafka.so.1 $XCHOME/lib/

    echoinfo "update xclient links"

    cd $XCHOME/lib/
    ln -sf librdkafka.so.1 librdkafka.so

    cd $XCHOME/sbin/
    ln -sf xsync-client-$verno xsync-client
    ln -sf libkafkatools.so.1.0.0 libkafkatools.so.1

    cd $XCHOME/sbin/
    ln -sf ../bin/watch-filters.lua watch-filters.lua

    echoinfo "generate xclient dist pkg: ${_cdir}/dist/xclient-dist-$verno.tar.gz"
    cd ${_cdir}/dist/
    tar -zcf xclient-dist-$verno.tar.gz ./xclient-$verno/

    ln -s xclient-$verno xclient-current

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
tar -zvcf "$pkgname" --exclude="$_proj/.git" --exclude="$_proj/build" --exclude="$_proj/dist" --exclude="$_proj/libs" --exclude="$_proj/target" "$_proj/"
cd $workdir

if [ -f "$outdir/$pkgname" ]; then
    echo "[INFO] SUCCESS update-pkg: $outdir/$pkgname"
else
    echo "[ERROR] FAILED update-pkg: $outdir/$pkgname"
fi
