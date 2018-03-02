# xsync

开源网络数据同步软件。在使用前必须遵守 xsync 使用协议，并获得授权许可！
<big>目前本软件正在开发中！</big>

## 0. xsync 介绍

用于在客户机 xsync-client（x86_64 Linux、MacOSX、Windows、Android、iOS）与服务器 xsync-server（x86_64 Linux: ubuntu，el 6.x，el 7.x）之间极速传输文件、消息。

目前仅支持 x86_64 Linux。

## 1. 系统环境要求

### 1.1 xsync-client

- x86_64 Linux
    - ubuntu
    - el 6.x (rhel6.x, centos6.x)
    - el 7.x (rhel7.x, centos7.x)
- MacOSX (TODO)
- Windows (TODO)
- Android (TODO)
- iOS (TODO)

### 1.2 xsync-server

- x86_64 Linux
    - ubuntu
    - el 6.x (rhel6.x, centos6.x)
    - el 7.x (rhel7.x, centos7.x)

## 2. 构建的准备工作

进入源码目录：

```
    # cd ${xsync_root}
```

### 2.1 构建 xsync-server 和 xsync-client
```
    # sh prepare/install-devel-libs.sh
```

### 2.2 仅仅构建 xsync-server
```
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
