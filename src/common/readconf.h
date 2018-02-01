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
 * readconf.h
 *   compiled ok on Linux/Windows
 *
 * author:
 *   cheungmine@gmail.com, master@pepstack.com
 *
 * create: 2012-05-21
 * update: 2018-01-31
 */

#ifndef _READCONF_H__
#define _READCONF_H__

#if defined(__cplusplus)
extern "C" {
#endif


#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifndef _MSC_VER
  #include <unistd.h>
#endif

typedef int CONF_BOOL;
#define CONF_TRUE   1
#define CONF_FALSE  0

typedef int CONF_RESULT;
#define CONF_RET_SUCCESS      (0)
#define CONF_RET_ERROR       (-1)
#define CONF_RET_OUTMEM      (-4)

#define CONF_MAX_BUFSIZE    1020
#define CONF_MAX_SECNAME     240

#define CONF_SEC_BEGIN  91 /* '[' */
#define CONF_SEC_END    93 /* ']' */
#define CONF_SEPARATOR  61 /* '=' */
#define CONF_NOTE_CHAR  35 /* '#' */
#define CONF_SEC_SEMI   58 /* ':' */


typedef struct _conf_position_t * CONF_position;

extern void* ConfMemAlloc (int numElems, int sizeElem);

extern void* ConfMemRealloc (void *oldBuffer, int oldSize, int newSize);

extern void ConfMemFree (void *pBuffer);

extern char* ConfMemCopyString (char **dst, const char *src);

extern CONF_position ConfOpenFile (const char *conf);

extern void ConfCloseFile (CONF_position cpos);

extern char *ConfGetNextPair (CONF_position cpos, char **key, char **val);

extern char *ConfGetFirstPair (CONF_position cpos, char **key, char **val);

extern const char *ConfGetSection (CONF_position cpos);

extern CONF_RESULT ConfCopySection (CONF_position cpos, char *secName);

extern int ConfReadValue (const char *confFile, const char *sectionName, const char *keyName, char *value);

extern int ConfReadValueRef (const char *confFile, const char *sectionName, const char *keyName, char **ppRefVal);

extern int ConfReadValueParsed (const char *confFile, const char *family, const char *qualifier, const char *key, char *value);

extern int ConfGetSectionList (const char *confFile, void **sectionList);

extern void ConfSectionListFree (void *sectionList);

extern char* ConfSectionListGetAt (void *sectionList, int secIndex);

extern CONF_BOOL ConfSectionGetFamily (const char *sectionName, char *family);

extern CONF_BOOL ConfSectionGetQualifier (const char *sectionName, char *qualifier);

extern int ConfSectionParse (char *sectionName, char **family, char **qualifier);

#if defined(__cplusplus)
}
#endif

#endif /* _READCONF_H__ */
