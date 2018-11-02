#!/bin/bash
#
#   dd 测试 xsync-client with inotifytools 脚本
#
# 2018-11-02
#
########################################################################
# 根据需要修改下面的值

maxfile=10000

interval_second=0.001


##################################################
# 下面的值不要更改 !

outprefix="/tmp/xclient/testlogs"

timeprefix=$(date +%s)

testoutdir="$outprefix/$timeprefix"

mkdir -p "$testoutdir"

logfile="$outprefix/ddtest-log.$timeprefix"

echo "start xclient dd test ..."

echo `date` > "$logfile"

i=0
while :
do
    ((i++))

    if [ "$i" -gt "$maxfile" ]; then
        break
    fi

    randno=`head -1 /dev/urandom |od  -N 2 | head -1|awk '{print $2}'`

    randfile="$testoutdir/ddtest.out.$randno"

    dd if=/dev/zero of="$randfile" bs=1k count=1

    sleep "$interval_second"

    echo "$randfile" >> "$logfile"
done

echo `date` >> "$logfile"

echo "xclient dd test output  : $testoutdir"
echo "xclient dd test log file: $logfile"
echo "xclient dd test end !"
