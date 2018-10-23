/***********************************************************************
* COPYRIGHT (C) 2018 PEPSTACK, PEPSTACK.COM
*
* THIS SOFTWARE IS PROVIDED 'AS-IS', WITHOUT ANY EXPRESS OR IMPLIED
* WARRANTY. IN NO EVENT WILL THE AUTHORS BE HELD LIABLE FOR ANY DAMAGES
* ARISING FROM THE USE OF THIS SOFTWARE.
*
* PERMISSION IS GRANTED TO ANYONE TO USE THIS SOFTWARE FOR ANY PURPOSE,
* INCLUDING COMMERCIAL APPLICATIONS, AND TO ALTER IT AND REDISTRIBUTE IT
* FREELY, SUBJECT TO THE FOLLOWING RESTRICTIONS:
*
*  THE ORIGIN OF THIS SOFTWARE MUST NOT BE MISREPRESENTED; YOU MUST NOT
*  CLAIM THAT YOU WROTE THE ORIGINAL SOFTWARE. IF YOU USE THIS SOFTWARE
*  IN A PRODUCT, AN ACKNOWLEDGMENT IN THE PRODUCT DOCUMENTATION WOULD
*  BE APPRECIATED BUT IS NOT REQUIRED.
*
*  ALTERED SOURCE VERSIONS MUST BE PLAINLY MARKED AS SUCH, AND MUST NOT
*  BE MISREPRESENTED AS BEING THE ORIGINAL SOFTWARE.
*
*  THIS NOTICE MAY NOT BE REMOVED OR ALTERED FROM ANY SOURCE DISTRIBUTION.
***********************************************************************/

/**
 * @file: sslapi.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.2.5
 *
 * @create: 2015-07-21
 *
 * @update: 2018-10-22 10:56:41
 *
 *----------------------------------------------------------------------
 * api doc:
 *   http://openssl.org/docs/manmaster/ssl/SSL_CTX_use_certificate.html
 *
 * reference:
 *   1) http://www.cppblog.com/flyonok/archive/2011/03/24/133100.html
 *   2) http://www.cppblog.com/flyonok/archive/2010/10/30/131840.html
 *   3) http://www.linuxidc.com/Linux/2011-04/34523.htm
 *   4) http://blog.csdn.net/demetered/article/details/12511771
 *   5) http://zhoulifa.bokee.com/6074014.html
 *   6) http://blog.csdn.net/jinhill/article/details/6960874
 *   7) http://www.ibm.com/developerworks/cn/linux/l-openssl.html
 *   8) http://linux.chinaunix.net/docs/2006-11-10/3175.shtml
 *   9) http://www.cnblogs.com/huhu0013/p/4791724.html
 *----------------------------------------------------------------------
 */

#ifndef SSLAPI_H_INCLUDED
#define SSLAPI_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/types.h>


#ifdef WIN32
    #ifndef _WINDOWS_
        #include <winsock2.h>
        #include <windows.h>
        #include <wchar.h>
    #endif
#else
    #include <sys/time.h>
#endif


/* openssl */
#include <openssl/rsa.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>


/* sslapi error code */
#define ERR_SSL_OK           0
#define ERR_SSL_new          (-101)
#define ERR_SSL_set_fd       (-102)
#define ERR_SSL_accept       (-103)


/**
 * SSLAPI_cert_verify()
 * returns:
 */
#define ERR_CERT_true        0
#define ERR_CERT_false      (1)
#define ERR_CERT_bad       (-1)


#ifdef WIN32
    #pragma comment(lib, "libeay32.lib")
    #pragma comment(lib, "ssleay32.lib")
#endif


#define SSL_LAST_ERROR(sslerr, errbuf) \
    (sslerr)? (*(sslerr)) = SSLAPI_get_last_error(errbuf) : SSLAPI_get_last_error(errbuf)


__attribute__((unused))
static int SSLAPI_get_last_error(char *errbuf)
{
    int err = ERR_peek_last_error();
    if (errbuf) {
        ERR_error_string(err, errbuf);
    }
    return err;
}


__attribute__((unused))
static void close_socket(int sockfd)
{
    if (sockfd && sockfd != -1) {
#ifdef _MSC_VER
        closesocket(sockfd);
#else
        close(sockfd);
#endif
    }
}


