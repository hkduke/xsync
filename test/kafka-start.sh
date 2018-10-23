#!/bin/bash
#################################################
_file=$(readlink -f $0)
_cdir=$(dirname $_file)
_name=$(basename $_file)


echo "start kafka-server..."

${_cdir}/kafka/bin/kafka-server-start.sh ${_cdir}/kafka/config/server.properties

