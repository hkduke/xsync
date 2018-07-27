#!/bin/bash
#
# @file:
#
#   Linux 从源码安装 ruby
#
# 用法:
#   $ sudo ./install_ruby.sh --prefix=/usr/local/lib/ruby-2.3.1 --package=../packages/redis-ruby/ruby-2.3.1.tar.gz --install-gem=../packages/redis-ruby/redis-4.0.1.gem
#
#   $ sudo ./install_ruby.sh --prefix=/usr/local/lib/ruby-2.3.1 --package=ftp://192.168.124.211/pub/hacl-repo/redis-ruby/ruby-2.3.1.tar.gz --install-gem=ftp://192.168.124.211/pub/hacl-repo/redis-ruby/redis-4.0.1.gem
#
#   wget https://cache.ruby-lang.org/pub/ruby/2.3/ruby-2.3.1.tar.gz
#   wget http://rubygems.org/downloads/redis-4.0.1.gem
#
#   http://www.cnblogs.com/xuliangxing/p/7146868.html
#
# @author: $author$
#
# @create: $create$
#
# @update:
#
#######################################################################
# will cause error on macosx
_file=$(readlink -f $0)

_cdir=$(dirname $_file)
_name=$(basename $_file)
_ver=0.0.1

. $_cdir/common.sh

. $_cdir/download.sh

# Set characters encodeing
#   LANG=en_US.UTF-8;export LANG
LANG=zh_CN.UTF-8;export LANG

# Treat unset variables as an error
set -o nounset

#######################################################################
#-----------------------------------------------------------------------
# FUNCTION: usage
# DESCRIPTION:  Display usage information.
#-----------------------------------------------------------------------
usage() {
    cat << EOT

Usage :  ${_name} --prefix=PATH --package=TARBALL
  Linux 从源码安装 ruby.

Options:
  -h, --help                  显示帮助
  -V, --version               显示版本

  --prefix=PATH               指定安装的路径，如：/usr/local/lib/ruby-2.3.1
  --package=TARBALL           指定安装的tar包，如：/path/to/ruby-2.3.1.tar.gz
  --install-gem=GEM           指定安装的gem，如：/path/to/redis-4.0.1.gem

返回值:
  0      成功
  非 0   失败

例子:

  $ sudo ${_name} --prefix=/usr/local/ruby \
--package=../packages/redis-ruby/ruby-2.3.1.tar.gz \
--install-gem=../packages/redis-ruby/redis-4.0.1.gem

  $ sudo ${_name} \
--prefix=/usr/local/ruby \
--package=ftp://192.168.124.211/pub/hacl-repo/redis-ruby/ruby-2.3.1.tar.gz \
--install-gem=ftp://192.168.124.211/pub/hacl-repo/redis-ruby/redis-4.0.1.gem

报告错误: 350137278@qq.com
EOT
}   # ----------  end of function usage  ----------

