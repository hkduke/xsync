#!/bin/bash
#
########################################################################
maxfileno=1000

interval_second=0.1

testlog_prefix="/tmp/xclient-watch-test-stash/test-log"

i=0
while :
do
    ((i++))

    echo "hello $i" > "$testlog_prefix"."$i"

    if [ "$i" -gt "$maxfileno" ]; then
        i=0
    fi

    sleep "$interval_second"
done