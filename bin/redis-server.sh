#!/bin/bash
#
# filename : redis-server.sh
#
#   redis-server.sh status|restart|start|stop|forcestop|debug
#
# author:
#   master@pepstack.com
#
# init created: 2018-02-10
# last updated: 2018-02-10
#
# find files modified in last 1 miniute
#   $ find ${path} -type f -mmin 1 -exec ls -l {} \;
########################################################################
# NOTE: readlink -f not support by MaxOS-X
_file=$(readlink -f $0)
_cdir=$(dirname $_file)
_name=$(basename $_file)

exit 0

SERVICE_NAME=redis-server
SERVICE_MODULE=redis-server

SCRIPTS_DIR=$(dirname $_cdir)/redis/bin
SCRIPTS_USER=root


usage() {
    echo "Usage: $_name {debug|start|stop|forcestop|status|restart}"
}

trace() {
    echo "  * scripts dir:  "${SCRIPTS_DIR}
    echo "  * log path:     "${LOGPATH}
    echo "  * log level:    "$1
    echo "  * max records:  "${MAX_RECORDS}
    echo "  * interval:     "${CHECK_INTERVAL}" seconds"
    echo "  * region name:  "${REGION_NAME}
    echo "  * streams:      "${REGION_STREAMS}
    echo "  * shards:       "${STREAM_SHARDS}
    echo "  * stash home:   "${STASH_HOME}
}


debug() {
    echo "Starting ${SERVICE_NAME} as debug mode ..."

    trace 'DEBUG'

    ${SCRIPTS_DIR}/${SERVICE_MODULE} \
        --log-path=${LOGPATH} \
        --log-level=DEBUG \
        --stash-home=${STASH_HOME} \
        --region-name=${REGION_NAME} \
        --region-streams=${REGION_STREAMS} \
        --stream-shards="${STREAM_SHARDS}" \
        --key-id=${KEY_ID} \
        --access-key=${ACCESS_KEY} \
        --interval=${CHECK_INTERVAL} \
        --max-records=${MAX_RECORDS}

    echo "* done."
}


start() {
    echo "Starting ${SERVICE_NAME} as user '${SCRIPTS_USER}' ..."

    c_pid=`ps -ef | grep ${SERVICE_MODULE} | grep -v ' grep ' | awk '{print $2}'`

    OLD_IFS="$IFS"
    IFS="\n"
    pids=($c_pid)
    IFS="$OLD_IFS"

    if [ ${#pids[@]} = 0 ]; then
        trace ${LOGLEVEL}

        nohup ${SCRIPTS_DIR}/${SERVICE_MODULE} \
        --log-path=${LOGPATH} \
        --log-level=${LOGLEVEL} \
        --stash-home=${STASH_HOME} \
        --region-name=${REGION_NAME} \
        --region-streams=${REGION_STREAMS} \
        --stream-shards="${STREAM_SHARDS}" \
        --key-id=${KEY_ID} \
        --access-key=${ACCESS_KEY} \
        --interval=${CHECK_INTERVAL} \
        --max-records=${MAX_RECORDS}>/dev/null 2>&1 &
        echo "* done."
    else
        for pid in ${pids[@]}
        do
            echo "* ${SERVICE_NAME} ($pid) is already running."
        done
    fi
}


stop() {
    echo "Force stopping ${SERVICE_NAME} ..."

    c_pid=`ps -ef | grep ${SERVICE_MODULE} | grep -v ' grep ' | awk '{print $2}'`

    OLD_IFS="$IFS"
    IFS="\n"
    pids=($c_pid)
    IFS="$OLD_IFS"

    if [ ${#pids[@]} = 0 ]; then
        echo "* ${SERVICE_NAME} not running (stopped)"
    else
        for pid in ${pids[@]}
        do
            echo $pid | xargs kill -9
            echo "* ${SERVICE_NAME} ($pid) force stopped."
        done
    fi

    echo "TODO: Clear zombie processed ..."
    c_pid=`ps -ef|grep ${SERVICE_MODULE} | grep -v ' grep '|awk '{print $2,$9}'`

    echo "$c_pid"
}


forcestop() {
    echo "Force stopping ${SERVICE_NAME} ..."

    c_pid=`ps -ef | grep ${SERVICE_MODULE} | grep -v ' grep ' | awk '{print $2}'`

    OLD_IFS="$IFS"
    IFS="\n"
    pids=($c_pid)
    IFS="$OLD_IFS"

    if [ ${#pids[@]} = 0 ]; then
        echo "* ${SERVICE_NAME} not running (stopped)"
    else
        for pid in ${pids[@]}
        do
            echo $pid | xargs kill -9
            echo "* ${SERVICE_NAME} ($pid) force stopped."
        done
    fi

    echo "TODO: Clear zombie processed ..."
    c_pid=`ps -ef|grep ${SERVICE_MODULE} | grep -v ' grep '|awk '{print $2,$9}'`

    echo "$c_pid"
}


status() {
    c_pid=`ps -ef | grep ${SERVICE_MODULE} | grep -v ' grep ' | awk '{print $2}'`

    OLD_IFS="$IFS"
    IFS="\n"
    pids=($c_pid)
    IFS="$OLD_IFS"

    if [ ${#pids[@]} = 0 ]; then
        echo "* ${SERVICE_NAME} not running (stopped)"
    else
        for pid in ${pids[@]}
        do
            echo "* ${SERVICE_NAME} ($pid) is running with opened descriptors:"
            lsof -p $pid -n|awk '{print $2}'|sort|uniq -c|sort -nr|more
        done
    fi
}


# See how we were called.
case "$1" in
    debug)
        debug
        exit 0
    ;;

    start)
       start
       exit 0
    ;;

    stop)
       stop
       exit 0
    ;;

    forcestop)
        forcestop
        exit 0
    ;;

    status)
        status
        exit 0
    ;;

    restart)
        stop
        start
        exit 0
    ;;

    *)
        usage
        exit 0
    ;;

esac
