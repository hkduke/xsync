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

#ifndef SERVER_OPTS_H_INCLUDED
#define SERVER_OPTS_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../xsync-error.h"
#include "../xsync-config.h"

#include "../common/common_util.h"
#include "../common/sockapi.h"


typedef struct xs_server_opts_t
{
    union {
        int magic;
        int sidmax;
    };

    char host[XSYNC_HOSTNAME_MAXLEN + 1];
    ushort port;

    struct sockconn_opts sockopts;
} xs_server_opts_t;


__attribute__((used))
static int server_opt_init (XS_server_opts opts, int sid, const char * sidfile)
{
    bzero(opts, sizeof(xs_server_opts_t));

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
