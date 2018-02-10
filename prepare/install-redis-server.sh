#!/bin/bash
#
# File : install-redis-server.sh
#
#   install redis for xsync-server only
#
# init created: 2018-02-10
# last updated: 2018-02-10
#
########################################################################
# NOTE: readlink -f not support by MaxOS-X
_file=$(readlink -f $0)

_cdir=$(dirname $_file)
_name=$(basename $_file)

INSTALLDIR=$(dirname $_cdir)/redis
HIREDIS_INSTALLDIR=$(dirname $_cdir)/libs

cd ${_cdir}

echo "---- build and install <redis-3.2.11> ..."

rm -rf ${INSTALLDIR}/bin
mkdir -p ${INSTALLDIR}/conf
tar -zxf ${_cdir}/redis-3.2.11.tar.gz
cd ${_cdir}/redis-3.2.11/
make PREFIX=${INSTALLDIR} test install
cp ${_cdir}/redis-3.2.11/redis.conf ${INSTALLDIR}/conf
cp ${_cdir}/redis-3.2.11/sentinel.conf ${INSTALLDIR}/conf

echo "---- build and install hiredis (c api for redis) devel ..."
cd ${_cdir}/redis-3.2.11/deps/hiredis/
make PREFIX=${HIREDIS_INSTALLDIR} install

#echo "" > ${INSTALLDIR}/start-redis-server.sh

rm -rf ${_cdir}/redis-3.2.11
cd ${_cdir}

echo "[success] redis-3.2.11 installed at: ${INSTALLDIR}"
echo "[success] hiredis libs installed at: ${HIREDIS_INSTALLDIR}"

