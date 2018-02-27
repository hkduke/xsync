#!/bin/bash
#
# File : install-mysqlclient-el6.sh
#
#   install libmysqlclient on rhel6 (centos 6)
#
# Refer:
#   http://blog.csdn.net/ubuntu64fan/article/details/78842909
#   https://stackoverflow.com/questions/18385188/why-am-i-seeing-this-libzdb-configure-error
#
# init created: 2018-02-27
# last updated: 2018-02-27
#
########################################################################
#
# DO NOT USE: yum install mysql-devel

yum install -y flex

rpm -ivh mysql-community-common-5.7.20-1.el7.x86_64.rpm
rpm -ivh mysql-community-libs-5.7.20-1.el7.x86_64.rpm
rpm -ivh mysql-community-client-5.7.20-1.el7.x86_64.rpm
rpm -ivh mysql-community-devel-5.7.20-1.el7.x86_64.rpm

