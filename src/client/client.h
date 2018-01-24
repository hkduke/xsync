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

#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef APP_NAME
#  define APP_NAME  "xsync-client"
#endif

#ifndef APP_VERSION
#  define APP_VERSION  "0.0.1"
#endif

#ifndef LOGGER_CATEGORY_NAME
#  define LOGGER_CATEGORY_NAME  APP_NAME
#endif


#include "xsyncdef.h"
#include "client_api.h"


/**
 * print usage for app
 *
 * ------ color output ------
 * 30: black
 * 31: red
 * 32: green
 * 33: yellow
 * 34: blue
 * 35: purple
 * 36: dark green
 * 37: white
 * --------------------------
 * "\033[31m RED   \033[0m"
 * "\033[32m GREEN \033[0m"
 * "\033[33m YELLOW \033[0m"
 */

__attribute__((unused))
static void print_usage(void)
{
    printf("\033[47;35m* %s, Version: %s, Build: %s %s\033[0m\n",
        APP_NAME, APP_VERSION, __DATE__, __TIME__);
    printf("\033[35mUsage:\033[0m %s [Options]\n", APP_NAME);
    printf("\t\033[35mextremely synchronize files among servers.\033[0m\n");
    printf("\033[35mOptions:\033[0m\n"
        "\t-h, --help                  \033[35mdisplay help messages\033[0m\n"
        "\t-V, --version               \033[35mprint version information\033[0m\n"
        "\t-v, --verbose               \033[35moutput verbose messages\033[0m\n"
        "\n"
        "\t-C, --config=PATHFILE       \033[35mspecify path file to conf. '../conf/%s.conf' (default)\033[0m\n"
        "\t-O, --log4c-rcpath=PATH     \033[35mspecify path of log4crc file. '../conf/' (default)\033[0m\n"
        "\t-P, --priority=<PRIORITY>   \033[35moverwrite priority in log4crc, available PRIORITY:\033[0m\n"
        "\t                                      \033[35m'fatal'\033[0m\n"
        "\t                                      \033[35m'error' - used in stable release stage\033[0m\n"
        "\t                                      \033[35m'warn'\033[0m\n"
        "\t                                      \033[35m'info'  - used in release stage\033[0m\n"
        "\t                                      \033[35m'debug' - used only in devel\033[0m\n"
        "\t                                      \033[35m'trace' - show all details\033[0m\n"
        "\t-A, --appender=<APPENDER>   \033[35moverwrite appender in log4crc, available APPENDER:\033[0m\n"
        "\t                                      \033[35m'default' - using appender specified in log4crc\033[0m\n"
        "\t                                      \033[35m'stdout' - using appender stdout\033[0m\n"
        "\t                                      \033[35m'stderr' - using appender stderr\033[0m\n"
        "\t                                      \033[35m'syslog' - using appender syslog\033[0m\n"
        "\n"
        "\t-D, --daemon                \033[35mrun as daemon process\033[0m\n"
        "\t-K, --kill                  \033[35mkill all processes for this program\033[0m\n"
        "\t-L, --list                  \033[35mlist of pids for this program\033[0m\n"
        "\n"
        "\t-m, --md5=FILE              \033[35mmd5sum on given FILE\033[0m\n"
        "\t-r, --regexp=PATTERN        \033[35muse pattern for matching on <express>\033[0m\n"
        "\n"
        "\033[47;35m* COPYRIGHT (c) 2014-2020 PEPSTACK.COM, ALL RIGHTS RESERVED.\033[0m\n", APP_NAME);
}


__attribute__((unused))
static int check_file_mode (const char * file, int mode /* R_OK, W_OK */)
{
    if (0 == access(file, mode)) {
        return 0;
    } else {
        perror(file);
        return -1;
    }
}


/**
 * MUST equal with:
 *   $ md5sum $file
 *   $ openssl md5 $file
 */
__attribute__((unused))
static int md5sum_file (const char * filename, char * buf, size_t bufsize)
{
    FILE * fd;

    fd = fopen(filename, "rb");
    if ( ! fd ) {
        return (-1);
    } else {
        MD5_CTX  ctx;
        int err = 1;

        MD5_Init(&ctx);

        for (;;) {
            size_t len = fread(buf, 1, bufsize, fd);

            if (len == 0) {
                err = ferror(fd);

                if (! err) {
                    unsigned char md5[16];

                    MD5_Final(md5, &ctx);

                    for (len = 0; len < sizeof(md5); ++len) {
                        snprintf(buf + len * 2, 3, "%02x", md5[len]);
                    }
                }

                buf[32] = 0;

                fclose(fd);
                break;
            }

            MD5_Update(&ctx , (const void *) buf, len);
        }

        return err;
    }
}


int handle_inotify_event (struct inotify_event * event, xs_client_t * client);

#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_H_INCLUDED */
