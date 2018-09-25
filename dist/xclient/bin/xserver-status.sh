#!/bin/bash
#
# @file: xserver-status.h
#   查看 xsync-server 进程状态
#
# @author: master@pepstack.com
#
# @version: 0.0.1
#
# @create: 2018-09-13
#
# @update:
#
########################################################################
# NOTE: readlink -f not support by MaxOS-X
_file=$(readlink -f $0)

_cdir=$(dirname $_file)
_name=$(basename $_file)

. $_cdir/common.sh

procname="xsync-server"

echoinfo "$procname status:"

pid=$(pids_of_proc "$procname")

if [ -z "$pid" ]; then
    echoerror "process "$procname" not found"
else
    openfds=$(openfd_of_pid "$pid")
    echoinfo "pid=$pid, openfds=$openfds"
    echo "----------------------------------------------------------------------"
    echo " pid    pcpu  res  virt  stime  user    uid    comm              args"
    echo "----------------------------------------------------------------------"
    ps -e -o 'pid,pcpu,rsz,vsz,stime,user,uid,comm,args' | grep "$procname" | grep -v grep | sort -nrk5
    echoinfo "ok"
fi