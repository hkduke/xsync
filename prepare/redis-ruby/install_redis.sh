#!/bin/bash
#
# @file:
#
#   Linux 从源码安装 redis
#
# 用法:
#   $ sudo ./install_redis.sh --prefix=/usr/local/redis --package=./repo/redis-ruby/redis-4.0.10.tar.gz
#
#   $ sudo ./install_redis.sh --prefix=/usr/local/redis --package=ftp://192.168.124.211/pub/hacl-repo/redis-ruby/redis-4.0.10.tar.gz
#
#   http://makaidong.com/bcphp/3943_9044049.html
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
  Linux 从源码安装 redis.

Options:
  -h, --help                  显示帮助
  -V, --version               显示版本

  --prefix=PATH               指定安装的路径，如：/usr/local/redis
  --package=TARBALL           指定安装的tar包，如：/path/to/redis-4.0.10.tar.gz
  --ignore-test               忽略测试. 默认要测试

返回值:
  0      成功
  非 0   失败

例子:

  $ sudo ${_name} \
--prefix=/usr/local/redis \
--package=./repo/redis-ruby/redis-4.0.10.tar.gz

  $ sudo ${_name} \
--prefix=/usr/local/redis \
--package=ftp://192.168.124.211/pub/hacl-repo/redis-ruby/redis-4.0.10.tar.gz \
--ignore-test

报告错误: 350137278@qq.com
EOT
}   # ----------  end of function usage  ----------

