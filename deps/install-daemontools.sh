#!/bin/bash
#
# @file : install-daemontools.sh
#
#   install daemontools for linux (el5+, ubuntu12+) and tested on el6, 7.
#
# @create: 2018-11-02
# @update: 2018-11-02
#
########################################################################
# NOTE: readlink -f not support by MaxOS-X
_file=$(readlink -f $0)

_cdir=$(dirname $_file)
_name=$(basename $_file)
_ver=1.0.0

INSTALLDIR=$(dirname $_cdir)/libs

. $_cdir/common.sh

# Set characters encodeing
#   LANG=en_US.UTF-8;export LANG
LANG=zh_CN.UTF-8;export LANG

# https://blog.csdn.net/drbinzhao/article/details/8281645
# Treat unset variables as an error
set -o nounset

# Treat any error as exit
set -o errexit

# 安装路径
INSTALLPREFIX="/usr/local/lib"

###########################################################
# 安装 daemontools 到 $INSTALLPREFIX

function install_daemontools()
{
    local pkgname="daemontools-0.76.tar.gz"

    echoinfo "installing: $pkgname at: $INSTALLPREFIX/daemontools"

    cd ${_cdir}

    tar -zxf "${_cdir}/$pkgname"
    rm -rf "$INSTALLPREFIX/daemontools-0.76"
    mv "${_cdir}/daemontools-0.76" "$INSTALLPREFIX/"
    cd "$INSTALLPREFIX/daemontools-0.76"
    sh ./package/install

    cd ${_cdir}

    if [ -d "/etc/systemd/system/" ]; then
        echowarn "/etc/inittab is no longer used when using systemd"

        sed -i '/svscanboot/d' /etc/inittab

        # for el7 and later
        if [ -f "/etc/systemd/system/daemontools.service" ]; then
            rm -rf "/etc/systemd/system/daemontools.service.backup"
            mv "/etc/systemd/system/daemontools.service" "/etc/systemd/system/daemontools.service.backup"
        fi

        cp "${_cdir}/daemontools.service" "/etc/systemd/system/"
    fi
}


function start_daemontools()
{
    local pid=$(pids_of_proc "svscanboot")

    if [ -z "$pid" ]; then
        if [ -f "/command/svscanboot" ]; then
            if [ -f "/etc/systemd/system/daemontools.service" ]; then
                echoinfo "starting service: 'systemctl start daemontools.service'"

                systemctl start daemontools.service

                sleep 3

                systemctl status daemontools.service
            else
                echoinfo "starting command: '/command/svscanboot &'"

                /command/svscanboot &
            fi

            sleep 3
        else
            echoerror "svscanboot not found: /command/svscanboot"
            exit -1
        fi
    fi

    openfds=$(openfd_of_pid "$pid")
    echoinfo "svscanboot (pid=$pid, openfds=$openfds)"
    echo "----------------------------------------------------------------------"
    echo " pid    pcpu  res  virt  stime  user    uid    comm              args"
    echo "----------------------------------------------------------------------"
    ps -e -o 'pid,pcpu,rsz,vsz,stime,user,uid,comm,args' | grep "svscanboot" | grep -v grep | sort -nrk5
}


echoinfo "refer: [1] http://cr.yp.to/daemontools.html"
echoinfo "       [2] https://blog.csdn.net/superbfly/article/details/52877954"

if [[ ! -d "/service" && ! -d "/command" ]]; then
    echoinfo "install daemontools at: $INSTALLPREFIX/daemontools"

    install_daemontools;
    
    start_daemontools;
elif [[ -d "/service" && -d "/command" ]]; then
    dmrootlk=$(dirname $(dirname $(readlink "/command/supervise")))

    echowarn "daemontools already installed at: $dmrootlk -> $(readlink $dmrootlk)"
    
    start_daemontools;
else
    if [[ -d "/service" ]]; then
        echowarn "daemontools found: /service"
        echoerror "daemontools missing: /command"
    fi

    if [[ -d "/command" ]]; then
        echowarn "daemontools found: /command"
        echoerror "daemontools missing: /service"
    fi

    exit -1
fi
