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
 * @file: client_api.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.0.4
 *
 * @create: 2018-01-24
 *
 * @update: 2018-08-13 15:39:51
 */

#ifndef CLIENT_API_H_INCLUDED
#define CLIENT_API_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#define LOGGER_CATEGORY_NAME  XSYNC_CLIENT_APPNAME
#include "../common/log4c_logger.h"

#include "../common/threadlock.h"
#include "../common/inotiapi.h"

#include "../xsync-error.h"
#include "../xsync-config.h"
#include "../xsync-protocol.h"

#include "server_opts.h"


typedef struct xs_client_t              * XS_client;
typedef struct xs_server_conn_t         * XS_server_conn;
typedef struct xs_watch_path_t          * XS_watch_path;
typedef struct xs_watch_entry_t         * XS_watch_entry;
typedef struct xs_watch_event_t         * XS_watch_event;
typedef struct xs_path_filter_t         * XS_path_filter;


typedef XS_RESULT (*list_watch_path_cb_t)(XS_watch_path wp, void * data);


typedef struct xs_appopts_t
{
    // singleton

    char *startcmd;

    int isdaemon;
    int interactive;

    int threads;
    int queues;

    int from_watch;

    char clientid[XSYNC_CLIENTID_MAXLEN + 1];

    char diagnose_server[XSYNC_PATHFILE_MAXLEN + 1];

    char config[XSYNC_PATHFILE_MAXLEN + 1];
    char save_config[XSYNC_PATHFILE_MAXLEN + 1];
} xs_appopts_t;


/**
 * XS_client application api
 */
extern XS_RESULT XS_client_create (xs_appopts_t *opts, XS_client *outClient);

extern XS_VOID XS_client_release (XS_client *pClient);

extern int XS_client_lock (XS_client client, int try);

extern void XS_client_unlock (XS_client client);

extern int XS_client_wait_condition(XS_client client, struct timespec * timo);

extern XS_VOID XS_client_bootstrap (XS_client client);


/**
 * XS_client configuration api
 *
 * implemented in:
 *   client_conf.c
 */
extern XS_RESULT XS_client_conf_load_xml (XS_client client, const char *xmlfile);

extern XS_RESULT XS_client_conf_save_xml (XS_client client, const char *xmlfile);

extern XS_RESULT XS_client_conf_from_watch (XS_client client, const char *watch_parent);


/**
 * XS_client internal api
 */
extern XS_VOID XS_client_list_watch_paths (XS_client client, list_watch_path_cb_t list_wp_cb, void *data);

extern XS_VOID XS_client_clear_watch_paths (XS_client client);

extern XS_BOOL XS_client_add_watch_path (XS_client client, XS_watch_path wp);

extern XS_BOOL XS_client_find_watch_path (XS_client client, char *fullpath, XS_watch_path *outwp);

extern XS_BOOL XS_client_remove_watch_path (XS_client client, char *fullpath);


#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_API_H_INCLUDED */