__attribute__((unused))
static int connect_nonblock(int sd, const struct sockaddr *name, int namelen, int timeo_seconds)
{
    int err;
    int len = sizeof(err);

    // 设置为非阻塞模式
    unsigned long nonblock = 1;

#ifdef WIN32
    ioctlsocket(sd, FIONBIO, &nonblock);
#else
    ioctl(sd, FIONBIO, &nonblock);
#endif

    // 恢复为阻塞模式
    nonblock = 0;

    err = connect(sd, name, namelen);
    if (err == 0) {
        // 连接成功
#ifdef WIN32
        ioctlsocket(sd, FIONBIO, &nonblock);
#else
        ioctl(sd, FIONBIO, &nonblock);
#endif
        return 0;
    } else {
        struct timeval tm;
        fd_set set;

        tm.tv_sec  = timeo_seconds;
        tm.tv_usec = 0;

        FD_ZERO(&set);
        FD_SET(sd, &set);

        err = select(sd + 1, 0, &set, 0, &tm);
        if (err > 0) {
            if (FD_ISSET(sd, &set)) {
                // 下面的一句一定要，主要针对防火墙
                if (getsockopt(sd, SOL_SOCKET, SO_ERROR, (char *) &err, &len) != 0) {
                    // perror("getsockopt()");
                    err = -1;
                    goto onerror_ret;
                }

                if (err == 0) {
                    // 连接成功
                #ifdef WIN32
                    ioctlsocket(sd, FIONBIO, &nonblock);
                #else
                    ioctl(sd, FIONBIO, &nonblock);
                #endif
                    return 0;
                }
            }

            err = -2;
            goto onerror_ret;
        } else if (err == 0) {
            // 超时
            err = -3;
            goto onerror_ret;
        } else {
            // 错误
            perror("select()");
            err = -4;
            goto onerror_ret;
        }
    }

onerror_ret:
#ifdef WIN32
    ioctlsocket(sd, FIONBIO, &nonblock);
#else
    ioctl(sd, FIONBIO, &nonblock);
#endif

    return err;
}


