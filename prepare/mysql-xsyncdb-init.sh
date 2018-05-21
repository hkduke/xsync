#!/bin/bash
#
# @file:
#   初始化 mysql 数据库 xsyncdb.
#
# @create: 2018-05-21
# @update:
#
########################################################################
# NOTE: readlink -f not support by MaxOS-X
_file=$(readlink -f $0)

_cdir=$(dirname $_file)
_name=$(basename $_file)
########################################################################
# mysql 初始化创建 xsyncdb 数据库。以 root 帐号和密码登录，创建数据库 xsyncdb
#   和用户 xsync.

# root 登录配置： 更改下面的值以适配实际的环境
#
mysqluser=root
mysqlpass=$1
mysqlhost=localhost
mysqlport=3306

# xsync 需要创建下面的数据库 xsyncdb 和用户 xsyncuser。不要修改！
#
xsyncdb="xsyncdb"
xsyncuser="xsync"

# 用户 xsyncuser 登录密码。请修改此值！
xsyncpass="p@ssW0rd!"

# 配置从那个主机（ip 或 hostname) 访问 xsyncdb。根据需要修改！
xsynchost="localhost"

mysql -u "$mysqluser" -p"$mysqlpass" -P "$mysqlport" -h "$mysqlhost" << EOFMYSQL

# drop database xsyncdb;

create database if not exists $xsyncdb;
use $xsyncdb;

# create table here...

create table if not exists test(id bigint(20));

# drop user 'xsync'@'localhost';

create user if not exists "$xsyncuser"@"$xsynchost" identified by "$xsyncpass";
revoke all privileges,grant option from "$xsyncuser"@"$xsynchost";
grant all privileges on $xsyncdb.* to "$xsyncuser"@"$xsynchost";
flush privileges;

EOFMYSQL


mysql -u "$xsyncuser" -p"$xsyncpass" -P "$mysqlport" -h "$xsynchost" "$xsyncdb" << EOFMYSQL

show tables;

EOFMYSQL
