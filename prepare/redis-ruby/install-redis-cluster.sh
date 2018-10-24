#!/bin/bash
#
# File : install-redis-cluster.sh
#
#   install redis cluster for xsync-server only
#
# 在本地 libs 目录安装 redis 并不会破坏服务器上原有的 redis !
#
# @author: $author$
#
# @create: $create$
#
# @update:
#
########################################################################
# NOTE: readlink -f not support by MaxOS-X
_file=$(readlink -f $0)

_cdir=$(dirname $_file)
_name=$(basename $_file)
_ver=0.0.1

. $_cdir/common.sh

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

Usage :  ${_name} [Options]
  在单个 Linux 服务器上安装 redis 集群.

Options:
  -h, --help                  显示帮助
  -V, --version               显示版本

  --prefix=PATH               指定集群所在的路径. (如: /opt/redis-cluster)
  --cluster-id=ID             指定集群的唯一ID名称. ('test' 默认)
  --hostip=IP                 指定集群监听的IP地址. ('127.0.0.1' 默认)
  --ports=PORTS               指定集群的服务节点端口列表. (如: 7001-7009)
  --maxmemory=MEMGB           指定集群的每个服务节点内存上限(gb). (2g 默认)
  --requirepass=PASS          指定集群的每个服务节点密码. (默认无密码)
  --ignore-test               忽略测试. 默认要测试.
  --replica=NUM               指定集群数据的复制数. (1 默认)

Examples:

  $ sudo ${_name} --prefix=/opt/redis-cluster \
--cluster-id=dev \
--hostip="127.0.0.1" \
--ports="7001,7003-7009" \
--replica=2 \
--maxmemory=10g \
--requirepass='test' \
--ignore-test

  $ sudo ${_name} --prefix=/opt/redis-cluster \
--cluster-id=dev \
--hostip="192.168.124.211" \
--ports="7001-7009" \
--replica=1 \
--maxmemory=1g \
--ignore-test

  $ sudo ${_name} --prefix=/opt/redis-cluster \
--cluster-id=dev \
--hostip="127.0.0.1" \
--ports="7001-7009" \
--replica=2 \
--maxmemory=1g \
--ignore-test

报告错误: 350137278@qq.com
EOT
}   # ----------  end of function usage  ----------