__attribute__((unused))
static SSL_CTX * SSLAPI_sslctx_create(
    int is_server_peer,             /* 1 - SSLv23_server_method, 0 - SSLv23_client_method */
    int verify_peer_depth,          /* > 0 - SSL_VERIFY_PEER with depth, 0 - SSL_VERIFY_NONE (default) */
    const char * cert_pem_file,     /* 服务端或客户端的证书(需经CA签名) */
    const char * privkey_pem_file,  /* 服务端或客户端的私钥(建议加密存储) */
    const char * cacert_pem_file,   /* CA 的证书 */
    char * cert_passwd,             /* 私钥密码 */
    char * sslerrstr                /* at lease 120 bytes len */,
    int * sslerrno)
{
    SSL_CTX * sslctx;

    /**
     * SSL 库初始化
     * SSL_library_init() always returns "1", so it is safe to discard the return value.
     */
    SSL_library_init();

    /**
     * 载入所有 SSL 算法
     * OpenSSL_add_ssl_algorithms()
     */
    OpenSSL_add_all_algorithms();

    /**
     * 载入所有 SSL 错误消息
     */
    SSL_load_error_strings();

    /**
     * 创建SSL上下文环境 每个进程只需维护一个SSL_CTX结构体
     * 以 SSL V2 和 V3 标准兼容方式产生一个 SSL_CTX
     * 也可以用 SSLv2_server_method() 或 SSLv3_server_method() 单独表示 V2 或 V3标准
     *
     * SSL_METHOD的构造函数包括:
     *   SSL_METHOD *TLSv1_server_method(void);   TLSv1.0
     *   SSL_METHOD *TLSv1_client_method(void);   TLSv1.0
     *   SSL_METHOD *SSLv2_server_method(void);   SSLv2
     *   SSL_METHOD *SSLv2_client_method(void);   SSLv2
     *   SSL_METHOD *SSLv3_server_method(void);   SSLv3
     *   SSL_METHOD *SSLv3_client_method(void);   SSLv3
     *   SSL_METHOD *SSLv23_server_method(void);  SSLv3 but can rollback to v2
     *   SSL_METHOD *SSLv23_client_method(void);  SSLv3 but can rollback to v2
     */
    if (is_server_peer) {
        sslctx = SSL_CTX_new(SSLv23_server_method());
    } else {
        sslctx = SSL_CTX_new(SSLv23_client_method());
    }

    if (! sslctx) {
        SSL_LAST_ERROR(sslerrno, sslerrstr);
        return 0;
    }

    if (verify_peer_depth > 0) {
        /* 验证 */
        SSL_CTX_set_verify(sslctx, SSL_VERIFY_PEER, 0);

        if (is_server_peer) {
            /* 若验证，则放置CA证书 */
            SSL_CTX_set_client_CA_list(sslctx, SSL_load_client_CA_file(cacert_pem_file));

            /* 设置最大的验证用户证书的上级数 */
            SSL_CTX_set_verify_depth(sslctx, verify_peer_depth);
        }
    } else {
        /* 不验证(默认) */
        SSL_CTX_set_verify(sslctx, SSL_VERIFY_NONE, 0);
    }

    if (cacert_pem_file) {
        /**
         * 要验证对方的话,要有CA的证书.
         * 加载CA的证书文件
         */
        SSL_CTX_load_verify_locations(sslctx, cacert_pem_file, 0);
    }

    if (cert_passwd) {
        /**
         * 设置证书密码, 以避免手动输入
         */
        SSL_CTX_set_default_passwd_cb_userdata(sslctx, cert_passwd);
    }

    if (cert_pem_file) {
        /**
         * 加载自己的数字证书:
         *   SSL_CTX_use_certificate_ASN1
         */
        if (SSL_CTX_use_certificate_file(sslctx, cert_pem_file, SSL_FILETYPE_PEM) <= 0) {
            SSL_LAST_ERROR(sslerrno, sslerrstr);
            SSL_CTX_free(sslctx);
            return 0;
        }
    }

    if (privkey_pem_file) {
        /**
         * 加载自己的私钥,以用于签名
         */
        if (SSL_CTX_use_PrivateKey_file(sslctx, privkey_pem_file, SSL_FILETYPE_PEM) <= 0) {
            SSL_LAST_ERROR(sslerrno, sslerrstr);
            SSL_CTX_free(sslctx);
            return 0;
        }
    }

    if (cert_pem_file && privkey_pem_file) {
        /**
         * 调用了以上两个函数后,自己检验一下证书与私钥是否配对
         */
        if (! SSL_CTX_check_private_key(sslctx)) {
            SSL_LAST_ERROR(sslerrno, sslerrstr);
            SSL_CTX_free(sslctx);
            return 0;
        }
    }

    /**
     * 根据SSL/TLS规范, 在ClientHello中, 客户端会提交一份自己能够支持的
     * 加密方法的列表, 由服务端选择一种方法后在ServerHello中通知服务端,
     * 从而完成加密算法的协商.
     *
     * 可用的算法为:
     * EDH-RSA-DES-CBC3-SHA
     * EDH-DSS-DES-CBC3-SHA
     * DES-CBC3-SHA
     * DHE-DSS-RC4-SHA
     * IDEA-CBC-SHA
     * RC4-SHA
     * RC4-MD5
     * EXP1024-DHE-DSS-RC4-SHA
     * EXP1024-RC4-SHA
     * EXP1024-DHE-DSS-DES-CBC-SHA
     * EXP1024-DES-CBC-SHA
     * EXP1024-RC2-CBC-MD5
     * EXP1024-RC4-MD5
     * EDH-RSA-DES-CBC-SHA
     * EDH-DSS-DES-CBC-SHA
     * DES-CBC-SHA
     * EXP-EDH-RSA-DES-CBC-SHA
     * EXP-EDH-DSS-DES-CBC-SHA
     * EXP-DES-CBC-SHA
     * EXP-RC2-CBC-MD5
     * EXP-RC4-MD5
     *
     * 这些算法按一定优先级排列,如果不作任何指定,将选用DES-CBC3-SHA.
     * 用SSL_CTX_set_cipher_list可以指定自己希望用的算法
     * 实际上只是提高其优先级,是否能使用还要看对方是否支持.
     *
     * 选用了RC4做加密,MD5做消息摘要(先进行MD5运算,后进行RC4加密)
     */
    SSL_CTX_set_cipher_list(sslctx, "RC4-MD5");

    #ifdef WIN32
        /**
         * void RAND_seed(const void *buf, int num);
         * 构建随机数生成机制,WIN32平台必需.
         *
         * 在win32 的环境中client程序运行时出错(SSL_connect返回-1)的一个
         * 主要机制便是与UNIX平台下的随机数生成机制不同(握手的时候用的到).
         * 具体描述可见mod_ssl的FAQ.解决办法就是调用此函数,其中buf应该为
         * 一随机的字符串,作为"seed".
         *
         * 还可以采用一下两个函数:
         *     void RAND_screen(void);
         *     int RAND_event(UINT, WPARAM, LPARAM);
         * 其中RAND_screen()以屏幕内容作为"seed"产生随机数,RAND_event可
         * 以捕获windows中的事件(event),以此为基础产生随机数.
         * 如果一直有 用户干预的话,用这种办法产生的随机数能够"更加随机",
         * 但如果机器一直没人理(如总停在登录画面),则每次都将产生同样的数字.
         */
        do {
            int i;
            int seed_int[256];
            srand((unsigned)time(0));
            for (i = 0; i < 256; i++) {
                seed_int[i] = rand();
            }

            RAND_seed(seed_int, sizeof(seed_int));
        } while (RAND_status() == 0);
    #else
        do {
            RAND_poll();
            while (RAND_status() == 0) {
                unsigned short us_seed = (unsigned short) rand() % 65536;
                RAND_seed(&us_seed, sizeof(us_seed));
            }
        } while (0);
    #endif

    return sslctx;
}


