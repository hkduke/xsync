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
 * @file: xsync-protocol.h
 *   Define communication format of messages between xsync-client and xsync-server
 *
 * @author: master@pepstack.com
 *
 * @version: 0.2.6
 *
 * @create: 2018-01-29
 *
 * @update: 2018-10-22 10:56:41
 */

/***********************************************************************
 * 可用命令列表
 *
 *     XCON  客户端发起 socket 连接请求        XSConnectReq_t
 *
 *     XLOG  客户端发起开始文件条目请求        XSLogEntryReq_t
 *
 *     XSYN  客户端发起传输文件条目请求        XSSyncFileReq_t
 *
 *     XCMD  客户端发起让服务器执行命令请求    XSCommandReq_t
 *
 **********************************************************************/

/**********************************************************************
* xsync server tables on redis-cluster:
*
* 1) 客户端连接表
*
*  { XCON | key=[xs:$serverid:xcon:$connfd] }
*
*   key             host   port          clientid       session (shell `date +%s%N`)
* -----------------------------------------------------------------------------------
* xs:1:xcon:10   127.0.0.1  38690      xsync-test         1535697852319190425
* xs:1:xcon:12   127.0.0.1  38691      xsync-test         1535697951786758440
* xs:1:xcon:14   127.0.0.1  38694      xsync-test2
* xs:1:xcon:15   127.0.0.1  38696      xsync-test2
* ...
*
*
* 2) 传输文件日志表
*
*  { XLOG | key=[xs:$serverid:xlog:$clientid:$md5sum(watchfile)] }
*
*  ? = md5 = md5sum("/path/to/somefile")
*
*  时间戳单位: 秒
*
*  全  局                 |  客户端监控         | 条目创建 | 过  期  | 更  新  | 当前更新 | 文件大小  | 最后修改  | 客 户 端 | 服务端条目  | 服务端本地 | 完成   | 错误   | 服务端
*  唯一键                 |  文件全路径         | 时间戳   | 时间戳  | 时间戳  | 字节偏移 | 字    节  | 时    间  | 文件签名 | 文件全路径  | 文件签名值 | 状态   | 日志   | 条目ID
*   key                   |   pathfile          | credtime | exptime | updtime | offset   |  filesize |  modtime  | watchmd5 |  entryfile  |  entrymd5  | status | errlog | entryid
* ------------------------+---------------------+----------+---------+---------+----------+-----------+-----------+----------+-------------+------------+--------+--------+---------
* xs:1:xlog:xsync-test:?  | /path/to/somefile1  |
* xs:1:xlog:xsync-test:?  | /path/to/somefile2  |
* xs:1:xlog:xsync-test2:? | /path/to/somefile3  |
* xs:1:xlog:xsync-test2:? | /path/to/somefile4  |
* ...
*
*
* 3) 服务器同步文件会话表: 用户(客户端)与服务器建立连接之后, 发送 XLOG 命令, 服务器返回 entryid, 同步数据开始
*
*  { XSYN | key=[xs:$serverid:xsyn:$entryid] }
*
*   同步条目ID  |   XLOG 主键              |
*     key       |    logkey                |
* --------------+--------------------------+
* xs:1:xsyn:1   |  xs:1:xlog:xsync-test:?  |
* xs:1:xsyn:2   |
* xs:1:xsyn:3   |
* ...
*
*
*
**********************************************************************/

#ifndef XSYNC_PROTOCOL_H_
#define XSYNC_PROTOCOL_H_

