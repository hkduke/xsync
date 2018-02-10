
redis-3.2.11
==================

redis/
    bin/
       redis-server                Redis服务器的daemon启动程序
       redis-cli                   Redis命令行操作工具。也可以用telnet根据其纯文本协议来操作
       redis-benchmark             Redis性能测试工具，测试Redis在当前系统下的读写性能
       redis-check-aof             数据修复
       redis-check-dump            检查导出工具
    conf/
       redis.conf                  服务配置文件，只需要关注几个配置:
                                      port 6379
