#!/bin/bash
#
# @file: update-pkg.sh
#
#   update and package all source files (自动清理并打包源代码).
#
# @author: master@pepstack.com
#
# @create: 2018-06-21 23:11:00
# @update: 2018-10-23
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

# Treat any error as exit
set -o errexit

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

DISTROOT="${_cdir}/dist"

XCLIENT_HOME=${DISTROOT}/xclient-$verno
XSERVER_HOME=${DISTROOT}/xserver-$verno


function update_xserver_dist() {
    echoinfo "update xsync-server to: ${DISTROOT}/xserver-$verno"
    olddir=$(pwd)

    echowarn "TODO: build xserver packages"

    echoinfo "TODO: update all xserver packages"
    mkdir -p ${XSERVER_HOME}/{bin,conf,lib,sbin,service,stash-local}

    echoinfo "TODO: package all xserver packages: ${DISTROOT}/xserver-dist-$verno.tar.gz"
    cd ${DISTROOT}
    #tar -zcf xserver-dist-$verno.tar.gz ./xserver-$verno/

    ln -sf xserver-$verno xserver-current

    echoinfo "create daemontools service script: ${XCLIENT_HOME}/service/run"
    cp ${_cdir}/bin/run ${XSERVER_HOME}/service/
    chmod 1755 ${XSERVER_HOME}/service
    chmod 755 ${XSERVER_HOME}/service/run
    
    cd "$olddir"
}


function update_xclient_dist() {
    echoinfo "update xsync-client to: ${DISTROOT}/xclient-$verno"
    olddir=$(pwd)

    echoinfo "build xclient packages"

    cd ${_cdir}/src/kafkatools/
    make clean && make

    cd ${_cdir}/src/
    make clean && make

    cd ${_cdir}

    echoinfo "update all xclient packages"
    mkdir -p ${XCLIENT_HOME}/{bin,conf,lib,sbin,service,watch-test}

    echoinfo "create watch-test: /tmp/xclient/test-log/stash"
    mkdir -p /tmp/xclient/test-log/stash

    cd ${XCLIENT_HOME}
    ln -s watch-test watch

    cd ${XCLIENT_HOME}/watch/
    ln -s /tmp/xclient/test-log test-log

    cp ${_cdir}/conf/log4crc ${XCLIENT_HOME}/conf/
    cp ${_cdir}/conf/xclient.cfg ${XCLIENT_HOME}/conf/    
    cp ${_cdir}/bin/test-stash.sh ${XCLIENT_HOME}/bin/
    cp ${_cdir}/bin/common.sh ${XCLIENT_HOME}/bin/
    cp ${_cdir}/bin/__trycall.lua ${XCLIENT_HOME}/bin/
    cp ${_cdir}/bin/path-filter-1.lua ${XCLIENT_HOME}/bin/
    cp ${_cdir}/bin/event-task-1.lua ${XCLIENT_HOME}/bin/
    cp ${_cdir}/bin/watch-events-filter.lua ${XCLIENT_HOME}/bin/
    cp ${_cdir}/bin/xclient-status.sh ${XCLIENT_HOME}/bin/
    cp ${_cdir}/target/xsync-client-$verno ${XCLIENT_HOME}/sbin/
    cp ${_cdir}/target/libkafkatools.so.1.0.0 ${XCLIENT_HOME}/sbin/
    cp ${_cdir}/libs/lib/librdkafka.so.1 ${XCLIENT_HOME}/lib/

    echoinfo "update xclient links"

    cd ${XCLIENT_HOME}/lib/
    ln -s librdkafka.so.1 librdkafka.so

    cd ${XCLIENT_HOME}/sbin/
    ln -s xsync-client-$verno xsync-client
    ln -s libkafkatools.so.1.0.0 libkafkatools.so.1

    cd ${XCLIENT_HOME}/watch/
    ln -s ../bin/watch-events-filter.lua events-filter.lua

    echoinfo "package all xclient packages: ${DISTROOT}/xclient-dist-$verno.tar.gz"
    cd ${DISTROOT}
    tar -zcf xclient-dist-$verno.tar.gz ./xclient-$verno/

    ln -s xclient-$verno xclient-current

    echoinfo "create daemontools service script: ${XCLIENT_HOME}/service/run"
    cp ${_cdir}/bin/run ${XCLIENT_HOME}/service/
    chmod 1755 ${XCLIENT_HOME}/service
    chmod 755 ${XCLIENT_HOME}/service/run

    echoinfo "clean all"
    cd ${_cdir}/src/
    make clean
    cd "$olddir"

    echoinfo "update xclient dist packages ok."
}


if [ $# -eq 1 ]; then
    if [ "$1" == "-d" ]; then
        rm -rf ${DISTROOT}
        update_xclient_dist
        update_xserver_dist
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

pkgpath="$_proj"-"$verno"
pkgname="$pkgpath"_"$buildno".tar.gz

echoinfo "create package: $pkgname"
workdir=$(pwd)
outdir=$(dirname $_cdir)

cd $outdir
rm -rf "$pkgpath"
rm -rf "$pkgname"
cp -r "$_proj" "$pkgpath"
tar -zvcf "$pkgname" --exclude="$pkgpath/.git" --exclude="$pkgpath/build" --exclude="$pkgpath/dist" --exclude="$pkgpath/watch-test/test-stash" --exclude="$pkgpath/libs" --exclude="$pkgpath/target" "$pkgpath"
rm -rf "$pkgpath"

cd $workdir
if [ -f "$outdir/$pkgname" ]; then
    echo "[INFO] SUCCESS update-pkg: $outdir/$pkgname"
else
    echo "[ERROR] FAILED update-pkg: $outdir/$pkgname"
fi
