#!/bin/bash
#
# @file:
#    download files utility
#
# @author: zhangliang@ztgame
#
# @create: 2014-12-18
#
# @update: 2018-06-13 12:11:47
#
########################################################################

# Treat unset variables as an error
set -o nounset

# Treat any error as exit
set -o errexit


# wget_download_pkg "ftp://pub:public@pepstack.com/hacl-repo/redis-ruby/ruby-2.3.1.tar.gz"
#
function wget_download_pkg() {
    local pkguri=$1

    # 下载临时文件夹, 使用完毕需要删除
    local dltmpdir=$(mktemp -d /tmp/downloads-cache.XXXXXX)

    # 下载的文件名
    local dlpkgname=$(basename $pkguri)

    # 下载的文件全路径名
    local localpkgfile=$dltmpdir"/"$dlpkgname
    
    if [ "${pkguri:0:5}" = "http:" -o "${pkguri:0:6}" = "https:" -o \
        "${pkguri:0:4}" = "ftp:" -o "${pkguri:0:5}" = "ftps:" ]; then
        # using wget to download to local

        # 开始下载文件
        wget -c "$pkguri" -P "$dltmpdir"
    else
        # 开始复制文件
        cp "$(real_path $(dirname $pkguri))"/"$(basename $pkguri)" "$localpkgfile"
    fi

    echo "$localpkgfile"
}
