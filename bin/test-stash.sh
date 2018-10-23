#!/bin/bash
#
########################################################################
maxfileno=1000

interval_second=0.01

stash_prefix="/tmp/xclient/test-log/stash"

i=0
j=0
while :
do
    ((i++))
    ((j++))

    if [ "$j" -lt "$maxfileno" ]; then
        mkdir -p "${stash_prefix}/$j"
    fi

    echo "hello $i" > "${stash_prefix}/$i/hello_kitty.log.$i"

    if [ "$i" -gt "$maxfileno" ]; then
        i=0
    fi

    sleep "$interval_second"
done
