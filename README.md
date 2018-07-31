# xsync
extremely synchronize files among servers.

## before build

```
# for both xsync-server and xsync-client:
$ sh prepare/install-devel-libs.sh

# only for xsync-server:

$ cd prepare
$ sudo ./install-redis-cluster.sh --prefix=/opt/redis-cluster --cluster-id=dev --hostip="211.152.44.82" --ports="7001-7009" --replica=2 --maxmemory=1g --requirepass='develPAss'
$ sudo /opt/redis-cluster/dev/start.sh
$ sudo /opt/redis-cluster/dev/cluster_create.sh

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
