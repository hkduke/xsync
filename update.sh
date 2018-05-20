#!/bin/bash
#
# filename: update.sh
#    更新源文件的版本
#
# author:
#   master@pepstack.com
#
# create: 2018-05-20
# update: 2018-05-20
#
# find files modified in last 1 miniute
#   $ find ${path} -type f -mmin 1 -exec ls -l {} \;
########################################################################
# NOTE: readlink -f not support by MaxOS-X
_file=$(readlink -f $0)
_cdir=$(dirname $_file)
_name=$(basename $_file)


$_cdir/pysrc/updt.py --path="$_cdir/src" --filter="c,cpp" --author="master@pepstack.com" --recursive