if [ $# -eq 0 ]; then usage; exit 1; fi


function install_ruby_gem() {
    local rubyprefix=$1
    local pkgfile=$2
    local gemfile=$3

    local pkgdir=$(dirname $pkgfile)
    local pkgname=$(basename $pkgfile)

    local profile=/etc/profile.d/ruby-env.sh

    # 临时解压目录
    local tmpdir=$(mktemp -d $pkgdir/temp.XXXX)

    echoinfo "解压 $pkgfile 到: $tmpdir"

    tar -zxf $pkgfile -C $tmpdir

    local rbname=
    local rbfile=

    local filelist=`ls $tmpdir`
    for filename in $filelist; do
        if [ -d $tmpdir"/"$filename ]; then
            rbname=$filename
            rbfile=$tmpdir"/"$rbname
            break
        fi
    done

    if [ -z "$rbfile" ]; then
        echoerror "ruby untar directory not found!"
        exit -1
    fi

    local rubyhome=$rubyprefix"/"$rbname
    
    echoinfo "源码位置: "$rbfile
    echoinfo "安装目录: "$rubyhome

    if [ -d $rubyhome ]; then
        echowarn "覆盖已安装的ruby：$rubyhome"
    fi

    local olddir=`pwd`

    # 进入源码目录开始编译    
    cd $rbfile

    # 安装 gcc
    yum install -y gcc

    ./configure --prefix=$rubyhome

    make && make install

    if [ ! -d $rubyhome ]; then
        echoerror "安装 ruby 失败. 未发现: $rubyhome"
        cd $olddir
        echoinfo "删除临时目录：$tmpdir"
        rm -rf $tmpdir
        exit -1
    fi

    echoinfo "安装成功。开始配置 ruby 环境：$profile"

    # 增加行
    echo "export RUBY_HOME=$rubyhome" > $profile
    echo "export PATH=\$PATH:\$RUBY_HOME/bin" >> $profile

    echoinfo "更新配置：$profile"
    source $profile

    # 创建链接
    ln -sf $rubyhome/bin/ruby /usr/local/bin/ruby
    ln -sf $rubyhome/bin/gem /usr/local/bin/gem

    # 安装 zlib-devel
    yum install -y zlib-devel

    # 生成一个Makefile文件    
    cd $rbfile/ext/zlib/
    ruby ./extconf.rb
    export top_srcdir=../.. && make && make install

    # 安装 openssl-devel
    yum install -y openssl-devel

    # 生成一个Makefile文件
    cd $rbfile/ext/openssl/
    ruby ./extconf.rb
    export top_srcdir=../.. && make && make install

    if [ ! -z "$gemfile" ]; then
        echoinfo "安装 gem: $gemfile"
        gem install -l $gemfile
    fi

    cd $olddir
    echoinfo "删除临时目录：$tmpdir"
    rm -rf $tmpdir
        
    gem list

    local ret=`ruby --version`

    echo $ret
}

# TODO: 检查 gcc, make 是否安装
# debain/ubunutu
#   apt install build-essential
#
# redhat 必须安装 zlib-devel openssl-devel:
#   yum install make gcc gcc-c++ kernel-devel zlib-devel openssl-devel

prefix=
package=
gem=

# parse options:
RET=`getopt -o Vh --long version,help,prefix:,package:,install-gem:\
    -n ' * ERROR' -- "$@"`

if [ $? != 0 ] ; then echoerror "$_name exited with doing nothing." >&2 ; exit 1 ; fi

# Note the quotes around $RET: they are essential!
eval set -- "$RET"

# set option values
while true; do
    case "$1" in
        -V | --version) echoinfo "$(basename $0) -- version: $_ver"; exit 1;;
        -h | --help ) usage; exit 1;;

        --prefix ) prefix="$2"; shift 2 ;;
        --package ) package="$2"; shift 2 ;;
        --install-gem ) gem="$2"; shift 2 ;;

        -- ) shift; break ;;
        * ) break ;;
    esac
done

# check user if root
uname=`id -un`
echoinfo "当前登录为：$uname"

# check inputs
if [ -z "$prefix" ]; then
    echoerror "PATH not specified. using: --prefix=PATH"
    exit 1
fi

if [ -z "$package" ]; then
    echoerror "TARBALL not specified. using: --package=TARBALL"
    exit 1
fi

# 路径是否存在
if [ ! -d "$prefix" ]; then
    echowarn "path not found. create: $prefix"
    mkdir -p "$prefix"
fi

abs_package=$(wget_download_pkg "$package")

# 文件是否存在
if [ ! -f "$abs_package" ]; then
    echoerror "package not found: $abs_package"
    exit -1
else
    echoinfo "package: $abs_package"
fi


install_gem=
if [ ! -z "$prefix" ]; then
    abs_gem=$(wget_download_pkg "$gem")
    
    if [ -f "$abs_gem" ]; then
        install_gem=$abs_gem
        echoinfo "install-gem: $install_gem"
    else
        echowarn "gem not existed: $gem"
    fi
fi

# 安装, TODO: 判断返回值
install_ruby_gem "$prefix" "$abs_package" "$install_gem"

# 删除下载目录
if [ ! -z "$abs_package" ]; then
    deltmpdir=$(dirname $abs_package)
    echowarn "删除临时目录: $deltmpdir"
    rm -rf $deltmpdir
fi

# 删除下载目录
if [ ! -z "$install_gem" ]; then
    deltmpdir=$(dirname $install_gem)
    echowarn "删除临时目录: $deltmpdir"
    rm -rf $deltmpdir
fi

# TODO: 验证安装是否成功


exit 0