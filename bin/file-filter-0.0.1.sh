#!/bin/bash
#
# @file: file-filter.h
#   监控文件过滤脚本.
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

###########################################################
# 目录过滤函数!!
#
function do_file_filter() {
    local wp="$1"


    echo "$ACCEPT"
}

###########################################################
# 目录的绝对全路径以第一个参数传递进来
#
pathfile="$1"

echoinfo "filter file: '"$pathfile"'"

# 调用过滤函数
do_file_filter "$pathfile"