if [ $# -eq 0 ]; then usage; exit 1; fi


function install_redis() {
    local redisprefix=$1
    local pkgfile=$2
    local igntest=$3

    local pkgdir=$(dirname $pkgfile)
    local pkgname=$(basename $pkgfile)

    local profile=/etc/profile.d/redis-env.sh

    # 临时解压目录
    local tmpdir=$(mktemp -d $pkgdir/temp.XXXX)

    echoinfo "解压 $pkgfile 到: $tmpdir"

    tar -zxf $pkgfile -C $tmpdir

    local redname=
    local redfile=

    local filelist=`ls $tmpdir`
    for filename in $filelist; do
        if [ -d $tmpdir"/"$filename ]; then
            redname=$filename
            redfile=$tmpdir"/"$redname
            break
        fi
    done

    if [ -z "$redfile" ]; then
        echoerror "redis untar directory not found!"
        exit -1
    fi

    local redishome=$redisprefix"/"$redname
    
    echoinfo "源码位置: "$redfile
    echoinfo "安装目录: "$redishome

    if [ -d $redishome ]; then
        echowarn "覆盖已安装的 redis：$redishome"
    fi

    local olddir=`pwd`

    # 进入源码目录开始编译    
    cd $redfile

    # 安装 gcc (for make), tcl (for make test)
    yum install -y gcc gcc-c++ tcl

    make PREFIX=$redishome install

    if [ "$igntest" = false ]; then
        make test
    fi

    mkdir -p $redishome/conf

	cp $redfile/redis.conf $redishome/conf/
    cp $redfile/sentinel.conf $redishome/conf/
    cp $redfile/src/redis-trib.rb $redishome/bin/

    # 进入 hiredis 源码目录开始编译    
    cd $redfile/deps/hiredis/
    make PREFIX=$redishome install

    echoinfo "删除临时目录：$tmpdir"
    rm -rf $tmpdir

    if [ ! -d $redishome ]; then
        echoerror "安装 redis 失败. 未发现: $redishome"
        exit -1
    fi

    echoinfo "安装成功。开始配置 redis 环境：$profile"

    # 增加行
    echo "export REDIS_HOME=$redishome" > $profile
    echo "export PATH=\$PATH:\$REDIS_HOME/bin" >> $profile

    echoinfo "更新配置：$profile"
    source $profile

    ln -sf $redishome/bin/redis-trib.rb /usr/local/bin/redis-trib.rb
    ln -sf $redishome/bin/redis-server /usr/local/bin/redis-server
    ln -sf $redishome/bin/redis-cli /usr/local/bin/redis-cli

    local ret=`redis-server --version`

    echo $ret
}


# redis 高性能配置
#
function config_redis_perf() {
    echoinfo "[1] 更改监听队列大小(默认 128 太小了)"
   	sysctl -w net.core.somaxconn=2048

    # 永久设置 (重启后保持), 执行命令
    echo "net.core.somaxconn=2048" > /etc/sysctl.d/net_core_somaxconn.conf

    # 使生效
    sysctl -p

    echoinfo "[2] 内核针对内存分配的策略: 1 允许分配所有的物理内存"

    # overcommit_memory 指定了内核针对内存分配的策略，其值可以是0、1、2。
    # 0， 表示内核将检查是否有足够的可用内存供应用进程使用；
    #     如果有足够的可用内存，内存申请允许；否则，内存申请失败，并把错误返回给应用进程。
    # 1， 表示内核允许分配所有的物理内存，而不管当前的内存状态如何。
    # 2， 表示内核允许分配超过所有物理内存和交换空间总和的内存
    sysctl -w vm.overcommit_memory=1

    # 永久设置设置 (重启后保持), 执行命令
    echo "vm.overcommit_memory=1" > /etc/sysctl.d/vm_overcommit_memory.conf

    # 使生效
    sysctl -p

    echoinfo "禁用透明巨页内存配置以提高性能"
    # 查看当前 transparent_hugepage 状态:
	cat /sys/kernel/mm/transparent_hugepage/enabled

    #[always] madvise never
    #
	#参数说明:
    #
    #	never 关闭，不使用透明内存
    #	alway 尽量使用透明内存，扫描内存，有512个 4k页面可以整合，就整合成一个2M的页面
    #	madvise 避免改变内存占用

    # 禁用透明巨页内存和大页面:
    echo never > /sys/kernel/mm/transparent_hugepage/enabled
    echo never > /sys/kernel/mm/transparent_hugepage/defrag

	# 永久禁用, 将命令加入到 rc.local 中:
   	ret=$(cat /etc/rc.local | grep "echo never > /sys/kernel/mm/transparent_hugepage/enabled")
    if [ -z "$ret" ]; then
        echo "echo never > /sys/kernel/mm/transparent_hugepage/enabled" >> /etc/rc.local
    fi

    ret=$(cat /etc/rc.local | grep "echo never > /sys/kernel/mm/transparent_hugepage/defrag")
    if [ -z "$ret" ]; then
        echo "echo never > /sys/kernel/mm/transparent_hugepage/defrag" >> /etc/rc.local
    fi

    echoinfo "/sys/kernel/mm/transparent_hugepage/enabled:"
    cat /sys/kernel/mm/transparent_hugepage/enabled
    
    echoinfo "/sys/kernel/mm/transparent_hugepage/defrag:"
    cat /sys/kernel/mm/transparent_hugepage/defrag

    sysctl net.core.somaxconn

    sysctl vm.overcommit_memory
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
RET=`getopt -o Vh --long version,help,ignore-test,prefix:,package:,\
    -n ' * ERROR' -- "$@"`

if [ $? != 0 ] ; then echoerror "$_name exited with doing nothing." >&2 ; exit 1 ; fi

# Note the quotes around $RET: they are essential!
eval set -- "$RET"

ignore_test=false

# set option values
while true; do
    case "$1" in
        -V | --version) echoinfo "$(basename $0) -- version: $_ver"; exit 1;;
        -h | --help ) usage; exit 1;;

        --prefix ) prefix="$2"; shift 2 ;;
        --package ) package="$2"; shift 2 ;;
        --ignore-test ) ignore_test=true; echowarn "ignore make test"; shift;;

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

# 安装, TODO: 判断返回值
install_redis "$prefix" "$abs_package" "$ignore_test"

# 删除下载目录
if [ ! -z "$abs_package" ]; then
    deltmpdir=$(dirname $abs_package)
    echowarn "删除临时目录: $deltmpdir"
    rm -rf $deltmpdir
fi

# TODO: 验证安装是否成功

config_redis_perf

redis-server --version
redis-cli --version

exit 0