__attribute__((unused))
static void SSLAPI_sslctx_release(SSL_CTX ** sslctx)
{
    if (sslctx) {
        SSL_CTX *ctx = *sslctx;
        *sslctx = 0;

        if (ctx) {
            SSL_CTX_free(ctx);
        }
    }
}


__attribute__((unused))
static int SSLAPI_ssl_create(SSL_CTX * sslctx, int sockfd,
    SSL ** pssl,
    char * sslerrstr /* at lease 120 bytes len */,
    int * sslerrno)
{
    SSL * ssl;

    /* 基于 ctx 产生一个新的 SSL */
    ssl = SSL_new(sslctx);
    if (! ssl) {
        SSL_LAST_ERROR(sslerrno, sslerrstr);
        return ERR_SSL_new;
    }

    /* 将连接的 socket 加入到 SSL */
    if (SSL_set_fd(ssl, sockfd) == 0) {
        SSL_LAST_ERROR(sslerrno, sslerrstr);

        /* 释放 SSL */
        SSL_free(ssl);

        return ERR_SSL_set_fd;
    }

    /**
     * SSL_set_mode() adds the mode set via bitmask in mode to ssl.
     *   Options already set before are not cleared.
     * 设备SSL连接模式自动重试
     */
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

    *pssl = ssl;

    return ERR_SSL_OK;
}


__attribute__((unused))
static void SSLAPI_ssl_release(SSL ** pssl, int sockfd)
{
    if (pssl) {
        SSL * ssl = *pssl;
        *pssl = 0;
        if (ssl) {
            if (sockfd && sockfd != -1) {
                /* 关闭 SSL 连接 */
                int r = SSL_shutdown(ssl);
                if (! r) {
                     /* If we called SSL_shutdown() first then we always get
                      *   return value of ’0’.
                      * In this case, try again, but first send a TCP FIN to
                      *   trigger the other side’s close_notify.
                      * https://linux.die.net/man/2/shutdown
                      */
                     shutdown(sockfd, 1);

                     r = SSL_shutdown(ssl);
                }

                if (r != 1) {
                    int sslerrno;
                    char sslerrstr[120];
                    SSL_LAST_ERROR(&sslerrno, sslerrstr);
                    perror(sslerrstr);
                }
            }

            /* 释放 SSL */
            SSL_free(ssl);
        }
    }
}


/**
 * 获取SSL证书
 */
