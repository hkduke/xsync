# xsync
extremely synchronize files among servers.


## install lua

for both client and server

1) install dependencies of lua

    centos: yum install readline-devel
    debian: apt-get install libreadline-dev

2) build and install lua
 
   # cd lua-5.3.5/
   # make linux test && sudo make install

## before build

```
# for both xsync-server and xsync-client:

$ sh prepare/install-devel-libs.sh

# only for xsync-server:

#### redis-cluster installation
$ cd ${xsync_root}/prepare
$ sudo ./install-redis-cluster.sh --prefix=/opt/redis-cluster --cluster-id=dev --hostip="127.0.0.1" --ports="7001-7009" --replica=2 --maxmemory=1g --requirepass='test'

$ sudo /opt/redis-cluster/dev/start.sh
$ sudo /opt/redis-cluster/dev/cluster_create.sh

$ cd ${xsync_root}/libs/include
$ ln -s /opt/redis-cluster/redis/redis-5.0-RC3/include/hiredis hiredis
As a result:
$ ll
  hiredis -> /opt/redis-cluster/redis/redis-5.0-RC3/include/hiredis/

$ cd ${xsync_root}/libs/lib
$ ln -s /opt/redis-cluster/redis/redis-5.0-RC3/lib/libhiredis.a libhiredis.a
$ ln -s /opt/redis-cluster/redis/redis-5.0-RC3/lib/libhiredis.so libhiredis.libhiredis.so
As a result:
$ ll:
  libhiredis.a -> /opt/redis-cluster/redis/redis-5.0-RC3/lib/libhiredis.a
  libhiredis.so -> /opt/redis-cluster/redis/redis-5.0-RC3/lib/libhiredis.so*

For Ubuntu:
    $ sudo apt-get install libmysqlclient-dev

$ sh prepare/install-libzdb-mysql.sh

```

## build

```
$ cd src/
$ make
or
$ make clean
```


## Show opened handles by xsync-client:

```
$ ps -ef | grep xsync-client   # ${pid}
$ lsof -n|awk '{print $2}'|sort|uniq -c|sort -nr|grep ${pid}
```
