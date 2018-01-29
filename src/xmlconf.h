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

/**
 * xmlconf.h
 *
 * author: master@pepstack.com
 *
 * refer:
 *   - https://www.systutorials.com/docs/linux/man/3-mxml/
 *
 * create: 2016-07-06
 * update: 2018-01-29
 */
#ifndef XML_CONF_H_INCLUDED
#define XML_CONF_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include <mxml.h>

/*
#define CHECK_NODE_ERR(node, errnum) \
    if (!node) { \
        err = (errnum); \
        goto ERROR_RET; \
    }

__attribute__((used))
static int safe_stricmp(const char *a, const char *b)
{
    if (!a || !b) {
        return -1;
    }
    else {
        int ca, cb;
        do {
            ca = (unsigned char) *a++;
            cb = (unsigned char) *b++;
            ca = tolower(toupper(ca));
            cb = tolower(toupper(cb));
        } while (ca == cb && ca != '\0');

        return ca - cb;
    }
}

__attribute__((used))
static int parse_bool_asint(const char * boolValue, int *intValue)
{
    char value[10];
    int tolen;

    *intValue = 0;

    tolen = snprintf(value, sizeof(value), "%s", boolValue);

    if (tolen > 0 && tolen < sizeof(value)) {
        const char * true_array[] = {"1", "yes", "y", "true", "t", "enabled", "enable", "valid", 0};
        const char * false_array[] = {"0", "no", "n", "false", "f", "disabled", "disable", "invalid", 0};

        strlwr(value);

        if (str_in_array(value, true_array)) {
            *intValue = TRUE;
            return TRUE;
        } else if (str_in_array(value, false_array)) {
            *intValue = FALSE;
            return TRUE;
        }
    }

    return FALSE;
}

__attribute__((used))
static int parse_second_ratio(char * timeuint, float *second_ratio)
{
    float sr = 1.0f;

    const char * ms_array[] = {"millisecond", "msec", "ms", 0};
    const char * s_array[] = {"default", "second", "sec", "s", "", 0};
    const char * m_array[] = {"minute", "min", "m", 0};
    const char * h_array[] = {"hour", "hr", "h", 0};

    strlwr(timeuint);

    if (str_in_array(timeuint, s_array)) {
        sr = 1.0f;
    } else if (str_in_array(timeuint, ms_array)) {
        sr = 0.001f;
    } else if (str_in_array(timeuint, m_array)) {
        sr = 60.0f;
    } else if (str_in_array(timeuint, h_array)) {
        sr = 3600.0f;
    } else {
        return FALSE;
    }

    *second_ratio = sr;
    return TRUE;
}

__attribute__((used))
static int parse_byte_ratio(char * sizeuint, float *byte_ratio)
{
    float br;

    const char * mb_array[] = {"megabyte", "mbyte", "mb", "m", 0};
    const char * kb_array[] = {"kilobyte", "kbyte", "kb", "k", 0};
    const char * gb_array[] = {"gigabyte", "gbyte", "gb", "g", 0};
    const char * b_array[] = {"default", "byte", "b", "", 0};

    strlwr(sizeuint);

    if (str_in_array(sizeuint, b_array)) {
        br = 1.0f;
    } else if (str_in_array(sizeuint, kb_array)) {
        br = 1024;
    } else if (str_in_array(sizeuint, mb_array)) {
        br = 1024*1024;
    } else if (str_in_array(sizeuint, gb_array)) {
        br = 1024*1024*1024;
    } else {
        return FALSE;
    }

    *byte_ratio = br;
    return TRUE;
}

__attribute__((used))
static int MxmlNodeGetStringAttr(mxml_node_t *node, const char * nodeName, char * strValue, int sizeValue)
{
    const char * szAttrTemp;
    int tolen;

    *strValue = 0;

    if (! node) {
        return FALSE;
    }

    szAttrTemp = mxmlElementGetAttr(node, nodeName);
    if (! szAttrTemp) {
        return FALSE;
    }

    tolen = snprintf(strValue, sizeValue, "%s", szAttrTemp);
    if (tolen > 0 && tolen < sizeValue) {
        return TRUE;
    }

    return FALSE;
}

__attribute__((used))
static int MxmlNodeGetIntegerAttr(mxml_node_t *node, const char * nodeName, int * intValue)
{
    const char * szAttrTemp;

    *intValue = 0;

    if (! node) {
        return FALSE;
    }

    szAttrTemp = mxmlElementGetAttr(node, nodeName);
    if (! szAttrTemp || ! strcmp(szAttrTemp, "") || ! strcmp(szAttrTemp, "(null)")) {
        return FALSE;
    } else if (parse_bool_asint(szAttrTemp, intValue)) {
        return TRUE;
    }

    *intValue = atoi(szAttrTemp);
    return TRUE;
}


typedef int (*ListNodeCallback)(mxml_node_t *, void *);

__attribute__((used))
static int MxmlNodeListChildNodes(mxml_node_t *parentNode, const char * childNodeName, ListNodeCallback nodeCallback, void *param)
{
    mxml_node_t * childNode;

    for (childNode = mxmlFindElement(parentNode, parentNode, childNodeName, 0, 0, MXML_DESCEND);
        childNode != 0;
        childNode = mxmlFindElement(childNode, parentNode, childNodeName, 0, 0, MXML_DESCEND)) {

        if (! nodeCallback(childNode, param)) {
            return FALSE;
        }
    }

    return TRUE;
}
*/

#if defined(__cplusplus)
}
#endif

#endif /* XML_CONF_H_INCLUDED */
