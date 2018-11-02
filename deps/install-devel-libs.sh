#!/bin/bash
#
# @file : install-devel-libs.sh
#
#   install devel lib for building both xsync-client and xsync-server
#
# @create: 2018-01-19
# @update: 2018-11-02
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

usage()
{
    cat << EOT

Usage :  ${_name}
  install all dependencies libs for development of xsync-server and xsync-client.

Options:
  -h, --help                  show help information
  -V, --version               show version

  --prepare                   prepare for devel environments
  --install                   install all dependencies at: ${INSTALLDIR}

example:
  $ sudo ${_name} --prepare
  $ ${_name} --install

report: 350137278@qq.com
EOT
}   # ----------  end of function usage  ----------


function install_autoconf()
{
    appname=$(autoconf --version | awk 'NR==1{print $1}')
    appverno=$(autoconf --version | awk 'NR==1{print $4}')

    pkgver="2.69"
    installpkg="no"

    echoinfo "package: autoconf-$pkgver.tar.gz"

    if [ "$appname" != "autoconf" ]; then
        installpkg="yes"

        echowarn "not found: autoconf"
    elif [ `echo "$appverno < $pkgver" | bc` -ne 0 ]; then
        installpkg="yes"

        echowarn "update: autoconf-$appverno"
    else
        echowarn "found existed: autoconf-$appverno"
    fi

    if [ "$installpkg" == "yes" ]; then
        echoinfo "installing: $appname-$pkgver ..."

        cd ${_cdir}
        tar -zxf ${_cdir}/autoconf-2.69.tar.gz
        cd ${_cdir}/autoconf-2.69
        ./configure
        make && sudo make install
        rm -rf "${_cdir}/autoconf-2.69"
        cd ${_cdir}
    fi
}


function install_automake()
{
    appname=$(automake --version | awk 'NR==1{print $1}')
    appverno=$(automake --version | awk 'NR==1{print $4}')

    pkgver="1.15"
    installpkg="no"

    echoinfo "package: automake-$pkgver.tar.gz"

    if [ "$appname" != "automake" ]; then
        installpkg="yes"
        
        echowarn "not found: automake"
    elif [ `echo "$appverno < $pkgver" | bc` -ne 0 ]; then
        installpkg="yes"
        
        echowarn "update: automake-$appverno"
    else
        echowarn "found existed: automake-$appverno"
    fi

    if [ "$installpkg" == "yes" ]; then
        echoinfo "installing: $appname-$pkgver ..."

        cd ${_cdir}
        tar -zxf ${_cdir}/automake-1.15.tar.gz
        cd ${_cdir}/automake-1.15
        ./configure
        make && sudo make install
        rm -rf "${_cdir}/automake-1.15"
        cd ${_cdir}
    fi
}


function install_libtool()
{
    appname=$(libtool --version | awk 'NR==1{print $1}')
    appverno=$(libtool --version | awk 'NR==1{print $4}')

    pkgver="2.2.6b"
    installpkg="no"

    echoinfo "package: libtool-$pkgver.tar.gz"

    if [ "$appname" != "ltmain.sh" ]; then
        installpkg="yes"

        echowarn "not found: libtool"
    elif [ "$appverno" \< "$pkgver" ]; then
        installpkg="yes"

        echowarn "update: libtool-$appverno"
    else
        echowarn "found existed: libtool-$appverno"
    fi

    if [ "$installpkg" == "yes" ]; then
        echoinfo "installing: $appname-$pkgver ..."

        cd ${_cdir}
        tar -zxf ${_cdir}/libtool-2.2.6b.tar.gz
        cd ${_cdir}/libtool-2.2.6b
        ./configure
        make && sudo make install
        rm -rf "${_cdir}/libtool-2.2.6b"
        cd ${_cdir}
    fi
}


function prep_install()
{
    # check supported os and verno
    osid_list=("centos" "rhel" "ubuntu")

    osid=$(linux_os_id)
    osname=$(linux_os_alias)
    osver=$(linux_os_verno)

    major_ver=$(verno_major_id "$osver")
    minor_ver=$(verno_minor_id "$osver")

    # 检查是否支持的操作系统
    ret=$(array_find osid_list "$osid")

    if [ $ret -eq -1 ]; then
        echoerror "os not supported: $osname"
        exit -1
    fi

    # 检查是否支持的操作系统版本
    if [[ "$osid" = "centos" || "$osid" = "rhel" ]]; then
        if [ "$major_ver" -lt 5 ]; then
            echoerror "os verno(<6) not supported: $osname"
            exit -1
        fi

        sudo yum install -y make gcc gcc-c++ tcl kernel-devel libtool zlib-devel openssl-devel readline-devel pcre-devel ncurses-devel libxml2 libxml2-devel cairo cairo-devel

    elif [ "$osid" = "ubuntu" ]; then
        if [ "$major_ver" -lt 14 ]; then
            echoerror "os verno(<14) not supported: $osname"
            exit -1
        fi

        sudo apt-get install -y build-essential autoconf libtool tcl openssl libssl-dev libpcre3 libpcre3-dev zlib1g-dev libreadline-dev libncurses-dev libxml2 libxml2-dev cairo cairo-dev
    fi

    echoinfo "check and install autoconf (http://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz)"
    install_autoconf;

    echoinfo "check and install automake (http://ftp.gnu.org/gnu/automake/automake-1.15.tar.gz)"
    install_automake;

    echoinfo "check and install libtool (http://ftp.gnu.org/gnu/libtool/libtool-2.2.6b.tar.gz)"
    install_libtool;
}


