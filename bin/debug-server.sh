#!/bin/bash
#
# File : install-devel-libs.sh
#
#   install devel lib for building both xsync-client and xsync-server
#
# init created: 2018-01-19
# last updated: 2018-02-25
#
########################################################################
# NOTE: readlink -f not support by MaxOS-X
_file=$(readlink -f $0)

_cdir=$(dirname $_file)
_name=$(basename $_file)

TARGETDIR=$(dirname $_cdir)/target

$TARGETDIR/xsync-server --redis-cluster='127.0.0.1:7001,127.0.0.1:7002,127.0.0.1:7003,127.0.0.1:7004,127.0.0.1:7005,127.0.0.1:7006,127.0.0.1:7007,127.0.0.1:7008,127.0.0.1:7009' \
    --redis-auth='PepSt@ck'
