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
 * update: 2018-02-12
 */

#ifndef XSYNC_PROTOCOL_H_
#define XSYNC_PROTOCOL_H_

#if defined(__cplusplus)
extern "C"
{
#endif

#include "xsync-config.h"

#include "./common/byteorder.h"
#include "./common/threadlock.h"


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
 * xsync_newconn_t
 *
 *   This is the 1st message sent by client to server so as to
 *     establish a new socket connection.
 *
 *   The xsync_newconn_t makes up with a fix head of 64 bytes.
 *
 *        4 bytes      |       4 bytes
 * --------------------+--------------------
 *  0      XSNC        |        MAGIC      7
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


typedef struct xsync_newconn_t
{
    union {
        struct {
            /* XSYN */
            byte_t tagid[4];
            uint32_t magic;

            /* xsync-client version */
            uint32_t version;
            uint32_t timestamp;

            /* 40 characters length id */
            byte_t clientid[XSYNC_CLIENTID_MAXLEN];

            uint32_t randnum;
            uint32_t crc32;
        };

        char msgconn_buf[64];
    };
} GNUC_PACKED ARM_PACKED xsync_newconn_t;

#ifdef _MSC_VER
#  pragma pack()
#endif


__no_warning_unused(static)
inline void xsync_newconn_init (xsync_newconn_t *msg)
{
    assert(sizeof(*msg) == 64);
}


#if defined(__cplusplus)
}
#endif

#endif /* XSYNC_PROTOCOL_H_ */
