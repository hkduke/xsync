/***********************************************************************
* Copyright (c) 2018 pepstack, pepstack.com
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*   claim that you wrote the original software. If you use this software
*   in a product, an acknowledgment in the product documentation would be
*   appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not be
*   misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
***********************************************************************/

/**
 * xsync-protocol.h
 *   Define communication format of messages between xsync-client and xsync-server
 *
 * author: master@pepstack.com
 *
 * create: 2018-02-10
 * update: 2018-02-13
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


/**
 * xsync_conn_req_t
 *
 *   This is the 1st message sent by client to server so as to
 *     establish a new socket connection.
 *
 *   The xsync_newconn_t makes up with a fix head of 64 bytes.
 *
 *        4 bytes      |       4 bytes
 * --------------------+--------------------
 *  0      XSYN        |        MAGIC      7
 * --------------------+--------------------
 *  8     Version      |      TimeStamp   15
 * --------------------+--------------------
 *  16          CLIENTID 40 bytes
 *                 ...   ...              55
 * --------------------+--------------------
 *  56  Random Number  |        CRC32     63
 * -----------------------------------------
 *  64
 */

#ifdef _MSC_VER
#  pragma pack(1)
#endif

typedef struct xsync_conn_req_t
{
    union {
        struct {
            ub1 msg_id[4];          /* XSYN */
            ub4 magic;

            ub4 client_version;     /* xsync-client version */
            ub4 client_utctime;

            /* 40 characters length id */
            ub1 clientid[XSYNC_CLIENTID_MAXLEN];

            ub4 randnum;
            ub4 crc32;
        };

        ub1 conn_req[64];
    };

    ub1 msg_buffer[64];
} GNUC_PACKED ARM_PACKED xsync_conn_req_t;

#ifdef _MSC_VER
#  pragma pack()
#endif


/***********************************************************************
 *
 *
 *
 *
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
            ub4 crc32;
        };

        ub1 conn_req[64];
    };

    ub1 req_buffer[64];
} GNUC_PACKED ARM_PACKED XSYNC_ConnectRequest;

#ifdef _MSC_VER
#  pragma pack()
#endif


#ifdef _MSC_VER
#  pragma pack(1)
#endif
typedef struct XSYNC_ConnectReply
{
    ub4 msgid;                       /* 接受或指派备用服务器 */

    union {
        struct {
            ub4 reject_code;         /* != 0 */
        };

        /** accept reply */
        struct {
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

            ub4 crc32;
        };
    };
} GNUC_PACKED ARM_PACKED XSYNC_ConnectReply;
#ifdef _MSC_VER
#  pragma pack()
#endif




__no_warning_unused(static)
inline void XSYNC_connect_request_init (xsync_conn_req_t *msg, uint32_t magic, char clientid[40])
{
}


#if defined(__cplusplus)
}
#endif

#endif /* XSYNC_PROTOCOL_H_ */
