/**
 * logger.h
 *   - A wrapper for LOG4C
 *
 * 2014-08-01: init created
 *
 * 2015-11-17: add color output
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
 *
 * Linux time commands:
 *   http://blog.sina.com.cn/s/blog_4171e80d01010rtr.html
 */
#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#include <log4c.h>

#ifndef LOGGER_BUF_LEN
    #define LOGGER_BUF_LEN  1020
#endif


#ifdef LOGGER_CATEGORY_NAME
    #define LOGGER_CATEGORY_NAME_REAL  LOGGER_CATEGORY_NAME
#else
    #define LOGGER_CATEGORY_NAME_REAL  "root"
#endif

/* #define LOGGER_COLOR_OUTPUT */

static log4c_category_t * logger_get_cat (int priority)
{
    log4c_category_t * cat = log4c_category_get (LOGGER_CATEGORY_NAME_REAL);

    if (cat && log4c_category_is_priority_enabled (cat, priority)) {
        return cat;
    } else {
        return 0;
    }
}


static void logger_write (log4c_category_t *cat, int priority,
    const char *file, int line, const char *func, const char * msg)
{
    log4c_category_log (cat, priority, "(%s:%d) <%s> %s", file, line, func, msg);
}


static void LOGGER_INIT ()
{
    if (0 != log4c_init()) {
        printf("\n**** log4c_init(%s) failed.\n", LOGGER_CATEGORY_NAME_REAL);
        printf("\n* This error might occur on Redhat Linux if you had installed expat before log4c.\n");
        printf("\n* To solve this issue you can uninstall both log4c and expat and reinstall them.\n");
        printf("\n* Please make sure log4c must be installed before expat installation on Redhat.\n");
        printf("\n* The other reasons cause failure are not included here, please refer to the manual.\n");
    } else {
        printf("\n* log4c_init(%s) success.\n", LOGGER_CATEGORY_NAME_REAL);
    }
}


static void LOGGER_FINI ()
{
    printf("\n* log4c_fini.\n");
    log4c_fini();
}


