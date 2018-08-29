#!/bin/bash
#
# File : install-devel-libs.sh
#
#   install devel lib for building both xsync-client and xsync-server
#
# init created: 2018-01-19
# last updated: 2018-02-25
#
########################################################################
# NOTE: readlink -f not support by MaxOS-X
_file=$(readlink -f $0)

_cdir=$(dirname $_file)
_name=$(basename $_file)

INSTALLDIR=$(dirname $_cdir)/libs

cd ${_cdir}

echo "---- build and install <mxml-2.9> ..."
tar -zxf ${_cdir}/mxml-2.9.tgz
cd ${_cdir}/mxml-2.9/
./configure --prefix=${INSTALLDIR}
make && make install
rm -rf ${_cdir}/mxml-2.9
cd ${_cdir}

echo "---- build and install <expat-2.1.0> ..."
tar -zxf ${_cdir}/expat-2.1.0.tgz
cd ${_cdir}/expat-2.1.0/
./configure --prefix=${INSTALLDIR}
make && make install
rm -rf ${_cdir}/expat-2.1.0
cd ${_cdir}

echo "---- build and install <jemalloc-3.6.0> ..."
tar -zxf ${_cdir}/jemalloc-3.6.0.tgz
cd ${_cdir}/jemalloc-3.6.0/
./configure --prefix=${INSTALLDIR}
make && make install
rm -rf ${_cdir}/jemalloc-3.6.0
cd ${_cdir}

echo "---- build and install <log4c-1.2.4>..."
tar -zxf ${_cdir}/log4c-1.2.4.tgz
cd ${_cdir}/log4c-1.2.4/
./configure --prefix=${INSTALLDIR} --without-expat
make && make install
rm -rf ${_cdir}/log4c-1.2.4
cd ${_cdir}

echo "---- build and install <sqlite-autoconf-3130000> ..."
tar -zxf ${_cdir}/sqlite-autoconf-3130000.tgz
cd ${_cdir}/sqlite-autoconf-3130000/
./configure --prefix=${INSTALLDIR}
make && make install
rm -rf ${_cdir}/sqlite-autoconf-3130000
cd ${_cdir}

echo "---- build and install <openssl-1.0.2e> ..."
tar -zxf ${_cdir}/openssl-1.0.2e.tgz
cd ${_cdir}/openssl-1.0.2e/
./config --prefix=${INSTALLDIR}
make && make install
rm -rf ${_cdir}/openssl-1.0.2e
cd ${_cdir}

echo "---- build and install <zlib-1.2.11> ..."
tar -zxf ${_cdir}/zlib-1.2.11.tar.gz
cd ${_cdir}/zlib-1.2.11/
./configure --prefix=${INSTALLDIR}
make && make install
rm -rf ${_cdir}/zlib-1.2.11
cd ${_cdir}

echo "---- build and install <libevent-2.1.8-stable> ..."
tar -zxf ${_cdir}/libevent-2.1.8-stable.tar.gz
cd ${_cdir}/libevent-2.1.8-stable/
./configure --prefix=${INSTALLDIR}
make && make install
rm -rf ${_cdir}/libevent-2.1.8-stable
cd ${_cdir}

echo "[success] all packages installed at: ${INSTALLDIR}"
