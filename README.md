# xsync
extremely synchronize files among servers.

## before build

```
$ sh prepare/install-libs.sh
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
