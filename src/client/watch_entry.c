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
 * @file: watch_entry.c
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.1.5
 *
 * @create: 2018-01-24
 *
 * @update: 2018-09-21 12:40:23
 */

#include "client_api.h"

#include "watch_entry.h"
#include "server_conn.h"
#include "watch_event.h"

#include "client_conf.h"

#include "../common/common_util.h"


extern XS_VOID XS_watch_entry_create (const XS_watch_path wp, int sid, const char *filename, XS_watch_entry *outEntry)
{
    /*
    XS_watch_entry entry;

    char nameid[20];
    int nameoff;
    ssize_t nbsize;
    int namelen;

    *outEntry = 0;

    namelen = strlen(filename);

    nameoff = snprintf(nameid, sizeof(nameid), "%d:%d/", sid, wp->watch_wd);

    nbsize = nameoff + namelen + sizeof('\0') + MD5_HASH_FIXLEN + sizeof('\0') + wp->pathsize + namelen + sizeof('\0');

    entry = (XS_watch_entry) mem_alloc(1, sizeof(struct xs_watch_entry_t) + sizeof(char) * nbsize);

    entry->nameoff = nameoff;
    entry->namelen = namelen;
    entry->pathsize = wp->pathsize;

    memcpy(xs_entry_nameid(entry), nameid, entry->nameoff);
    memcpy(xs_entry_filename(entry), filename, namelen);
    memcpy(xs_entry_fullpath(entry), wp->fullpath, wp->pathsize);
    memcpy(xs_entry_fullpath(entry) + wp->pathsize, filename, namelen);

    xs_entry_fullpath(entry)[xs_entry_path_endpos(entry)] = '/';

    entry->rofd = -1;

    entry->wd = wp->watch_wd;
    entry->sid = sid;

    entry->hash = xs_watch_entry_hash(entry->namebuf);

    entry->cretime = time(0);

    __interlock_set(&entry->in_use, 1);

    *outEntry = (XS_watch_entry) RefObjectInit(entry);

    LOGGER_TRACE("%p ('%s' hash=%d fullpath='%s')", entry, xs_entry_nameid(entry), entry->hash, xs_entry_fullpath(entry));

    */
}


extern XS_VOID XS_watch_entry_release (XS_watch_entry * inEntry)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inEntry, watch_entry_delete);
}


extern XS_BOOL XS_watch_entry_not_in_use (XS_watch_entry entry)
{
    int in_use = __interlock_get(&entry->in_use);

    return (in_use == 0? 1 : 0);
}
