#!/bin/bash
#
# @file: event-task.h
#   事件任务回调脚本.
#
#      event-task.h $event $pathfile $serventry
#
#   返回值:
#      1: 继续处理  (OK)
#      0: 忽略处理  (IGN)
#     -1: 存在错误  (ERR)
#
# @author: master@pepstack.com
#
# @version: 0.0.1
#
# @create: 2018-09-20
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
OK="1"
IGN="0"
ERR="-1"

###########################################################
# event:     事件名称
# pathfile: 本地文件全路径名
#
function on_event_task() {
    local event="$1"
    local pathfile="$2"

    # 模拟执行时间 5s
    #sleep 5

    echo "$OK"
}


###########################################################
# event:     事件名称
# pathfile:  本地文件全路径名
# entry:     服务器文件全路径
#
function on_event_task2() {
    local event="$1"
    local pathfile="$2"
    local entry="$3"

    # 模拟执行时间 5s
    sleep 5

    echo "$OK"
}


###########################################################
# 参数 1: 事件名称(必选)
# 参数 2: 本地文件全路径(必选)
# 参数 3: 服务器文件全路径(可选)
#
if [ $# == 2 ]; then
    echoinfo "on_event_task: event=($1) pathfile=($2)"
    on_event_task "$1" "$2"
elif [ $# == 3 ]; then
    echoinfo "on_event_task2: event=($1) pathfile=($2) entry=($3)"
    on_event_task2 "$1" "$2" "$3"
fi
