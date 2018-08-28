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

$TARGETDIR/xsync-client -I
