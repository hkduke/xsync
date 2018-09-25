#!/bin/bash
#
########################################################################
_file=$(readlink -f $0)
_cdir=$(dirname $_file)
_name=$(basename $_file)

while :
do
    echo "hello" >> /tmp/stash/test.log
    sleep 0.01
done