#if defined(__cplusplus)
extern "C"
{
#endif

#include "xsync-config.h"

#include "./common/byteorder.h"
#include "./common/threadlock.h"

#include "./common/randctx.h"

#include <zlib.h>


#ifdef __GNUC__
#  define GNUC_PACKED    __attribute__((packed))
#else
#  define GNUC_PACKED
#endif

#ifdef __arm
#  define ARM_PACKED    __packed
#else
#  define ARM_PACKED
#endif

__attribute__((used))
static union {
    /* big endian */
    char c[4];
    ub4 msgid;
} XS_MSGID_XCON = {{'X','C','O','N'}};

__attribute__((used))
static union {
    /* big endian */
    char c[4];
    ub4 msgid;
} XS_MSGID_XLOG = {{'X','L','O','G'}};

__attribute__((used))
static union {
    /* big endian */
    char c[4];
    ub4 msgid;
} XS_MSGID_XSYN = {{'X','S','Y','N'}};

__attribute__((used))
static union {
    /* big endian */
    char c[4];
    ub4 msgid;
} XS_MSGID_XCMD = {{'X','C','M','D'}};


/***********************************************************************
 * XSConnectReq_t
 *
 *   This is the 1st message sent by client to server so as to
 *     establish a new session connection.
 *
 *   The xsync_newconn_t makes up with a fix head of 64 bytes.
 *
 *        4 bytes      |       4 bytes
 * --------------------+--------------------
 *  0  ConnType        |        MAGIC      7
 * --------------------+--------------------
 *  8   Version        |      TimeStamp   15
 * --------------------+--------------------
 *  16          CLIENTID 40 bytes
 *                 ...   ...              55
 * --------------------+--------------------
 *  56  Random Number  |        CRC32     63
 * -----------------------------------------
 *  64
 **********************************************************************/
#define XS_CONNECT_REQ_SIZE    64

#ifdef _MSC_VER
#  pragma pack(1)
#endif

typedef struct XSConnectReq_t
{
    union {
        struct {
            ub4 msgid;              /* XCON */
            ub4 magic;              /* xserver 要求的魔数 */

            ub4 randnum;
            ub4 client_version;     /* xsync-client version */

            ub8 client_utctime;     /* 64 bits time */

            /* 36 characters length of client id */
            ub1 clientid[XSYNC_CLIENTID_MAXLEN];

            ub4 crc32_checksum;
        };

        ub1 head[XS_CONNECT_REQ_SIZE];
    };
} GNUC_PACKED ARM_PACKED XSConnectReq_t;

#ifdef _MSC_VER
#  pragma pack()
#endif


/**********************************************************************
 * XSConnectReply_t
 *   服务端返回
 *
 *********************************************************************/
#define XS_CONNECT_ACCEPT_REPLY_SIZE    40
#define XS_CONNECT_REJECT_REPLY_SIZE    8


#ifdef _MSC_VER
#  pragma pack(1)
#endif
typedef struct XSConnectReply_t
{
    union {
        // 拒绝连接
        struct {
            ub4 reject_msgid;        /* 拒绝消息 = 'NOCX' */
            ub4 reject_code;         /* 拒绝代码: 错误代码 */
        };

        // 接受连接
        struct {
            ub4 msgid;              /* 接受消息 = 'XCON' */
            ub4 magic;              /* 结果代码: 根据请求的 randnum 和 magic 计算得到的魔数 */

            ub4 server_version;     /* xsync-server version */
            ub4 bitflags;           /* 附加参数标识: 指定启用的编码, 加密, 压缩, 备用服务等. 默认 0 */

            ub8 server_utctime;     /* xsync-server time */

            ub8 session;            /* global unique id for connection session */

            ub4 paramslen;          /* 附加参数长度: 默认 0 */
            ub4 crc32_checksum;     /* 校验值: 仅校验以上全部内容 */

            ub1 params[0];          /* 附加参数 */
        };

        ub1 head[XS_CONNECT_ACCEPT_REPLY_SIZE];
    };
} GNUC_PACKED ARM_PACKED XSConnectReply_t;
#ifdef _MSC_VER
#  pragma pack()
#endif


/**********************************************************************
 * XLOG Command Request
 *   注册条目命令.
 *
 *
 *********************************************************************/
#define XS_LOGENTRY_REQ_SIZE    64

#ifdef _MSC_VER
#  pragma pack(1)
#endif

typedef struct XSLogEntryReq_t
{
    union {
        struct {
            ub4 msgid;              /* XLOG */
            ub4 datalen;            /* data body length in bytes NOT including sizeof head */

            ub8 session;            /* XCON 成功之后服务端返回全局唯一会话 ID */

            ub4 reserved1;          /* 0 */
            ub4 reserved2;          /* 0 */

            ub8 exptime;            /* 文件过期时间: 0 不过期 */

            ub8 modtime;            /* 文件最后修改时间 */

            ub8 filesize;           /* 文件最后字节尺寸 */

            ub1 filemd5[16];        /* 文件最后 md5: 16 字节表示 */

            ub1 pathfile[0];        /* 变长数组, 保存文件全路径名. 必须以 '\0' 结尾! */
        };

        ub1 head[XS_LOGENTRY_REQ_SIZE];
    };
} GNUC_PACKED ARM_PACKED XSLogEntryReq_t;

#ifdef _MSC_VER
#  pragma pack()
#endif


/**********************************************************************
 * XSYN Command Request
 *   数据同步命令. 每个数据包的包头都是这个结构 (固定 40 个字节大小).
 *   datalen 是本次传输文件数据的字节数, 不包括此包头.
 *
 *********************************************************************/
#define XS_SYNC_REQ_SIZE    40

#ifdef _MSC_VER
#  pragma pack(1)
#endif

typedef struct XSSyncFileReq_t
{
    union {
        struct {
            ub4 msgid;              /* XSYN */
            ub4 datalen;            /* data body length in bytes NOT including sizeof head */

            ub8 session;

            ub4 reserved1;          /* 0 */
            ub4 reserved2;          /* 0 */

            ub8 entryid;            /* 条目 ID */

            ub8 offset;             /* 文件偏移字节 */
        };

        ub1 head[XS_SYNC_REQ_SIZE];
    };
} GNUC_PACKED ARM_PACKED XSSyncFileReq_t;

#ifdef _MSC_VER
#  pragma pack()
#endif


/**********************************************************************
 * XSVersion_t:
 *
 *  主版本号.子版本号.修正版本号.编译版本号
 *   1.0.2.3
 *
 *   MajorVersion.MinorVersion.RevisionNumber[.BuildNumber]
 *
 *  verstring: "1.2.3.0"
 *
 *********************************************************************/

#ifdef _MSC_VER
#  pragma pack(1)
#endif

typedef struct XSVersion_t {
    union {
        /* big endian */
        struct {
            ub1 major;
            ub1 minor;
            ub1 revision;
            ub1 buildno;
        };

        /* big endian */
        ub4 verno;
    };

    char verstring[16];
} GNUC_PACKED ARM_PACKED XSVersion_t;

#ifdef _MSC_VER
#  pragma pack()
#endif


__no_warning_unused(static)
ub4 build_version_from_string (const char * verstring, XSVersion_t * version)
{
    int i, len, ndot, dot[4];

    version->verno = 0;

    len = (int) strlen(verstring);

    if ( len >= (int) sizeof(version->verstring) ) {
        return (-1);
    }

    i = ndot = 0;
    while (i < len) {
        if (verstring[i] == '.') {
            dot[ndot++] = i;
        }

        if (ndot == 4) {
            return (-1);
        }

        ++i;
    }

    if (ndot >= 2) {
        bzero(version->verstring, sizeof(version->verstring));
        strncpy(version->verstring, verstring, dot[0]);
        version->major = (ub1) atoi(version->verstring);

        bzero(version->verstring, sizeof(version->verstring));
        strncpy(version->verstring, verstring + dot[0] + 1, dot[1] - dot[0] - 1);
        version->minor = (ub1) atoi(version->verstring);

        if (ndot == 2) {
            bzero(version->verstring, sizeof(version->verstring));
            strncpy(version->verstring, verstring + dot[1] + 1, len - dot[1] - 1);
            version->revision = (ub1) atoi(version->verstring);
        } else {
            bzero(version->verstring, sizeof(version->verstring));
            strncpy(version->verstring, verstring + dot[1] + 1, dot[2] - dot[1] - 1);
            version->revision = (ub1) atoi(version->verstring);
        }

        if (ndot == 3) {
            bzero(version->verstring, sizeof(version->verstring));
            strncpy(version->verstring, verstring + dot[2] + 1, len - dot[2] - 1);
            version->buildno = (ub1) atoi(version->verstring);
        }

        memcpy(version->verstring, verstring, len);
        version->verstring[len] = 0;

        return (ub4) BO_i32_betoh(version->verno);
    }

    return (-1);
}


__no_warning_unused(static)
const char * parse_version_to_string (ub4 verno, XSVersion_t * version)
{
    version->verno = (ub4) BO_i32_htobe(verno);

    snprintf(version->verstring, sizeof(version->verstring), "%d.%d.%d.%d",
        version->major, version->minor, version->revision, version->buildno);

    version->verstring[ sizeof(version->verstring) - 1 ] = 0;

    return version->verstring;
}


__no_warning_unused(static)
ub1 * XSConnectRequestBuild (XSConnectReq_t *req,
    char clientid[XSYNC_CLIENTID_MAXLEN],
    uint32_t magic,
    ub8 utctime,
    ub4 randnum,
    ub1 *chunk)
{
    ub4 b;
    ub8 b2;

    XSVersion_t appver;

    bzero(req, sizeof(*req));

    /* XCON */
    req->msgid = XS_MSGID_XCON.msgid;

    req->magic = magic;

    req->randnum = randnum;
    req->client_version = build_version_from_string(XSYNC_CLIENT_VERSION, &appver);
    req->client_utctime = utctime;

    memcpy(req->clientid, clientid, XSYNC_CLIENTID_MAXLEN);

    /**
     * write to send buffer
     */
    do {
        ub1 *pbuf = chunk;

        memcpy(pbuf, &req->msgid, sizeof(req->msgid));
        pbuf += sizeof(req->msgid);

        b = BO_i32_htobe(req->magic);
        memcpy(pbuf, &b, sizeof(b));
        pbuf += sizeof(b);

        b = BO_i32_htobe(req->randnum);
        memcpy(pbuf, &b, sizeof(b));
        pbuf += sizeof(b);

        memcpy(pbuf, &req->client_version, sizeof(req->client_version));
        pbuf += sizeof(req->client_version);

        b2 = BO_i64_htole(req->client_utctime);
        memcpy(pbuf, &b2, sizeof(b2));
        pbuf += sizeof(b2);

        memcpy(pbuf, req->clientid, XSYNC_CLIENTID_MAXLEN);
        pbuf += XSYNC_CLIENTID_MAXLEN;

        req->crc32_checksum = (ub4) crc32(0L, (const unsigned char *) chunk, XS_CONNECT_REQ_SIZE - sizeof(req->crc32_checksum));

        b = BO_i32_htobe(req->crc32_checksum);
        memcpy(pbuf, &b, sizeof(b));
        pbuf += sizeof(b);

        assert(pbuf - chunk == XS_CONNECT_REQ_SIZE);
    } while(0);

    return chunk;
}


__no_warning_unused(static)
inline XS_BOOL XSConnectRequestParse (ub1 *chunk, XSConnectReq_t *req)
{
    ub1 * pbuf = chunk;

    ub4 crcsum = (ub4) crc32(0L, (const unsigned char *) chunk, XS_CONNECT_REQ_SIZE - sizeof(ub4));

    req->crc32_checksum = (ub4) BO_bytes_betoh_i32(chunk + XS_CONNECT_REQ_SIZE - sizeof(ub4));

    if (crcsum != req->crc32_checksum) {
        return XS_FALSE;
    }

    memcpy(&req->msgid, pbuf, sizeof(req->msgid));
    pbuf += sizeof(ub4);

    req->magic = (ub4) BO_bytes_betoh_i32(pbuf);
    pbuf += sizeof(ub4);

    req->randnum = (ub4) BO_bytes_betoh_i32(pbuf);
    pbuf += sizeof(ub4);

    memcpy(&req->client_version, pbuf, sizeof(ub4));
    pbuf += sizeof(ub4);

    req->client_utctime = (ub8) BO_bytes_betoh_i64(pbuf);
    pbuf += sizeof(ub8);

    memcpy(req->clientid, pbuf, XSYNC_CLIENTID_MAXLEN);
    pbuf += XSYNC_CLIENTID_MAXLEN;

    /* crcsum */
    pbuf += sizeof(ub4);

    assert(pbuf - chunk == XS_CONNECT_REQ_SIZE);

    return XS_TRUE;
}


__no_warning_unused(static)
inline const char * XSConnectRequestPrint (const XSConnectReq_t *req, char *buffer, int bufsize)
{
    char clientid[XSYNC_CLIENTID_MAXLEN + 1];
    memcpy(clientid, req->clientid, XSYNC_CLIENTID_MAXLEN);
    clientid[XSYNC_CLIENTID_MAXLEN] = 0;

    XSVersion_t ver;

    snprintf(buffer, bufsize, "\n  clientid='%s'\n  msgid='%c%c%c%c'\n  magic=%d\n  randnum=%d\n  version=%s(%d)\n  utctime=%ju",
        clientid,
        req->head[0], req->head[1], req->head[2], req->head[3],
        req->magic,
        req->randnum,
        parse_version_to_string(req->client_version, &ver),
        req->client_version,
        req->client_utctime);

    buffer[bufsize - 1] = 0;

    return buffer;
}


__no_warning_unused(static)
ub1 * XSConnectReplyRejectBuild (XSConnectReply_t *reply,
    ub4 reject_code,
    ub1 *chunk)
{
    ub4 b;
    ub1 *pbuf = chunk;

    bzero(reply, sizeof(*reply));

    reply->msgid = XS_MSGID_XCON.msgid;

    BO_swap_bytes(&reply->msgid, sizeof(reply->msgid));
    reply->reject_code = reject_code;

    memcpy(pbuf, &reply->msgid, sizeof(reply->msgid));
    pbuf += sizeof(reply->msgid);

    b = BO_i32_htobe(reply->reject_code);
    memcpy(pbuf, &b, sizeof(b));
    pbuf += sizeof(b);

    return chunk;
}


__no_warning_unused(static)
ub1 * XSConnectReplyAcceptBuild (XSConnectReply_t *reply,
    ub4 magic,
    ub8 utctime,
    ub8 session,
    ub1 *chunk)
{
    XSVersion_t appver;

    bzero(reply, sizeof(*reply));

    reply->msgid = XS_MSGID_XCON.msgid;

    reply->magic = 0;

    reply->server_version = build_version_from_string(XSYNC_SERVER_VERSION, &appver);
    reply->bitflags = 0;

    reply->server_utctime = utctime;
    reply->session = session;

    reply->paramslen = 0;

    //reply->crc32_checksum = ();

    return chunk;
}

#if defined(__cplusplus)
}
#endif

#endif /* XSYNC_PROTOCOL_H_ */