__attribute__((unused))
static X509 * SSLAPI_ssl_get_cert(SSL * ssl,
    char * sslerrstr   /* at lease 120 bytes len */,
    int * sslerrno)
{
    /**
     * X509 * SSL_get_peer_certificate(SSL *);
     * 用此函数从SSL结构中提取出对方的证书(此时证书得到且已经验证过了)
     * 整理成X509结构.
     */
    X509 * cert = SSL_get_peer_certificate(ssl);
    if (! cert) {
        SSL_LAST_ERROR(sslerrno, sslerrstr);
        return 0;
    }

    if (SSL_get_verify_result(ssl) != X509_V_OK) {
        SSL_LAST_ERROR(sslerrno, sslerrstr);
    } else {
        *sslerrno = 0;
        *sslerrno = X509_V_OK;
    }

    return cert;
}


/**
 * 使用完毕,释放证书
 */
__attribute__((unused))
static void SSLAPI_ssl_release_cert(X509 ** pcert)
{
    if (pcert) {
        X509 *cert = *pcert;
        *pcert = 0;

        if (cert) {
            X509_free(cert);
        }
    }
}


/**
 * 打印证书信息
 */
__attribute__((unused))
static void SSLAPI_cert_print(X509 *cert, char *buf, int bufsize)
{
    char *line;

    /**
     * X509_NAME * X509_get_issuer_name(X509 *cert);
     * 得到证书签署者(往往是CA)的名字
     *
     * X509_NAME * X509_get_subject_name(X509 *a);
     * 得到证书所有者的名字
     *
     * char * X509_NAME_oneline(X509_NAME *a, char *buf, int size);
     * 将以上两个函数得到的对象变成字符型,以便打印出来.
     */

    printf("peer x509 certificate:\n");
    line = X509_NAME_oneline(X509_get_subject_name(cert), buf, bufsize);
    printf("\tsubject=[%s]\n", line);
    if (line != buf) {
        OPENSSL_free(line);
    }

    line = X509_NAME_oneline(X509_get_issuer_name(cert), buf, bufsize);
    printf("\tissuer=[%s]\n", line);
    if (line != buf) {
        OPENSSL_free(line);
    }
}


/**
 * 验证证书的合法性
 */
__attribute__((unused))
static int SSLAPI_cert_verify_CN(X509 * cert, const char * CN, char * buf, int sizebuf)
{
    *buf = 0;

    if (! cert) {
        /* bad cert*/
        return ERR_CERT_bad;
    } else {
        X509_NAME_get_text_by_NID(X509_get_issuer_name(cert), NID_commonName, buf, sizebuf);
        return strcmp(buf, CN)? ERR_CERT_false : ERR_CERT_true;
    }
}


/**
 * 通过SSL发送数据块
 */
__attribute__((unused))
static int SSLAPI_write_bulky (SSL * ssl,
    const char* writebuf,  /* buffer to write bulky message for SSL_write */
    int writelen,          /* total size of bulky in bytes for write */
    int pernum             /* num of bytes to write for per SSL_write. maybe 1 */
)
{
    int num, ret;
    int left = writelen;
    int offset = 0;

    while (left > 0) {
        num = left > pernum? pernum : left;

        ret = SSL_write(ssl, writebuf + offset, num);

        if (ret > 0) {
            left -= ret;
            offset += ret;
        } else if (ret == -1 && SSL_get_error(ssl, ret) == SSL_ERROR_WANT_WRITE) {
            continue;
        } else {
            break;
        }
    }

    return offset;
}


/**
 * 通过SSL接收数据块
 */
__attribute__((unused))
static int SSLAPI_read_bulky (SSL * ssl,
    char* readbuf, /* buffer to recv bulky message from SSL_read */
    int readlen,   /* total size of bulky in bytes for read */
    int pernum     /* num of bytes to read for per SSL_read. maybe 1 */
)
{
    int  num, ret;
    int  left = readlen;
    int  offset = 0;

    while (left > 0) {
        num = left > pernum? pernum : left;

        ret = SSL_read(ssl, readbuf + offset, num);

        if (ret > 0) {
            left -= ret;
            offset += ret;
        } else if (ret == -1 && SSL_get_error(ssl, ret) == SSL_ERROR_WANT_READ) {
            continue;
        } else {
            break;
        }
    }

    return offset;
}

#if defined(__cplusplus)
}
#endif

#endif /* SSLAPI_H_INCLUDED */
