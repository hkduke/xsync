#!/bin/bash
#
########################################################################
maxfile=10000

interval_second=0.001

outdir="/tmp/xclient/test-log"
logfile="/tmp/xclient/dd_test.log"

echo "test xclient with dd test start ..."

echo `date` > "$logfile"

i=0
while :
do
    ((i++))

    if [ "$i" -gt "$maxfile" ]; then
        break
    fi

    randno=`head -1 /dev/urandom |od  -N 2 | head -1|awk '{print $2}'`

    randfile="$outdir/dd_test.out.$randno"

    dd if=/dev/zero of="$randfile" bs=1k count=1

    sleep "$interval_second"

    echo "$randfile" >> "$logfile"
done

echo `date` >> "$logfile"

echo "test xclient with dd test end."