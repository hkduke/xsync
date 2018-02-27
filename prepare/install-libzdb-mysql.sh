#!/bin/bash
#
# File : install-libzdb-mysql.sh
#
#   install libzdb for mysql for building xsync-server
#
# MySQL client packages downloads:
#   http://ftp.ntu.edu.tw/MySQL/Downloads/MySQL-5.6/
#
# Refer:
#   http://www.tildeslash.com/libzdb/
#   https://bitbucket.org/tildeslash/libzdb/
#
# NOTE:
#   1) 在 ubuntu 上编译需要安装 libmysqlclient:
#      sudo apt-get install libmysqlclient-dev
#
#   2) 在 rhel6 上编译需要安装 libmysqlclient:
#
#     先删除已经安装的MySQL包 (?):
#      # rpm -qa|grep MySQL
#        MySQL-client-5.6.39-1.el6.x86_64
#        MySQL-devel-5.6.39-1.el6.x86_64
#        MySQL-shared-5.6.39-1.el6.x86_64
#        MySQL-shared-compat-5.6.39-1.el6.x86_64
#
#      # rpm -e MySQL-client-5.6.39-1.el6.x86_64
#      # rpm -e MySQL-devel-5.6.39-1.el6.x86_64
#      # rpm -e MySQL-shared-compat-5.6.39-1.el6.x86_64 --nodeps
#      # rpm -e MySQL-shared-5.6.39-1.el6.x86_64 --nodeps
#
#     然后安装:
#      # rpm -ivh MySQL-client-5.6.39-1.el6.x86_64.rpm MySQL-devel-5.6.39-1.el6.x86_64.rpm MySQL-shared-compat-5.6.39-1.el6.x86_64.rpm MySQL-shared-5.6.39-1.el6.x86_64.rpm
#
#     最后的libmysqlclient.so 在:
#       /usr/lib64/
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

cd ${_cdir}

echo "---- build and install <libzdb-3.1> ..."
tar -zxf ${_cdir}/libzdb-3.1.tar.gz
cd ${_cdir}/libzdb-3.1/
./configure --prefix=${INSTALLDIR} --without-postgresql --without-sqlite
make && make install
rm -rf ${_cdir}/libzdb-3.1
cd ${_cdir}

echo "[success] libzdb-3.1 for mysql installed at: ${INSTALLDIR}"