function install_libs()
{
    cd ${_cdir}

    echoinfo "---- build and install <jemalloc-5.1.0> ..."
    tar -zxf ${_cdir}/jemalloc-5.1.0.tar.gz
    cd ${_cdir}/jemalloc-5.1.0/
    ./autogen.sh --with-jemalloc-prefix=je_ --prefix=${INSTALLDIR}
    # rhel: make dist && make && make install
    make && make install
    rm -rf "${_cdir}/jemalloc-5.1.0"
    cd ${_cdir}

    echoinfo "---- build and install <expat-2.1.0> ..."
    tar -zxf ${_cdir}/expat-2.1.0.tgz
    cd ${_cdir}/expat-2.1.0/
    ./configure --prefix=${INSTALLDIR}
    make && make install
    rm -rf "${_cdir}/expat-2.1.0"
    cd ${_cdir}

    echoinfo "---- build and install <log4c-1.2.4>..."
    tar -zxf ${_cdir}/log4c-1.2.4.tgz
    cd ${_cdir}/log4c-1.2.4/
    ./configure --prefix=${INSTALLDIR} --without-expat
    make && make install
    rm -rf "${_cdir}/log4c-1.2.4"
    cd ${_cdir}

    echoinfo "---- build and install <openssl-1.0.2e> ..."
    tar -zxf ${_cdir}/openssl-1.0.2e.tgz
    cd ${_cdir}/openssl-1.0.2e/
    ./config --prefix=${INSTALLDIR}
    make && make install
    rm -rf "${_cdir}/openssl-1.0.2e"
    cd ${_cdir}

    echoinfo "---- build and install <zlib-1.2.11> ..."
    tar -zxf ${_cdir}/zlib-1.2.11.tar.gz
    cd ${_cdir}/zlib-1.2.11/
    ./configure --prefix=${INSTALLDIR}
    make && make install
    rm -rf "${_cdir}/zlib-1.2.11"
    cd ${_cdir}

    echoinfo "---- build and install <libevent-2.1.8-stable> ..."
    tar -zxf ${_cdir}/libevent-2.1.8-stable.tar.gz
    cd ${_cdir}/libevent-2.1.8-stable/
    ./configure --prefix=${INSTALLDIR}
    make && make install
    rm -rf "${_cdir}/libevent-2.1.8-stable"
    cd ${_cdir}

    echoinfo "---- build and install <lua-5.3.5r> http://troubleshooters.com/codecorn/lua/lua_c_calls_lua.htm ..."
    tar -zxf ${_cdir}/lua-5.3.5.tar.gz
    cd ${_cdir}/lua-5.3.5/
    make linux test && make install INSTALL_TOP=${INSTALLDIR}
    rm -rf "${_cdir}/lua-5.3.5"
    cd ${_cdir}

    echoinfo "---- build and install <libiconv-1.15> ..."
    tar -zxf ${_cdir}/libiconv-1.15.tar.gz
    cd ${_cdir}/libiconv-1.15/
    ./configure --prefix=${INSTALLDIR}
    make && make install
    rm -rf "${_cdir}/libiconv-1.15"
    cd ${_cdir}

    echoinfo "---- build and install <inotify-tools-3.20.4> ..."
    tar -zxf ${_cdir}/inotify-tools-3.20.4.tar.gz
    cd ${_cdir}/inotify-tools-3.20.4/
    ./configure --prefix=${INSTALLDIR}
    make && make install
    rm -rf "${_cdir}/inotify-tools-3.20.4"
    cd ${_cdir}

    echoinfo "---- build and install <librdkafka-master> https://github.com/edenhill/librdkafka ..."
    tar -zxf ${_cdir}/librdkafka-master.tar.gz
    cd ${_cdir}/librdkafka-master/
    ./configure --prefix=${INSTALLDIR}
    make && make install
    rm -rf "${_cdir}/librdkafka-master"
    cd ${_cdir}

    echoinfo "---- build and install <hiredis-1.0.1> ..."
    tar -zxf ${_cdir}/hiredis-1.0.1.tar.gz
    cd ${_cdir}/hiredis-1.0.1/
    make PREFIX=${INSTALLDIR} install
    rm -rf "${_cdir}/hiredis-1.0.1"
    cd ${_cdir}

    echoinfo "---- build and install <igraph-0.7.1> http://igraph.org/nightly/get/c/igraph-0.7.1.tar.gz"
    tar -zxf ${_cdir}/igraph-0.7.1.tar.gz
    cd ${_cdir}/igraph-0.7.1/
    ./configure --prefix=${INSTALLDIR}
    make && make install
    rm -rf "${_cdir}/igraph-0.7.1"
    cd ${_cdir}

    echoinfo "---- build and install <json-c-20180305> https://github.com/json-c/json-c"
    tar -zxf ${_cdir}/json-c-20180305.tar.gz
    cd ${_cdir}/json-c-20180305/
    sh autogen.sh
    ./configure --prefix=${INSTALLDIR} --enable-threading
    make && make install
    make USE_VALGRIND=0 check
    rm -rf "${_cdir}/json-c-20180305"
    cd ${_cdir}

    echoinfo "all packages successfully installed at: ${INSTALLDIR}"
}


################################# main ################################
if [ $# -eq 0 ]; then usage; exit 1; fi

# parse options:
RET=`getopt -o Vh --long version,help,prepare,install\
    -n ' * ERROR' -- "$@"`

if [ $? != 0 ] ; then echoerror "$_name exited with doing nothing." >&2 ; exit 1 ; fi

# Note the quotes around $RET: they are essential!
eval set -- "$RET"

# set option values
while true; do
    case "$1" in
        -V | --version) echoinfo "$(basename $0) -- version: $_ver"; exit 1;;
        -h | --help ) usage; exit 1;;

        --prepare ) prep_install; exit 0;;
	--install ) install_libs; exit 0;;

        -- ) shift; break ;;
        * ) break ;;
    esac
done
