#!/bin/bash
#
# File : install-libzdb-mysql.sh
#
#   install libzdb for mysql for building xsync-server
#
# Refer:
#   http://www.tildeslash.com/libzdb/
#   https://bitbucket.org/tildeslash/libzdb/
#
# init created: 2018-02-27
# last updated: 2018-02-27
#
########################################################################
# NOTE: readlink -f not support by MaxOS-X
_file=$(readlink -f $0)

_cdir=$(dirname $_file)
_name=$(basename $_file)

INSTALLDIR=$(dirname $_cdir)/libs

# install on rhel 6.x (centos 6.x)
#
# yum install mysql-devel
# yum install flex
#
# https://stackoverflow.com/questions/18385188/why-am-i-seeing-this-libzdb-configure-error

cd ${_cdir}

echo "---- build and install <libzdb-3.1> ..."
tar -zxf ${_cdir}/libzdb-3.1.tar.gz
cd ${_cdir}/libzdb-3.1/
./configure --prefix=${INSTALLDIR} --without-postgresql --without-sqlite
make && make install
rm -rf ${_cdir}/libzdb-3.1
cd ${_cdir}

echo "[success] libzdb-3.1 for mysql installed at: ${INSTALLDIR}"