#if defined(LOGGER_TRACE_UNKNOWN)
#   define LOGGER_UNKNOWN(message, args...)    do {} while (0)
#else
    #define LOGGER_UNKNOWN(message, args...)  \
        do { \
            log4c_category_t * cat = logger_get_cat (LOG4C_PRIORITY_UNKNOWN); \
            if (cat) { \
                char buf[LOGGER_BUF_LEN+1]; \
                snprintf (buf, LOGGER_BUF_LEN, message, ##args); \
                buf[LOGGER_BUF_LEN] = 0; \
                logger_write (cat, LOG4C_PRIORITY_UNKNOWN, __FILE__, __LINE__, __FUNCTION__, buf); \
            } \
        } while (0)
#endif


#if defined(LOGGER_TRACE_DISABLED)
#   define LOGGER_TRACE(message, args...)    do {} while (0)
#else
#   define LOGGER_TRACE(message, args...)  \
    do { \
        log4c_category_t * cat = logger_get_cat (LOG4C_PRIORITY_TRACE); \
        if (cat) { \
            char buf[LOGGER_BUF_LEN+1]; \
            snprintf(buf, LOGGER_BUF_LEN, message, ##args); \
            buf[LOGGER_BUF_LEN] = 0; \
            logger_write (cat, LOG4C_PRIORITY_TRACE, __FILE__, __LINE__, __FUNCTION__, buf); \
        } \
    } while (0)
#endif


#if defined(LOGGER_DEBUG_DISABLED)
#   define LOGGER_DEBUG(message, args...)    do {} while (0)
#else
#if defined(LOGGER_COLOR_OUTPUT)
    #define LOGGER_DEBUG(message, args...)  \
        do { \
            log4c_category_t * cat = logger_get_cat (LOG4C_PRIORITY_DEBUG); \
            if (cat) { \
                char buf[LOGGER_BUF_LEN + 1]; \
                char *psz = buf; \
                psz = buf + snprintf(psz, 20, "\033[36m"); \
                psz = psz + snprintf(psz, LOGGER_BUF_LEN - 20, message, ##args); \
                snprintf(psz, 20, "\033[0m"); \
                buf[LOGGER_BUF_LEN] = 0; \
                logger_write (cat, LOG4C_PRIORITY_DEBUG, __FILE__, __LINE__, __FUNCTION__, buf); \
            } \
        } while (0)
#else
    #define LOGGER_DEBUG(message, args...)  \
        do { \
            log4c_category_t * cat = logger_get_cat (LOG4C_PRIORITY_DEBUG); \
            if (cat) { \
                char buf[LOGGER_BUF_LEN+1]; \
                snprintf (buf, LOGGER_BUF_LEN, message, ##args); \
                buf[LOGGER_BUF_LEN] = 0; \
                logger_write (cat, LOG4C_PRIORITY_DEBUG, __FILE__, __LINE__, __FUNCTION__, buf); \
            } \
        } while (0)
#endif
#endif


#if defined(LOGGER_WARN_DISABLED)
#   define LOGGER_WARN(message, args...)    do {} while (0)
#else
#if defined(LOGGER_COLOR_OUTPUT)
    #define LOGGER_WARN(message, args...)  \
        do { \
            log4c_category_t * cat = logger_get_cat (LOG4C_PRIORITY_WARN); \
            if (cat) { \
                char buf[LOGGER_BUF_LEN + 1]; \
                char *psz = buf; \
                psz = buf + snprintf(psz, 20, "\033[33m"); \
                psz = psz + snprintf(psz, LOGGER_BUF_LEN - 20, message, ##args); \
                snprintf(psz, 20, "\033[0m"); \
                buf[LOGGER_BUF_LEN] = 0; \
                logger_write (cat, LOG4C_PRIORITY_WARN, __FILE__, __LINE__, __FUNCTION__, buf); \
            } \
        } while (0)
#else
    #define LOGGER_WARN(message, args...)  \
        do { \
            log4c_category_t * cat = logger_get_cat (LOG4C_PRIORITY_WARN); \
            if (cat) { \
                char buf[LOGGER_BUF_LEN+1]; \
                snprintf (buf, LOGGER_BUF_LEN, message, ##args); \
                buf[LOGGER_BUF_LEN] = 0; \
                logger_write (cat, LOG4C_PRIORITY_WARN, __FILE__, __LINE__, __FUNCTION__, buf); \
            } \
        } while (0)
#endif
#endif


#if defined(LOGGER_INFO_DISABLED)
    #define LOGGER_INFO(message, args...)    do {} while (0)
#else
#if defined(LOGGER_COLOR_OUTPUT)
    #define LOGGER_INFO(message, args...)  \
        do { \
            log4c_category_t * cat = logger_get_cat (LOG4C_PRIORITY_INFO); \
            if (cat) { \
                char buf[LOGGER_BUF_LEN + 1]; \
                char *psz = buf; \
                psz = buf + snprintf(psz, 20, "\033[32m"); \
                psz = psz + snprintf(psz, LOGGER_BUF_LEN - 20, message, ##args); \
                snprintf(psz, 20, "\033[0m"); \
                buf[LOGGER_BUF_LEN] = 0; \
                logger_write (cat, LOG4C_PRIORITY_INFO, __FILE__, __LINE__, __FUNCTION__, buf); \
            } \
        } while (0)
#else
    #define LOGGER_INFO(message, args...)  \
        do { \
            log4c_category_t * cat = logger_get_cat (LOG4C_PRIORITY_INFO); \
            if (cat) { \
                char buf[LOGGER_BUF_LEN+1]; \
                snprintf (buf, LOGGER_BUF_LEN, message, ##args); \
                buf[LOGGER_BUF_LEN] = 0; \
                logger_write (cat, LOG4C_PRIORITY_INFO, __FILE__, __LINE__, __FUNCTION__, buf); \
            } \
        } while (0)
#endif
#endif


#if defined(LOGGER_ERROR_DISABLED)
    #define LOGGER_ERROR(message, args...)    do {} while (0)
#else
#if defined(LOGGER_COLOR_OUTPUT)
    #define LOGGER_ERROR(message, args...)    \
        do { \
            log4c_category_t * cat = logger_get_cat (LOG4C_PRIORITY_ERROR); \
            if (cat) { \
                char buf[LOGGER_BUF_LEN + 1]; \
                char *psz = buf; \
                psz = buf + snprintf(psz, 20, "\033[31m"); \
                psz = psz + snprintf(psz, LOGGER_BUF_LEN - 20, message, ##args); \
                snprintf(psz, 20, "\033[0m"); \
                buf[LOGGER_BUF_LEN] = 0; \
                logger_write (cat, LOG4C_PRIORITY_ERROR, __FILE__, __LINE__, __FUNCTION__, buf); \
            } \
        } while (0)
#else
    #define LOGGER_ERROR(message, args...)  \
        do { \
            log4c_category_t * cat = logger_get_cat (LOG4C_PRIORITY_ERROR); \
            if (cat) { \
                char buf[LOGGER_BUF_LEN+1]; \
                snprintf (buf, LOGGER_BUF_LEN, message, ##args); \
                buf[LOGGER_BUF_LEN] = 0; \
                logger_write (cat, LOG4C_PRIORITY_ERROR, __FILE__, __LINE__, __FUNCTION__, buf); \
            } \
        } while (0)
#endif
#endif


#if defined(LOGGER_FATAL_DISABLED)
    #define LOGGER_FATAL(message, args...)    do {} while (0)
#else
#if defined(LOGGER_COLOR_OUTPUT)
    #define LOGGER_FATAL(message, args...)  \
        do { \
            log4c_category_t * cat = logger_get_cat (LOG4C_PRIORITY_FATAL); \
            if (cat) { \
                char buf[LOGGER_BUF_LEN + 1]; \
                char *psz = buf; \
                psz = buf + snprintf(psz, 20, "\033[47;31m"); \
                psz = psz + snprintf(psz, LOGGER_BUF_LEN - 20, message, ##args); \
                snprintf(psz, 20, "\033[0m"); \
                buf[LOGGER_BUF_LEN] = 0; \
                logger_write (cat, LOG4C_PRIORITY_FATAL, __FILE__, __LINE__, __FUNCTION__, buf); \
            } \
        } while (0)
#else
    #define LOGGER_FATAL(message, args...)  \
        do { \
            log4c_category_t * cat = logger_get_cat (LOG4C_PRIORITY_FATAL); \
            if (cat) { \
                char buf[LOGGER_BUF_LEN+1]; \
                snprintf (buf, LOGGER_BUF_LEN, message, ##args); \
                buf[LOGGER_BUF_LEN] = 0; \
                logger_write (cat, LOG4C_PRIORITY_FATAL, __FILE__, __LINE__, __FUNCTION__, buf); \
            } \
        } while (0)
#endif
#endif


#endif /* LOGGER_H_INCLUDED */
