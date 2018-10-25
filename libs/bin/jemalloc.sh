#!/bin/sh

prefix=/root/Workspace/github.com/pepstack/xsync/libs
exec_prefix=/root/Workspace/github.com/pepstack/xsync/libs
libdir=${exec_prefix}/lib

LD_PRELOAD=${libdir}/libjemalloc.so.2
export LD_PRELOAD
exec "$@"
