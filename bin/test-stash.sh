#!/bin/bash
#
########################################################################
maxfileno=1000

interval_second=0.01

testlog_prefix="/tmp/xclient-watch-test-stash"

i=0
j=0
while :
do
    ((i++))
    ((j++))

    if [ "$j" -lt "$maxfileno" ]; then
        mkdir -p "/tmp/xclient-watch-test-stash/$j"
    fi

    echo "hello $i" > "$testlog_prefix/$i/hello_kitty.log.$i"

    if [ "$i" -gt "$maxfileno" ]; then
        i=0
    fi

    sleep "$interval_second"
done
