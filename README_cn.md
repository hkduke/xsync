# xsync


@file:

@author:

@create:

@update:

--------

开源网络数据同步软件。在使用前必须遵守 xsync 使用协议，并获得授权许可！
<big>目前本软件正在开发中！</big>


## 0. xsync 介绍

用于在客户机 xsync-client（x86_64 Linux、MacOSX、Windows、Android、iOS）与服务器 xsync-server（x86_64 Linux: ubuntu，el 6.x，el 7.x）之间极速传输文件、消息。

目前仅支持 x86_64 Linux。

## 1. 系统环境要求

### 1.1 xsync-client

- x86_64 Linux
    - ubuntu14
    - el 6.x (rhel6.x, centos6.x)
    - el 7.x (rhel7.x, centos7.x)
- MacOSX (TODO)
- Windows (TODO)
- Android (TODO)
- iOS (TODO)

xsync-client 当前在 el6, el7, ubuntu18.04 上测试通过。

不同的平台应该首先采用源码编译 libs, 然后编译 xsync-client。

### 1.2 xsync-server

- x86_64 Linux
    - ubuntu18.04
    - el 6.x (rhel6.x, centos6.x)
    - el 7.x (rhel7.x, centos7.x)

xsync-server 基于 redis-cluster, redis-cluster 最好安装在 el7 集群上。

## 2. 构建 xsync

源码目录： ${xsync_root}

执行某些辅助脚本可能需要 python2.7。


### 2.1 仅仅构建 xsync-client

```
    # sh prepare/install-devel-libs.sh
```

### 2.2 构建 xsync-server

2.2.1 安装 mysql 开发客户端

服务端使用了 redis 作为缓存 db，使用了 mysql 作为持久化存储。因此编译的机器上需要安装 mysql 开发客户端。

- 在 ubuntu 上编译需要安装 libmysqlclient:
```
     $ sudo apt-get install libmysqlclient-dev
```

- 在 rhel6 上编译需要安装 libmysqlclient:

先删除已经安装的MySQL包 (?):
```
    # rpm -qa|grep -i 'mysql'
       MySQL-client-5.6.39-1.el6.x86_64
       MySQL-devel-5.6.39-1.el6.x86_64
       MySQL-shared-5.6.39-1.el6.x86_64
       MySQL-shared-compat-5.6.39-1.el6.x86_64
     # rpm -e MySQL-client-5.6.39-1.el6.x86_64
     # rpm -e MySQL-devel-5.6.39-1.el6.x86_64
     # rpm -e MySQL-shared-compat-5.6.39-1.el6.x86_64 --nodeps
     # rpm -e MySQL-shared-5.6.39-1.el6.x86_64 --nodeps
```

然后安装:
```
    # rpm -ivh MySQL-client-5.6.39-1.el6.x86_64.rpm MySQL-devel-5.6.39-1.el6.x86_64.rpm MySQL-shared-compat-5.6.39-1.el6.x86_64.rpm MySQL-shared-5.6.39-1.el6.x86_64.rpm
```

libmysqlclient.so 在:

      /usr/lib64/

最后安装开发包：


```
    # sh prepare/install-devel-libs.sh
    # sh prepare/install-redis-server.sh
    # sh prepare/install-libzdb-mysql.sh
```

## 3. 开始构建

```
    $ cd ${xsync_root}/src/
    $ make
    清理
    $ make clean
```

构建之后的结果在：${xsync_root}/target/


## 4. Q&A

- Q1：显示程序（xsync-client）打开的句柄数

```
    # ps -ef | grep xsync-client   # ${pid}
    # lsof -n|awk '{print $2}'|sort|uniq -c|sort -nr|grep ${pid}
```
