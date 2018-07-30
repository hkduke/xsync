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
  --host=HOST                 指定集群的IP地址. (hostname 默认)
  --ports=PORTS               指定集群的服务节点端口列表. (如: 7001,7002,7003)
  --maxmemory=MEMGB           指定集群的每个服务节点内存上限(gb). (2g 默认)
  --requirepass=PASS          指定集群的密码. ('test' 默认)
  --ignore-test               忽略测试. 默认要测试.
  --replica=NUM               指定集群数据的复制数. (1 默认)

Examples:

  $ sudo ${_name} --prefix=/opt/redis-cluster \
--cluster-id=test \
--hosts="localhost:7001,localhost:7002,localhost:7003" \
--replica=2 \
--maxmemory=10g \
--requirepass=RedisP@ss \
--ignore-test

报告错误: 350137278@qq.com
EOT
}   # ----------  end of function usage  ----------

if [ $# -eq 0 ]; then usage; exit 1; fi

# 开始安装 redis-cluster
cluster_id="test"
replica="1"
prefix=
host="$(uname -n)"
ports=
maxmemory="2g"
timeoutms=12000
requirepass="test"
ignore_test=false

# parse options:
RET=`getopt -o Vh --long version,help,ignore-test,prefix:,cluster-id:,host:,ports:,replica:,maxmemory:,requirepass:,\
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
        --host ) host="$2"; shift 2 ;;
        --ports ) ports="$2"; shift 2 ;;
        --replica ) replica="$2"; shift 2 ;;
        --maxmemory ) replica="$2"; shift 2 ;;
        --requirepass ) replica="$2"; shift 2 ;;        
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

REDIS_BINDIR="$REDIS_PREFIX/redis-4.0.10/bin"

CLUSTER_ROOT=$prefix/$cluster_id
HIREDIS_LIBS=$prefix/libs


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

echoinfo "redis-4.0.10  will be installed at: $REDIS_PREFIX"
echoinfo "ruby-2.3.1    will be installed at: $RUBY_PREFIX"
echoinfo "redis-cluster will be installed at: $CLUSTER_ROOT"

OLD_IFS="$IFS"
IFS=","
ps=($ports)
IFS="$OLD_IFS"

# get length of an array
arrlen=${#ps[@]}

# use for loop read all hosts
hostports=()
portslist=()

for (( i=0; i<${arrlen}; i++ ));
do
    port=$(echo ${ps[i]} | sed 's/^\s+//' | sed 's/\s+$//')

    if ((i == 0)); then
        portslist[i]="$port"
        hostports[i]="$host:$port"
    else
        ret=$(array_find portslist "$port")

        if [ "$ret" == "-1" ]; then
            portslist[i]="$port"
            hostports[i]="$host:$port"
        else
            echoerror "duplicated port: $port"
            exit -1
        fi
    fi
done

$_cdir/install_redis.sh --prefix=$REDIS_PREFIX --package=$_cdir/redis-4.0.10.tar.gz
$_cdir/install_ruby.sh --prefix=$RUBY_PREFIX --package=$_cdir/ruby-2.3.1.tar.gz --install-gem=$_cdir/redis-4.0.1.gem

start_sh="$CLUSTER_ROOT/start.sh"

echo "#!/bin/bash" > "$start_sh"
echo "# @created: $(now_datetime)" >> "$start_sh"
echo "###########################################################" >> "$start_sh"

arrlen=${#portslist[@]}
for (( i=0; i<${arrlen}; i++ ));
do
    PORT="${portslist[i]}"

    echoinfo "generate config: "$CLUSTER_ROOT/redis-$PORT.conf

    prefix2=$(echo "$prefix" | sed 's/\//\\\//g')

    cat "$redis_port_conf" | \
        sed 's/TIMEOUTMS/'"$timeoutms"'/g' | \
        sed 's/MEMGB/'"$maxmemory"'/g' | \
        sed 's/PASS/'"$requirepass"'/g' | \
        sed 's/HOST/'"$host"'/g' | \
        sed 's/PORT/'"$PORT"'/g' | \
        sed 's/CLUSTERID/'"$cluster_id"'/g' | \
        sed 's/PREFIX/'"$prefix2"'/g' > "$CLUSTER_ROOT/redis-$PORT.conf"

    echo "$REDIS_BINDIR/redis-server $CLUSTER_ROOT/redis-$PORT.conf" >> "$start_sh"
done

echo "##### $REDIS_BINDIR/redis-trib.rb create --replicas $replica ${hostports[@]}" >> "$start_sh"

chmod +x "$start_sh"
