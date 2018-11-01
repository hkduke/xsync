#!/bin/bash
#
# @file : install-devel-libs.sh
#
#   install devel lib for building both xsync-client and xsync-server
#
# @create: 2018-01-19
# @update: 2018-10-25
#
########################################################################
# NOTE: readlink -f not support by MaxOS-X
_file=$(readlink -f $0)

_cdir=$(dirname $_file)
_name=$(basename $_file)
_ver=0.0.1

INSTALLDIR=$(dirname $_cdir)/libs

. $_cdir/common.sh

# Set characters encodeing
#   LANG=en_US.UTF-8;export LANG
LANG=zh_CN.UTF-8;export LANG

# https://blog.csdn.net/drbinzhao/article/details/8281645
# Treat unset variables as an error
set -o nounset

# Treat any error as exit
set -o errexit

###########################################################

function install_autoconf()
{
    echoinfo "---- check and install autoconf (http://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz)"
    appname=$(autoconf --version | awk 'NR==1{print $1}')
    appverno=$(autoconf --version | awk 'NR==1{print $4}')

    echoinfo "found: $appname-$appverno"

    pkgver="2.69"
    installpkg="no"

    if [ "$appname" != "autoconf" ]; then
        installpkg="yes"
    elif [ `echo "$appverno < $pkgver" | bc` -ne 0 ]; then
        installpkg="yes"
    fi

    if [ "$installpkg" == "yes" ]; then
        echoinfo "installing: $appname-$pkgver ..."

        cd ${_cdir}
        tar -zxf ${_cdir}/autoconf-2.69.tar.gz
        cd ${_cdir}/autoconf-2.69
        ./configure
        make && sudo make install
        cd ${_cdir}
    fi
}


function install_automake()
{
    echoinfo "---- check and install automake (http://ftp.gnu.org/gnu/automake/automake-1.15.tar.gz)"
    appname=$(automake --version | awk 'NR==1{print $1}')
    appverno=$(automake --version | awk 'NR==1{print $4}')

    echoinfo "found: $appname-$appverno"

    pkgver="1.15"
    installpkg="no"

    if [ "$appname" != "automake" ]; then
        installpkg="yes"
    elif [ `echo "$appverno < $pkgver" | bc` -ne 0 ]; then
        installpkg="yes"
    fi

    if [ "$installpkg" == "yes" ]; then
        echoinfo "installing: $appname-$pkgver ..."

        cd ${_cdir}
        tar -zxf ${_cdir}/automake-1.15.tar.gz
        cd ${_cdir}/automake-1.15
        ./configure
        make && sudo make install
        cd ${_cdir}
    fi
}


function install_libtool()
{
    echoinfo "---- check and install libtool (http://ftp.gnu.org/gnu/libtool/libtool-2.2.6b.tar.gz)"
    appname=$(libtool --version | awk 'NR==1{print $1}')
    appverno=$(libtool --version | awk 'NR==1{print $4}')

    echoinfo "found: $appname-$appverno"

    pkgver="2.2.6c"
    installpkg="no"

    if [ "$appname" != "ltmain.sh" ]; then
        installpkg="yes"
    elif [ "$appverno" \< "$pkgver" ]; then
        installpkg="yes"
    fi

    if [ "$installpkg" == "yes" ]; then
        echoinfo "installing: $appname-$pkgver ..."

        cd ${_cdir}
        tar -zxf ${_cdir}/libtool-2.2.6b.tar.gz
        cd ${_cdir}/libtool-2.2.6b
        ./configure
        make && sudo make install
        cd ${_cdir}
    fi
}

exit 0
