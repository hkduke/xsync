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
 * @file: file_entry.c
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.2.8
 *
 * @create: 2018-01-29
 *
 * @update: 2018-10-22 10:56:41
 */

#include "server_api.h"

#include "file_entry.h"


extern XS_VOID XS_file_entry_create (XS_file_entry *outEntry)
{
    XS_file_entry entry;

    *outEntry = 0;

    entry = (XS_file_entry) mem_alloc(1, sizeof(struct xs_file_entry_t));

    entry->wofd = -1;

    __interlock_set(&entry->in_use, 1);

    *outEntry = (XS_file_entry) RefObjectInit(entry);

    LOGGER_TRACE("entry=%p", entry);
}


extern XS_VOID XS_file_entry_release (XS_file_entry * inEntry)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inEntry, xs_file_entry_delete);
}


extern XS_BOOL XS_file_entry_not_in_use (XS_file_entry entry)
{
    int in_use = __interlock_get(&entry->in_use);

    return (in_use == 0? 1 : 0);
}
