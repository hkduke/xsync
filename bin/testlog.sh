#!/bin/bash
#
########################################################################
_file=$(readlink -f $0)
_cdir=$(dirname $_file)
_name=$(basename $_file)

i=0

while :
do
    ((i++))

    echo "hello $i" > /tmp/stash/test.log."$i"

    if [ "$i" -gt "1200" ]; then
        i=0
    fi

    sleep 0.1
done
