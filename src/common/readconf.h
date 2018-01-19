/**
 * readconf.h
 *   compiled ok on Linux/Windows
 *
 * Author:
 *   cheungmine@gmail.com
 * 2012-05-21
 */
#ifndef _READCONF_H__
#define _READCONF_H__

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
#define CONF_RET_SUCCESS  (0)
#define CONF_RET_ERROR  (-1)
#define CONF_RET_BADMEMORY (-4)

#define CONF_MAX_BUFSIZE 1020
#define CONF_MAX_SECNAME 240

#define CONF_SEC_BEGIN  91 /* '[' */
#define CONF_SEC_END    93 /* ']' */
#define CONF_SEPARATOR  61 /* '=' */
#define CONF_NOTE_CHAR  35 /* '#' */
#define CONF_SEC_SEMI   58 /* ':' */

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct _conf_position_t* CONF_POSITION;

extern void* ConfMemAlloc (int numElems, int sizeElem);

extern void* ConfMemRealloc (void *oldBuffer, int oldSize, int newSize);

extern void ConfMemFree (void *pBuffer);

extern char* ConfMemCopyString (char **dst, const char *src);

extern CONF_POSITION ConfOpenFile (const char *conf);

extern void ConfCloseFile (CONF_POSITION cpos);

extern char *ConfGetNextPair (CONF_POSITION cpos, char **key, char **val);

extern char *ConfGetFirstPair (CONF_POSITION cpos, char **key, char **val);

extern const char *ConfGetSection (CONF_POSITION cpos);

extern CONF_RESULT ConfCopySection (CONF_POSITION cpos, char *secName);

extern CONF_BOOL ConfReadValue (const char *confFile,
    const char *sectionName, const char *keyName, char *value);

extern CONF_BOOL ConfReadValueParsed (const char *confFile,
    const char *family, const char *qualifier, const char *key, char *value);

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
