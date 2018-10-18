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
 * @file: inotifyapi.h
 *   inotifytools thread safe api
 *
 * @author: master@pepstack.com
 *
 * @version: 0.1.8
 *
 * @create: 2018-09-29
 *
 * @update: 2018-10-17 10:20:32
 */

#ifndef INOTIFYAPI_H_INCLUDED
#define INOTIFYAPI_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../common/common_incl.h"

/**
 * https://sourceforge.net/projects/inotify-tools/
 */
#include <inotifytools/inotify.h>
#include <inotifytools/inotifytools.h>

#define INOTI_EVENTS_MASK  (IN_DELETE | IN_DELETE_SELF | IN_CREATE | IN_MODIFY | IN_CLOSE_WRITE | IN_MOVE | IN_ONLYDIR)

// inotifyapi.c for initializing
//
extern pthread_mutex_t  __inotifyapi_mutex_lock;


#define __inotifytools_lock() \
    pthread_mutex_lock(&__inotifyapi_mutex_lock)


#define __inotifytools_unlock() \
    pthread_mutex_unlock(&__inotifyapi_mutex_lock)


__attribute__((unused))
static inline void inotifytools_cleanup_s ()
{
    __inotifytools_lock();
    inotifytools_cleanup();
    __inotifytools_unlock();
}


__attribute__((unused))
static inline int inotifytools_error_s ()
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_error();
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline char * inotifytools_event_to_str_s (int events)
{
    char *ret;
    __inotifytools_lock();
    ret = inotifytools_event_to_str(events);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline char * inotifytools_event_to_str_sep_s (int events, char sep)
{
    char *ret;
    __inotifytools_lock();
    ret = inotifytools_event_to_str_sep(events, sep);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline char * inotifytools_filename_from_wd_s (int wd)
{
    char *ret;
    __inotifytools_lock();
    ret = inotifytools_filename_from_wd(wd);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_fprintf_s (FILE *file, struct inotify_event *event, char *fmt)
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_fprintf(file, event, fmt);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_get_max_queued_events_s ()
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_get_max_queued_events();
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_get_max_user_instances_s ()
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_get_max_user_instances();
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_get_max_user_watches_s ()
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_get_max_user_watches();
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_get_num_watches_s ()
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_get_num_watches();
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_get_stat_by_filename_s (char const *filename, int event)
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_get_stat_by_filename(filename, event);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_get_stat_by_wd_s (int wd, int event)
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_get_stat_by_wd(wd, event);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_get_stat_total_s (int event)
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_get_stat_total(event);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_ignore_events_by_regex_s (char const *pattern, int flags)
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_ignore_events_by_regex(pattern, flags);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_initialize_s ()
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_initialize();
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline void inotifytools_initialize_stats_s ()
{
    __inotifytools_lock();
    inotifytools_initialize_stats();
    __inotifytools_unlock();
}


__attribute__((unused))
static inline struct inotify_event * inotifytools_next_event_s (int timeout)
{
    struct inotify_event * ret;
    __inotifytools_lock();
    ret = inotifytools_next_event(timeout);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline struct inotify_event * inotifytools_next_events_s (int timeout, int num_events)
{
    struct inotify_event * ret;
    __inotifytools_lock();
    ret = inotifytools_next_events(timeout, num_events);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_printf_s (struct inotify_event *event, char *fmt)
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_printf(event, fmt);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_remove_watch_by_filename_s (char const *filename)
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_remove_watch_by_filename(filename);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_remove_watch_by_wd_s (int wd)
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_remove_watch_by_wd(wd);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline void inotifytools_replace_filename_s (char const *oldname, char const *newname)
{
    __inotifytools_lock();
    inotifytools_replace_filename(oldname, newname);
    __inotifytools_unlock();
}


__attribute__((unused))
static inline void inotifytools_set_filename_by_filename_s (char const *oldname, char const *newname)
{
    __inotifytools_lock();
    inotifytools_set_filename_by_filename(oldname, newname);
    __inotifytools_unlock();
}


__attribute__((unused))
static inline void inotifytools_set_filename_by_wd_s (int wd, char const *filename)
{
    __inotifytools_lock();
    inotifytools_set_filename_by_wd(wd, filename);
    __inotifytools_unlock();
}


__attribute__((unused))
static inline void inotifytools_set_printf_timefmt_s (char *fmt)
{
    __inotifytools_lock();
    inotifytools_set_printf_timefmt(fmt);
    __inotifytools_unlock();
}


__attribute__((unused))
static inline int inotifytools_snprintf_s (char *out, int size, struct inotify_event *event, char *fmt)
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_snprintf(out, size, event, fmt);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_sprintf_s (char *out, struct inotify_event *event, char *fmt)
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_sprintf(out, event, fmt);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_str_to_event_s (char const *event)
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_str_to_event(event);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_str_to_event_sep_s (char const *event, char sep)
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_str_to_event_sep(event, sep);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_watch_file_s (char const *filename, int events, ino_callback_t cbfunc, void *cbarg)
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_watch_file(filename, events, cbfunc, cbarg);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_watch_files_s (char const *filenames[], int events, ino_callback_t cbfunc, void *cbarg)
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_watch_files(filenames, events, cbfunc, cbarg);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_watch_recursively_s (char const *path, int events, ino_callback_t cbfunc, void *cbarg)
{
    int ret = 0;
    __inotifytools_lock();
    if (inotifytools_wd_from_filename(path) == -1) {
        ret = inotifytools_watch_recursively(path, events, cbfunc, cbarg);
    }
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_watch_recursively_with_exclude_s (char const *path, int events, char const **exclude_list, ino_callback_t cbfunc, void *cbarg)
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_watch_recursively_with_exclude(path, events, exclude_list, cbfunc, cbarg);
    __inotifytools_unlock();
    return ret;
}


__attribute__((unused))
static inline int inotifytools_wd_from_filename_s (char const *filename)
{
    int ret;
    __inotifytools_lock();
    ret = inotifytools_wd_from_filename(filename);
    __inotifytools_unlock();
    return ret;
}


#if defined(__cplusplus)
}
#endif

#endif /* INOTIFYAPI_H_INCLUDED */
