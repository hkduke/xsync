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

#include "client.h"

int run_forever (char * xmlconf);


int main (int argc, char * argv [])
{
    int ret;

    char buff[4096];

    char xmlconf[256];
    char log4crc[256];

    char priority[20] = {0};
    char appender[60] = {0};

    __attribute__((unused)) int isdaemon = 0;
    __attribute__((unused)) int verbose = 0;

    /* command arguments */
    const struct option lopts[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'V'},
        {"verbose", no_argument, 0, 'v'},
        {"config", required_argument, 0, 'C'},
        {"log4c-rcpath", required_argument, 0, 'O'},
        {"priority", required_argument, 0, 'P'},
        {"appender", required_argument, 0, 'A'},
        {"daemon", no_argument, 0, 'D'},
        {"kill", no_argument, 0, 'K'},
        {"list", no_argument, 0, 'L'},
        {"md5", required_argument, 0, 'm'},
        {"regexp", required_argument, 0, 'r'},
        {0, 0, 0, 0}
    };

    /**
     * get default real path for xsync-client.conf
     */
    ret = getpwd(buff, sizeof(buff));
    if (! ret) {
        fprintf(stderr, "\033[31m[error: getpwd]\033[0m %s\n", buff);
        exit(-1);
    }
    *strrchr(buff, '/') = 0;
    ret = snprintf(xmlconf, sizeof(xmlconf), "%s/conf/%s.conf", buff, APP_NAME);
    if (ret < 20 || ret >= sizeof(xmlconf)) {
        fprintf(stderr, "\033[31m[error]\033[0m invalid conf path: %s\n", buff);
        exit(-1);
    }

    ret = snprintf(log4crc, sizeof(log4crc), "LOG4C_RCPATH=%s/conf/", buff);
    if (ret < 20 || ret >= sizeof(log4crc)) {
        fprintf(stderr, "\033[31m[error]\033[0m invalid log4c path: %s\n", buff);
        exit(-1);
    }

    /* parse command arguments */
    while ((ret = getopt_long(argc, argv, "DKLhVvC:O:P:A:m:r:", lopts, 0)) != EOF) {
        switch (ret) {
        case 'D':
            isdaemon = 1;
            break;

        case 'h':
            print_usage();
            exit(0);
            break;

        case 'C':
            /* overwrite default config file */
            ret = snprintf(xmlconf, sizeof(xmlconf), "%s", optarg);
            if (ret < 20 || ret >= sizeof(xmlconf)) {
                fprintf(stderr, "\033[31m[error]\033[0m specified invalid conf file: %s\n", optarg);
                exit(-1);
            }

            if (getfullpath(xmlconf, buff, sizeof(buff)) != 0) {
                fprintf(stderr, "\033[31m[error]\033[0m %s\n", buff);
                exit(-1);
            } else {
                ret = snprintf(xmlconf, sizeof(xmlconf), "%s", buff);
                if (ret < 20 || ret >= sizeof(xmlconf)) {
                    fprintf(stderr, "\033[31m[error]\033[0m invalid conf file: %s\n", buff);
                    exit(-1);
                }
            }
            break;

        case 'O':
            /* overwrite default log4crc file */
            ret = snprintf(log4crc, sizeof(log4crc), "%s", optarg);
            if (ret < 0 || ret >= sizeof(log4crc)) {
                fprintf(stderr, "\033[31m[error]\033[0m specified invalid log4c path: \033[31m%s\033[0m\n", optarg);
                exit(-1);
            }

            if (getfullpath(log4crc, buff, sizeof(buff)) != 0) {
                fprintf(stderr, "\033[31m[error]\033[0m %s\n", buff);
                exit(-1);
            } else {
                ret = snprintf(log4crc, sizeof(log4crc), "LOG4C_RCPATH=%s", buff);
                if (ret < 10 || ret >= sizeof(log4crc)) {
                    fprintf(stderr, "\033[31m[error]\033[0m invalid log4c path: %s\n", buff);
                    exit(-1);
                }
            }
            break;

        case 'P':
            ret = snprintf(priority, sizeof(priority), "%s", optarg);
            if (ret < 0 || ret >= sizeof(priority)) {
                fprintf(stderr, "\033[31m[error]\033[0m specified invalid priority: %s\n", optarg);
                exit(-1);
            }
            break;

        case 'A':
            ret = snprintf(appender, sizeof(appender), "%s", optarg);
            if (ret < 0 || ret >= sizeof(appender)) {
                fprintf(stderr, "\033[31m[error]\033[0m specified invalid appender: \033[31m%s\033[0m\n", optarg);
                exit(-1);
            }
            break;

        case 'K':
            exit(0);
            break;

        case 'L':
            exit(0);
            break;

        case 'm':
            ret = check_file_mode(optarg, R_OK);

            if (ret) {
                fprintf(stderr, "\033[31m[error: md5sum]\033[0m file not found: %s\n\n", optarg);
            } else {
                ret = md5sum_file(optarg, buff, sizeof(buff));

                if (ret == 0) {
                    fprintf(stdout, "\033[32m[success: md5sum]\033[0m %s (%s)\n", buff, optarg);
                } else {
                    fprintf(stderr, "\033[31m[error: md5sum]\033[0m file: %s\n", optarg);
                }
            }
            exit(ret);
            break;

        case 'r':
            exit(0);
            break;

        case 'V':
            fprintf(stdout, "\033[35m%s, Version: %s, Build: %s %s\033[0m\n\n", APP_NAME, APP_VERSION, __DATE__, __TIME__);
            exit(0);
            break;

        case 'v':
            verbose = 1;
            break;
        }
    }

    fprintf(stdout, "\033[34m* Default conf file  : %s\033[0m\n", xmlconf);
    fprintf(stdout, "\033[34m* Default log4c path : %s\033[0m\n", log4crc + sizeof("LOG4C_RCPATH"));
    fprintf(stdout, "\033[32m* Using conf file    : %s\033[0m\n", xmlconf);
    fprintf(stdout, "\033[32m* Using log4c path   : %s\033[0m\n", log4crc + sizeof("LOG4C_RCPATH"));

    config_log4crc(APP_NAME, log4crc, priority, appender, sizeof(appender));

    LOGGER_INIT();
    LOGGER_INFO("%s (v%s) startup ...", APP_NAME, APP_VERSION);

    ret = run_forever(xmlconf);

    LOGGER_FATAL("%s (v%s) shutdown !", APP_NAME, APP_VERSION);
    LOGGER_FINI();

    return ret;
}


int run_forever (char * xmlconf)
{
    int err;

    xs_client_t * client = 0;

    err = XS_client_create(xmlconf, &client);

    if (! err) {
        XS_client_waiting_events(client);

        XS_client_release(&client);
    }

    return err;
}


int handle_inotify_event (struct inotify_event * event, xs_client_t * client)
{
    xs_watch_path_t * wp = client_wd_table_lookup (client, event->wd);
    assert(wp && wp->watch_wd == event->wd);

    LOGGER_DEBUG("TODO: wd=%d mask=%d cookie=%d len=%d dir=%s. (%s/%s)",
        event->wd, event->mask, event->cookie, event->len, ((event->mask & IN_ISDIR) ? "yes" : "no"),
        wp->fullpath, (event->len > 0 ? event->name : "None"));

    return 0;
}
