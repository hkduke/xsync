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
#######################################
replica=1
prepare_dir=$(dirname $_cdir)
project_dir=$(dirname $prepare_dir)

cluster_dir=$project_dir/redis-cluster
hiredis_dir=$project_dir/libs

redis_port_conf="$_cdir/redis-PORT.conf"

cluster_hosts="localhost:17001,localhost:17002,localhost:17003"

echoinfo "redis-4.0.10 will be installed at: /usr/local/redis/"
echoinfo "ruby-2.3.1   will be installed at: /usr/local/ruby/"

$_cdir/install_redis.sh --prefix=/usr/local/redis --package=$_cdir/redis-4.0.10.tar.gz
$_cdir/install_ruby.sh --prefix=/usr/local/ruby --package=$_cdir/ruby-2.3.1.tar.gz --install-gem=$_cdir/redis-4.0.1.gem

redis_bindir="/usr/local/redis/redis-4.0.10/bin"


mkdir -p $cluster_dir/{17001,17002,17003,run,log}

# 修改 $redis_port_conf 为如下配置文件:
# $redis_bindir/redis-server $cluster_dir/conf/redis-17001.conf
# $redis_bindir/redis-server $cluster_dir/conf/redis-17002.conf
# $redis_bindir/redis-server $cluster_dir/conf/redis-17003.conf

# $redis_bindir/redis-trib.rb create --replicas $replica localhost:17001 localhost:17002 localhost:17003
