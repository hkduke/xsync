# xsync
extremely synchronize files among servers.

## before build

```
# for both xsync-server and xsync-client:
$ sh prepare/install-devel-libs.sh

# only for xsync-server:
$ sh prepare/install-redis-server.sh
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
