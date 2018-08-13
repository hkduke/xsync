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
 * @version: 0.0.4
 *
 * @create: 2018-01-29
 *
 * @update: 2018-08-10 18:11:59
 */

/***********************************************************************
 *     client-request   |  server-reply
 * ---------------------+--------------------------------------------
 * 1) new_connect       |  accept  / reject
 *                      |
 * 2) start_fstream     |  accept  / reject
 * 3) data1, data2, ... |  recv: donot reply
 *                      |  stop: send OOB to client(catch it by SIGURG)
 *                      |     http://www.cnblogs.com/c-slmax/p/5553857.html
 * 4) end_fstream       |  accept
 *                      |
 * 5) del_connect       |  accept
 *                      |
 * 6) start_update      |  accept / reject
 * 7) update_msg1,2,    |
 * 8) end_update        |
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


/***********************************************************************
 * XSYNC_ConnectRequest
 *
 *   This is the 1st message sent by client to server so as to
 *     establish a new socket connection.
 *
 *   The xsync_newconn_t makes up with a fix head of 64 bytes.
 *
 *        4 bytes      |       4 bytes
 * --------------------+--------------------
 *  0      XSnn        |        MAGIC      7
 * --------------------+--------------------
 *  8     Version      |      TimeStamp   15
 * --------------------+--------------------
 *  16          CLIENTID 40 bytes
 *                 ...   ...              55
 * --------------------+--------------------
 *  56  Random Number  |        CRC32     63
 * -----------------------------------------
 *  64
 **********************************************************************/
#ifdef _MSC_VER
#  pragma pack(1)
#endif

typedef struct XSYNC_ConnectRequest
{
    union {
        struct {
            ub4 msgid;              /* XS00, XS01 */
            ub4 magic;

            ub4 client_version;     /* xsync-client version */
            ub4 client_utctime;

            /* 40 characters length id */
            ub1 clientid[XSYNC_CLIENTID_MAXLEN];

            ub4 randnum;
            ub4 crc32_checksum;
        };

        ub1 connect_request[64];
    };

    ub1 request_buffer[64];
} GNUC_PACKED ARM_PACKED XSYNC_ConnectRequest;

#ifdef _MSC_VER
#  pragma pack()
#endif


/***********************************************************************
 * XSYNC_ConnectReply
 *
 **********************************************************************/
#ifdef _MSC_VER
#  pragma pack(1)
#endif
typedef struct XSYNC_ConnectReply
{
    union {
        struct {
            ub4 reject_msgid;        /* 拒绝消息ID */
            ub4 reject_code;         /* != 0 */
        };

        /** accept reply */
        struct {
            ub4 accept_msgid;        /* 接受或指派备用服务器 */
            ub4 token;               /* token or magic */

            ub4 server_version;      /* xsync-server version */
            ub4 server_utctime;

            ub8 session_id;

            ub4 standby_port;

            union {
                struct {
                    ub4 standby_ipv4;
                    ub1 placeholder[12];
                };

                ub1 standby_ipv6[16];
            };

            ub1 lo_cipher[8];
            ub1 hi_cipher[8];

            ub4 crc32_checksum;
        };

        ub1 connect_reply[64];
    };

    ub1 reply_buffer[64];
} GNUC_PACKED ARM_PACKED XSYNC_ConnectReply;
#ifdef _MSC_VER
#  pragma pack()
#endif


/**
 * version:
 *
 * 主版本号.子版本号.修正版本号.编译版本号
 *
 *   MajorVersion.MinorVersion.RevisionNumber[.BuildNumber]
 *
 *  string: "1.2.3"
 *  ub4:    [1|2|3|0]
 */
