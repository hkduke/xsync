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

PROJBINDIR=$(dirname $_cdir)/bin
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
# 当前安装的 lib 列表

LIB_JEMALLOC="jemalloc-5.1.0"
LIB_EXPAT="expat-2.1.0"
LIB_LOG4C="log4c-1.2.4"
LIB_OPENSSL="openssl-1.0.2e"
LIB_ZLIB="zlib-1.2.11"
LIB_LIBEVENT="libevent-2.1.8-stable"
LIB_LUA="lua-5.3.5"
LIB_LUA_CJSON="lua-cjson-2.1.1"
LIB_LIBICONV="libiconv-1.15"
LIB_INOTIFY_TOOLS="inotify-tools-3.20.4"
LIB_LIBRDKAFKA="librdkafka-master"
LIB_HIREDIS="hiredis-1.0.1"
LIB_IGRAPH="igraph-0.7.1"
LIB_JSON_C="json-c-20180305"


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
    elif [ "$appverno" \< "$pkgver" ]; then
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
    elif [ "$appverno" \< "$pkgver" ]; then
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

        sudo yum install -y \
			bc \
			lsof \
			make \
			gcc \
			gcc-c++ \
			tcl \
			tcsh \
			kernel-devel \
			libtool \
			zlib-devel \
			openssl-devel \
			readline-devel \
			pcre-devel \
			ncurses-devel \
			libxml2 \
			libxml2-devel \
			cairo \
			cairo-devel

    elif [ "$osid" = "ubuntu" ]; then
        if [ "$major_ver" -lt 14 ]; then
            echoerror "os verno(<14) not supported: $osname"
            exit -1
        fi

        sudo apt-get install -y \
			build-essential \
			autoconf \
			libtool \
			tcl \
			openssl \
			libssl-dev \
			libpcre3 \
			libpcre3-dev \
			zlib1g-dev \
			libreadline-dev \
			libncurses-dev \
			libxml2 \
			libxml2-dev \
			libcairo2 \
			libcairo2-dev
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

    echoinfo "---- build and install <$LIB_JEMALLOC> ..."
    tar -zxf ${_cdir}/$LIB_JEMALLOC.tar.gz
    cd ${_cdir}/$LIB_JEMALLOC/
    ./autogen.sh --with-jemalloc-prefix=je_ --prefix=${INSTALLDIR}
    # rhel: make dist && make && make install
    make && make install
    rm -rf "${_cdir}/$LIB_JEMALLOC"
    cd ${_cdir}

    echoinfo "---- build and install <$LIB_EXPAT> ..."
    tar -zxf ${_cdir}/$LIB_EXPAT.tgz
    cd ${_cdir}/$LIB_EXPAT/
    ./configure --prefix=${INSTALLDIR}
    make && make install
    rm -rf "${_cdir}/$LIB_EXPAT"
    cd ${_cdir}

    echoinfo "---- build and install <$LIB_LOG4C>..."
    tar -zxf ${_cdir}/$LIB_LOG4C.tgz
    cd ${_cdir}/$LIB_LOG4C/
    ./configure --prefix=${INSTALLDIR} --without-expat
    make && make install
    rm -rf "${_cdir}/$LIB_LOG4C"
    cd ${_cdir}

    echoinfo "---- build and install <$LIB_OPENSSL> ..."
    tar -zxf ${_cdir}/$LIB_OPENSSL.tgz
    cd ${_cdir}/$LIB_OPENSSL/
    ./config --prefix=${INSTALLDIR}
    make && make install
    rm -rf "${_cdir}/$LIB_OPENSSL"
    cd ${_cdir}

    echoinfo "---- build and install <$LIB_ZLIB> ..."
    tar -zxf ${_cdir}/$LIB_ZLIB.tar.gz
    cd ${_cdir}/$LIB_ZLIB/
    ./configure --prefix=${INSTALLDIR}
    make && make install
    rm -rf "${_cdir}/$LIB_ZLIB"
    cd ${_cdir}

    echoinfo "---- build and install <$LIB_LIBEVENT> ..."
    tar -zxf ${_cdir}/$LIB_LIBEVENT.tar.gz
    cd ${_cdir}/$LIB_LIBEVENT/
    ./configure --prefix=${INSTALLDIR}
    make && make install
    rm -rf "${_cdir}/$LIB_LIBEVENT"
    cd ${_cdir}

    echoinfo "---- build and install <$LIB_LUA> http://troubleshooters.com/codecorn/lua/lua_c_calls_lua.htm ..."
    tar -zxf ${_cdir}/$LIB_LUA.tar.gz
    cd ${_cdir}/$LIB_LUA/
    make linux test && make install INSTALL_TOP=${INSTALLDIR}
    rm -rf "${_cdir}/$LIB_LUA"
    cd ${_cdir}

    echoinfo "---- build and install <$LIB_LUA_CJSON> https://www.kyne.com.au/~mark/software/lua-cjson-manual.html"
    tar -zxf ${_cdir}/$LIB_LUA_CJSON.tar.gz
    cd ${_cdir}/$LIB_LUA_CJSON/
    make -e PREFIX=${INSTALLDIR}
    cp ${_cdir}/$LIB_LUA_CJSON/cjson.so ${INSTALLDIR}/lib/lua/5.3/
    rm -rf "${_cdir}/$LIB_LUA_CJSON"
    cd ${_cdir}

    echoinfo "---- build and install <$LIB_LIBICONV> ..."
    tar -zxf ${_cdir}/$LIB_LIBICONV.tar.gz
    cd ${_cdir}/$LIB_LIBICONV/
    ./configure --prefix=${INSTALLDIR}
    make && make install
    rm -rf "${_cdir}/$LIB_LIBICONV"
    cd ${_cdir}

    echoinfo "---- build and install <$LIB_INOTIFY_TOOLS> ..."
    tar -zxf ${_cdir}/$LIB_INOTIFY_TOOLS.tar.gz
    cd ${_cdir}/$LIB_INOTIFY_TOOLS/
    ./configure --prefix=${INSTALLDIR}
    make && make install
    rm -rf "${_cdir}/$LIB_INOTIFY_TOOLS"
    cd ${_cdir}

    echoinfo "---- build and install <$LIB_LIBRDKAFKA> https://github.com/edenhill/librdkafka ..."
    tar -zxf ${_cdir}/$LIB_LIBRDKAFKA.tar.gz
    cd ${_cdir}/$LIB_LIBRDKAFKA/
    ./configure --prefix=${INSTALLDIR}
    make && make install
    rm -rf "${_cdir}/$LIB_LIBRDKAFKA"
    cd ${_cdir}

    echoinfo "---- build and install <$LIB_HIREDIS> ..."
    tar -zxf ${_cdir}/$LIB_HIREDIS.tar.gz
    cd ${_cdir}/$LIB_HIREDIS/
    make PREFIX=${INSTALLDIR} install
    rm -rf "${_cdir}/$LIB_HIREDIS"
    cd ${_cdir}

    echoinfo "---- build and install <$LIB_IGRAPH> http://igraph.org/nightly/get/c/$LIB_IGRAPH.tar.gz"
    tar -zxf ${_cdir}/$LIB_IGRAPH.tar.gz
    cd ${_cdir}/$LIB_IGRAPH/
    ./configure --prefix=${INSTALLDIR}
    make && make install
    rm -rf "${_cdir}/$LIB_IGRAPH"
    cd ${_cdir}

    echoinfo "---- build and install <$LIB_JSON_C> https://github.com/json-c/json-c"
    tar -zxf ${_cdir}/$LIB_JSON_C.tar.gz
    cd ${_cdir}/$LIB_JSON_C/
    sh autogen.sh
    ./configure --prefix=${INSTALLDIR} --enable-threading
    make && make install
    make USE_VALGRIND=0 check
    rm -rf "${_cdir}/$LIB_JSON_C"
    cd ${_cdir}

    echoinfo "---- update lua: cp ${INSTALLDIR}/bin/lua => ${PROJBINDIR}"
    rm -f ${PROJBINDIR}/lua
    rm -f ${PROJBINDIR}/luac
    cp ${INSTALLDIR}/bin/lua ${PROJBINDIR}/
    cp ${INSTALLDIR}/bin/luac ${PROJBINDIR}/
    cd ${_cdir}

    echoinfo "---- below packages successfully installed at: ${INSTALLDIR}"
    echoinfo "    $LIB_JEMALLOC"
    echoinfo "    $LIB_EXPAT"
    echoinfo "    $LIB_LOG4C"
    echoinfo "    $LIB_OPENSSL"
    echoinfo "    $LIB_ZLIB"
    echoinfo "    $LIB_LIBEVENT"
    echoinfo "    $LIB_LUA"
    echoinfo "    $LIB_LUA_CJSON"
    echoinfo "    $LIB_LIBICONV"
    echoinfo "    $LIB_INOTIFY_TOOLS"
    echoinfo "    $LIB_LIBRDKAFKA"
    echoinfo "    $LIB_HIREDIS"
    echoinfo "    $LIB_IGRAPH"
    echoinfo "    $LIB_JSON_C"
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