if [ $# -eq 0 ]; then usage; exit 1; fi

REDIS_PKG_NAME="redis-5.0.0"


# 开始安装 redis-cluster
cluster_id="test"
replica="1"
prefix=
hostip="127.0.0.1"
ports=
maxmemory="2g"
timeoutms=12000
requirepass=
ignore_test=false


# parse options:
RET=`getopt -o Vh --long version,help,ignore-test,prefix:,cluster-id:,hostip:,ports:,replica:,maxmemory:,requirepass:,\
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
        --cluster-id ) cluster_id="$2"; shift 2 ;;
        --hostip ) host="$2"; shift 2 ;;
        --ports ) ports="$2"; shift 2 ;;
        --replica ) replica="$2"; shift 2 ;;
        --maxmemory ) maxmemory="$2"; shift 2 ;;
        --requirepass ) requirepass="$2"; shift 2 ;;        
        --ignore-test ) ignore_test=true; echowarn "ignore test"; shift;;

        -- ) shift; break ;;
        * ) break ;;
    esac
done

# check user if root
loguser=`id -un`
echoinfo "当前登录为：$loguser"
chk_root

if [ -z "$prefix" ]; then
    echoerror "prefix not given. (use: --prefix=PATH)"
    exit -1
fi

if [ -z "$ports" ]; then
    echoerror "ports not given. (use: --ports=PORTS)"
    exit -1
fi

prepare_dir=$(dirname $_cdir)
project_dir=$(dirname $prepare_dir)

# 配置文件位置
redis_port_conf="$_cdir/redis-PORT.conf"

REDIS_PREFIX=$prefix/redis
RUBY_PREFIX=$prefix/ruby

REDIS_BINDIR="$REDIS_PREFIX/$REDIS_PKG_NAME/bin"

CLUSTER_ROOT=$prefix/$cluster_id
HIREDIS_LIBS=$prefix/libs

echoinfo "$REDIS_PKG_NAME will be installed at: $REDIS_PREFIX"
echoinfo "redis-cluster will be deployed at: $CLUSTER_ROOT"

OLD_IFS="$IFS"
IFS=","
ps=($ports)
IFS="$OLD_IFS"

# get length of an array
arrlen=${#ps[@]}

# use for loop read all hosts
hostports=()
portslist=()
numports=0

for (( i=0; i<${arrlen}; i++ ));
do
    iport=0
    iportarr=()

    subpt=${ps[i]}

    OLD_IFS="$IFS"
    IFS="-"
    subs=($subpt)
    IFS="$OLD_IFS"
    
    sublen=${#subs[@]}

    if [ $sublen -eq 2 ]; then
        startport=${subs[0]}
        endport=${subs[1]}

        for (( portno=$startport; portno<=$endport; portno++ ));
        do
            iportarr[iport]="$portno"
            ((iport++))
        done
    elif [ $sublen -eq 1 ]; then
        iportarr[iport]="$subpt"
        ((iport++))
    else
        echoerror "bad argument for --ports: $subpt"
        exit -1
    fi
    
    for (( j=0; j<$iport; j++ ));
    do
        port=$(echo ${iportarr[j]} | sed 's/^\s+//' | sed 's/\s+$//')

        if ((numports == 0)); then
            portslist[numports]="$port"
            hostports[numports]="$host:$port"

            ((numports++))
        else
            ret=$(array_find portslist "$port")

            if [ "$ret" == "-1" ]; then
                portslist[numports]="$port"
                hostports[numports]="$host:$port"

                ((numports++))
            else
                echoerror "duplicated port: $port"
                exit -1
            fi
        fi
    done
done

if [ $numports -lt 9 ]; then
    echowarn "redis cluster requirs at least 9 nodes. this host has $numports nodes."
fi

if [ $numports -gt 20 ]; then
    echoerror "this host has too many nodes ($numports > 20)."
    exit -1
fi

# 目录必须为空
is_empty_dir "$CLUSTER_ROOT"
ret=$?

if [[ $ret == $DIR_NOT_EXISTED ]]; then
    echoinfo "create cluster path: $CLUSTER_ROOT"
    mkdir -p "$CLUSTER_ROOT/run"
    mkdir -p "$CLUSTER_ROOT/log"
elif [[ $ret == $DIR_NOT_EMPTY ]]; then
    echoerror "cluster path not empty: $CLUSTER_ROOT"
    exit -1
fi

# 开始安装 redis-5.0
#   redis-5.0 不需要安装 ruby
$_cdir/install_redis.sh --prefix=$REDIS_PREFIX --package=$_cdir/"$REDIS_PKG_NAME".tar.gz

start_sh="$CLUSTER_ROOT/start.sh"
echo "#!/bin/bash" > "$start_sh"
echo "# @created: $(now_datetime)" >> "$start_sh"
echo "###########################################################" >> "$start_sh"

cluster_create_sh="$CLUSTER_ROOT/cluster_create.sh"
echo "#!/bin/bash" > "$cluster_create_sh"
echo "# @file: cluster_create.sh" >> "$cluster_create_sh"
echo "#    initially create cluster sample." >> "$cluster_create_sh"
echo "# Usage:" >> "$cluster_create_sh"
echo "#   command to connect to redis-cluster:" >> "$cluster_create_sh"

if [ -z "$requirepass" ]; then
    echo "#    $REDIS_BINDIR/redis-cli -c -h $host -p ${portslist[0]}" >> "$cluster_create_sh"
else
    echo "#    $REDIS_BINDIR/redis-cli -c -h $host -p ${portslist[0]} -a $requirepass" >> "$cluster_create_sh"
fi

echo "# @created: $(now_datetime)" >> "$cluster_create_sh"
echo "###########################################################" >> "$cluster_create_sh"


arrlen=${#portslist[@]}

for (( i=0; i<${arrlen}; i++ ));
do
    PORT="${portslist[i]}"

    echoinfo "generate config: "$CLUSTER_ROOT/redis-$PORT.conf

    prefix2=$(echo "$prefix" | sed 's/\//\\\//g')

    echoinfo "create data dir: $CLUSTER_ROOT/$PORT"
    mkdir "$CLUSTER_ROOT/$PORT"

    if [ -z "$requirepass" ]; then
        cat "$redis_port_conf" | \
            sed 's/TIMEOUTMS/'"$timeoutms"'/g' | \
            sed 's/MEMGB/'"$maxmemory"'/g' | \
            sed 's/^requirepass/#requirepass/g' | \
            sed 's/HOST/'"$host"'/g' | \
            sed 's/PORT/'"$PORT"'/g' | \
            sed 's/CLUSTERID/'"$cluster_id"'/g' | \
            sed 's/PREFIX/'"$prefix2"'/g' > "$CLUSTER_ROOT/redis-$PORT.conf"
    else
        cat "$redis_port_conf" | \
            sed 's/TIMEOUTMS/'"$timeoutms"'/g' | \
            sed 's/MEMGB/'"$maxmemory"'/g' | \
            sed 's/PASS/'"$requirepass"'/g' | \
            sed 's/HOST/'"$host"'/g' | \
            sed 's/PORT/'"$PORT"'/g' | \
            sed 's/CLUSTERID/'"$cluster_id"'/g' | \
            sed 's/PREFIX/'"$prefix2"'/g' > "$CLUSTER_ROOT/redis-$PORT.conf"
    fi

    echo "$REDIS_BINDIR/redis-server $CLUSTER_ROOT/redis-$PORT.conf" >> "$start_sh"
done

echowarn "redis-trib.rb is not longer available! All commands and features belonging to redis-trib.rb have been moved to redis-cli"
echowarn "see: redis-cli --cluster help"

if [ -z "$requirepass" ]; then
    echoinfo "see $cluster_create_sh for how to init create redis-cluster"
    echo "$REDIS_BINDIR/redis-cli --cluster create ${hostports[@]} --cluster-replicas $replica" >> "$cluster_create_sh"
else
    echoinfo "see $cluster_create_sh for how to init create redis-cluster"
    echo "$REDIS_BINDIR/redis-cli -a '"$requirepass"' --cluster create ${hostports[@]} --cluster-replicas $replica" >> "$cluster_create_sh"
fi

chmod +x "$start_sh"
chmod +x "$cluster_create_sh"

# success
exit 0