__no_warning_unused(static)
inline ub4 version_string_to_ub4 (const char * version)
{
    ub4 vernum = 0;
    ub1 verbuf[4] = {0, 0, 0, 0};

    char tmpver[16];
    char *dot;
    int col = 4;

    strncpy(tmpver, version, sizeof(tmpver));

    tmpver[sizeof(tmpver) - 1] = 0;

    dot= strtok(tmpver, ".");

    while (dot && col-- > 0) {
        verbuf[col] = (ub1) atoi(dot);
        dot = strtok(NULL, ".");
    }

    memcpy(&vernum, verbuf, sizeof(ub4));

    return vernum;
}


__no_warning_unused(static)
inline ub1 * XSYNC_ConnectRequestBuild (XSYNC_ConnectRequest *req, uint32_t magic, ub4 utctime, ub4 randnum, char clientid[40])
{
    ub4 be;

    bzero(req, sizeof(XSYNC_ConnectRequest));

    req->connect_request[0] = 'X';
    req->connect_request[1] = 'S';
    req->connect_request[2] = '0';
    req->connect_request[3] = '0';

    req->magic = magic;

    req->client_version = version_string_to_ub4(XSYNC_CLIENT_VERSION);

    memcpy(req->clientid, clientid, 40);

    req->client_utctime = utctime;
    req->randnum = randnum;

    /**
     * write to send buffer
     */
    do {
        ub1 * pbuf = req->request_buffer;

        memcpy(pbuf, &req->msgid, sizeof(be));
        pbuf += sizeof(be);

        be = BO_i32_htobe(req->magic);
        memcpy(pbuf, &be, sizeof(be));
        pbuf += sizeof(be);

        memcpy(pbuf, &req->client_version, sizeof(req->client_version));
        pbuf += sizeof(be);

        be = BO_i32_htobe(req->client_utctime);
        memcpy(pbuf, &be, sizeof(be));
        pbuf += sizeof(be);

        memcpy(pbuf, req->clientid, sizeof(req->clientid));
        pbuf += sizeof(req->clientid);

        be = BO_i32_htobe(req->randnum);
        memcpy(pbuf, &be, sizeof(be));
        pbuf += sizeof(be);

        req->crc32_checksum = (ub4) crc32(0L, (const unsigned char *) req->request_buffer, sizeof(req->request_buffer) - sizeof(be));

        be = BO_i32_htobe(req->crc32_checksum);
        memcpy(pbuf, &be, sizeof(be));
        pbuf += sizeof(be);

        assert(pbuf - req->request_buffer == 64);
    } while(0);

    return req->request_buffer;
}


__no_warning_unused(static)
inline XS_BOOL XSYNC_ConnectRequestParse (unsigned char *req_buffer, XSYNC_ConnectRequest *req)
{
    ub4 b;
    ub1 * pbuf = req_buffer;

    b = (ub4) crc32(0L, (const unsigned char *) req_buffer, sizeof(req->request_buffer) - sizeof(b));

    req->crc32_checksum = (ub4) BO_bytes_betoh_i32(req_buffer + sizeof(req->request_buffer) - sizeof(b));

    if (b != req->crc32_checksum) {
        return XS_FALSE;
    }

    memcpy(&req->msgid, pbuf, sizeof(req->msgid));
    pbuf += sizeof(ub4);

    req->magic = (ub4) BO_bytes_betoh_i32(pbuf);
    pbuf += sizeof(ub4);

    memcpy(&req->client_version, pbuf, sizeof(ub4));
    pbuf += sizeof(ub4);

    req->client_utctime = (ub4) BO_bytes_betoh_i32(pbuf);
    pbuf += sizeof(ub4);

    memcpy(req->clientid, pbuf, sizeof(req->clientid));
    pbuf += sizeof(req->clientid);

    req->randnum = (ub4) BO_bytes_betoh_i32(pbuf);
    pbuf += sizeof(ub4);
    pbuf += sizeof(ub4);

    assert(pbuf - req_buffer == 64);

    return XS_TRUE;
}


__no_warning_unused(static)
inline void XSYNC_ConnectReplyInit (XSYNC_ConnectReply *msg)
{

}


#if defined(__cplusplus)
}
#endif

#endif /* XSYNC_PROTOCOL_H_ */
