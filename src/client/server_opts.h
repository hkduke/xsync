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
 * @file: server_opts.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.1.5
 *
 * @create: 2018-01-25
 *
 * @update: 2018-08-10 18:11:59
 */

#ifndef SERVER_OPTS_H_INCLUDED
#define SERVER_OPTS_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../xsync-error.h"
#include "../xsync-config.h"

#include "../common/sockapi.h"
#include "../common/common_util.h"


typedef struct xs_server_opts
{
    union {
        uint32_t magic;
        int sidmax;
    };

    char smagic[4];

    char host[XSYNC_HOSTNAME_MAXLEN + 1];
    char sport[XSYNC_PORTNUMB_MAXLEN + 1];

    int port;

    struct sockconn_opts sockopts;
} xs_server_opts;


__no_warning_unused(static)
int XS_server_opts_init (xs_server_opts * opts, int sid, const char * sidfile)
{
    bzero(opts, sizeof(xs_server_opts));

    FILE * fp = fopen(sidfile, "r");

    if (fp) {
        char buf[256] = {0};

        if (fgets(buf, sizeof(buf), fp)) {
            if (strchr(buf, '\n')) {
                *strchr(buf, '\n') = 0;
            }

            char * p = strchr(buf, ':');
            if (p) {
                char * m = strchr(p, '#');

                if (m) {
                    *p++ = 0;
                    *m++ = 0;

                    strcpy(opts->host, buf);
                    opts->port = (ushort) atoi(p);
                    opts->magic = (int) atoi(m);

                    // TODO:
                    sockconn_opts_init_default(&opts->sockopts);

                    // 成功: success
                    LOGGER_TRACE("sid=%d host=%s port=%d magic=%d (%s)", sid, opts->host, opts->port, opts->magic, sidfile);
                    return 0;
                }
            }
        }

        fclose(fp);
    }

    // 失败: error
    LOGGER_ERROR("sid=%d error(%d): %s. (%s)", sid, errno, strerror(errno), sidfile);
    return (-1);
}


#if defined(__cplusplus)
}
#endif

#endif /* SERVER_OPTS_H_INCLUDED */
