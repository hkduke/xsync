#!/bin/bash
#
# @file: path-filter.h
#   监控目录过滤脚本.
#
#      path-filter.h $fullpath $filename
#
#   返回值:
#      100=接受
#        0=拒绝
#       -1=重载配置
#
# @author: master@pepstack.com
#
# @version: 0.0.1
#
# @create: 2018-09-13
#
# @update:
#
#######################################################################
# NOTE: readlink -f not support by MaxOS-X
_file=$(readlink -f $0)

_cdir=$(dirname $_file)
_name=$(basename $_file)

. $_cdir/common.sh

###########################################################
# 以下返回值的定义不可更改, 通知是否接受或拒绝此目录 !!
#
ACCEPT="100"
REJECT="-100"
RELOAD="-1"

###########################################################
# 仅路径(path)过滤: 路径是绝对路径的全路径名, 以 '/' 结尾 !
#
function filter_path() {
    local path="$1"


    echo "$ACCEPT"
}


###########################################################
# 路径(path)+文件名(name)过滤
#
function filter_file() {
    local path="$1"
    local name="$2"


    echo "$ACCEPT"
}


###########################################################
# 参数 1: 全路径(必选)
# 参数 2: 文件名(可选)
#

if [ $# == 1 ]; then
    echoinfo "filter path: '"$1"'"
    filter_path "$1"
elif [ $# == 2 ]; then
    echoinfo "filter file: '"$1""$2"'"
    filter_file "$1" "$2"
fi