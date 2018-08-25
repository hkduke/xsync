## nginx 模块开发指南

ubuntu-18.04 with gcc-7

参考:

- https://blog.csdn.net/a236209186/article/details/52053021
- http://www.evanmiller.org/nginx-modules-guide.html

1) 下载源码

  $ wget http://nginx.org/download/nginx-1.15.2.tar.gz

  $ tar -zxf nginx-1.15.2.tar.gz

2) 解压后执行编译
  $ cd /path/to/nginx-1.15.2

  $ ./configure --prefix=/opt/nginx --with-http_ssl_module

  $ make && sudo make install

3) 启动和退出
  $ sudo /opt/nginx/sbin/nginx
  $ sudo /opt/nginx/sbin/nginx -s stop

如果显示端口被占用, 可以:
  $ sudo netstat -anp | grep 80

如果被 apache 占用:
  tcp6       0      0 :::80                   :::*                    LISTEN      4612/apache2

  $ which apache2
  $ sudo /usr/sbin/apache2ctl stop

现在可以访问 nginx:
  http://localhost

3) 带模块编译

编辑模块文件:
    ngx_http_echo/ngx_http_echo_module.c
    ngx_http_echo/config

重新编译:

复制模块源码到 nginx 模块目录:
  $ cp -r ngx_http_echo /path/to/nginx-1.15.2/src/http/modules/

  $ cd /path/to/nginx-1.15.2

  $ ./configure --prefix=/opt/nginx --with-http_ssl_module --add-module=./src/http/modules/ngx_http_echo

  adding module in /home/root1/Downloads/nginx-1.15.2/src/http/modules/ngx_http_echo
        + ngx_http_echo_module was configured

  $ make && sudo make install

4) 配置 nginx.conf 使用 echo 模块 (/opt/nginx/conf/nginx.conf):

    server {
        listen    80;
        
        ...

        # ngx_http_echo
        location /echo {
            echo "hello nginx";
        }
    }

5) 启动 nginx

访问: http://localhost/echo
