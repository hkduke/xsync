#!/bin/bash
#
#   dd 测试 xsync-client with inotifytools 脚本
#
# 2018-11-02
#
########################################################################
maxfile=10000

interval_second=0.001


##################################################
# 下面的值不要更改 !

timeprefix=$(date +%s)

outdir="/tmp/xclient/test-log/$timeprefix"

mkdir -p "$outdir"

logfile="/tmp/xclient/ddtest.log.$timeprefix"

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

    randfile="$outdir/ddtest.out.$randno"

    dd if=/dev/zero of="$randfile" bs=1k count=1

    sleep "$interval_second"

    echo "$randfile" >> "$logfile"
done

echo `date` >> "$logfile"

echo "test xclient with dd test end."