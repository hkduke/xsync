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

#ifndef CLIENT_API_H_INCLUDED
#define CLIENT_API_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#define LOGGER_CATEGORY_NAME  XSYNC_CLIENT_APPNAME
#include "../common/log4c_logger.h"

#include "../xsync-error.h"
#include "../xsync-config.h"


typedef struct xs_client_t              * XS_client;
typedef struct xs_server_opts_t         * XS_server_opts;
typedef struct xs_watch_path_t          * XS_watch_path;
typedef struct xs_watch_entry_t         * XS_watch_entry;
typedef struct xs_watch_event_t         * XS_watch_event;

typedef XS_RESULT (*list_watch_path_cb_t)(XS_watch_path wp, void * data);


typedef struct clientapp_opts
{
    // singleton

    char *startcmd;

    int isdaemon;

    int threads;
    int queues;

    int force_watch;

    char config[XSYNC_PATHFILE_MAXLEN + 1];
} clientapp_opts;


/**
 * XS_client application api
 */
extern XS_RESULT XS_client_create (clientapp_opts *opts, XS_client *outClient);

extern XS_VOID XS_client_release (XS_client *pClient);

extern int XS_client_lock (XS_client client);

extern void XS_client_unlock (XS_client client);

extern XS_VOID XS_client_bootstrap (XS_client client);


/**
 * XS_client configuration api
 *
 * implemented in:
 *   client_conf.c
 */
extern XS_RESULT XS_client_conf_load_xml (XS_client client, const char *xmlfile);

extern XS_RESULT XS_client_conf_save_xml (XS_client client, const char *xmlfile);

extern XS_RESULT XS_client_conf_load_ini (XS_client client, const char *inifile);

extern XS_RESULT XS_client_conf_save_ini (XS_client client, const char *inifile);

extern XS_RESULT XS_client_conf_from_watch (XS_client client, const char *watchdir);


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
