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
 * readconf.c
 *   compiled ok on Linux/Windows
 *
 * author:
 *   cheungmine@gmail.com, master@pepstack.com
 *
 * create: 2012-05-21
 * update: 2018-01-31
 */

#include "readconf.h"

#include <stdint.h>


typedef struct _conf_position_t
{
    FILE *_fp;
    char  _section[CONF_MAX_SECNAME+4];
    char  _buf[CONF_MAX_BUFSIZE+2];
} conf_position_t;


__attribute__((used))
static char * trim(char *s, char c)
{
    return (*s==0)?s:(((*s!=c)?(((trim(s+1,c)-1)==s)?s:(*(trim(s+1,c)-1)=*s,*s=c,trim(s+1,c))):trim(s+1,c)));
}


__attribute__((used))
static char * ltrim (char *s, char c)
{
    while(*s!=0&&*s==c){s++;}return s;
}


__attribute__((used))
static char * rtrim (char *s, char c)
{
    char *p=s,*q=s;while(*p!=0){if(*p!=c){q=p;q++;}p++;}if(q!=s)*q=0;return s;
}


__attribute__((used))
static char * dtrim (char *s, char c)
{
    return rtrim(ltrim(s, c), c);
}


__attribute__((used))
static int readln (FILE *fp, char *strbuf, int bufsize)
{
    int  i, j;
    char ch;

    for (i=0, j=0; i<bufsize; j++) {
        if (fread(&ch, sizeof(char), 1, fp) != 1) {
            /* read error */
            if (feof(fp) != 0) {
                if (j==0) {
                    return -1;  /* end file */
                } else {
                    break;
                }
            }

            return -2;
        } else {
            /* read a char */
            if (ch==10 || ch==0) { /* 10='\n' */
                /* read a line ok */
                break;
            }

            if (ch==12 || ch==0x1A) { /* 12='\f' */
                strbuf[i++]=ch;
                break;
            }

            if (ch != 13) { /* '\r' */
                strbuf[i++]=ch;
            }
        }

        if (i==bufsize) {
            return -3; /* exceed max chars */
        }
    }

    strbuf[i] = 0;
    return i;
}


__attribute__((used))
static int splitpair (char *strbuf, char sep, char **key, char **val)
{
    char *at = strchr(strbuf, sep);
    if (! at) {
        return CONF_FALSE;
    }

    *at++ = 0;
    *key = strbuf;
    *key = dtrim(dtrim(*key, 32), 9);

    *val = at;
    *val = dtrim(dtrim(dtrim(*val, 32), 9), 34); /* '\"' */

    if (*key != 0 && *val != 0) {
        return CONF_TRUE;
    }

    return CONF_FALSE;
}


__attribute__((used))
static char** _SectionListAlloc (int numSections)
{
    char **secs = (char**) ConfMemAlloc(numSections+1, sizeof(char*));
    secs[0] = (char*) (uintptr_t) numSections;
    return secs;
}


void * ConfMemAlloc (int numElems, int sizeElem)
{
    void *p = calloc(numElems, sizeElem);
    if (!p) {
        exit(CONF_RET_OUTMEM);
    }
    return p;
}


void * ConfMemRealloc (void *oldBuffer, int oldSize, int newSize)
{
    void *newBuffer = 0;
    assert(newSize >= 0);
    if (newSize==0) {
        ConfMemFree(oldBuffer);
        return newBuffer;
    }

    if (oldSize==newSize) {
        return oldBuffer? oldBuffer : ConfMemAlloc(1, newSize);
    }

    newBuffer = realloc(oldBuffer, newSize);
    if (!newBuffer) {
        exit(CONF_RET_OUTMEM);
    }

    if (newSize > oldSize) {
        bzero((char*)newBuffer+oldSize, newSize-oldSize);
    }
    return newBuffer;
}


void ConfMemFree (void *pBuffer)
{
    if (pBuffer) {
        free(pBuffer);
    }
}


char * ConfMemCopyString (char **dst, const char *src)
{
    int cb;
    *dst = 0;
    if (src) {
        cb = strlen(src) + 1;
        *dst = malloc(cb);
        memcpy(*dst, src, cb);
    }
    return *dst;
}


CONF_position ConfOpenFile (const char *conf)
{
    CONF_position cpos;
    FILE *fp = fopen(conf, "rb");
    if (! fp) {
        return 0;
    }
    cpos = (CONF_position) ConfMemAlloc(1, sizeof(conf_position_t));
    cpos->_fp = fp;
    return cpos;
}


void ConfCloseFile (CONF_position cpos)
{
    fclose(cpos->_fp);
    ConfMemFree(cpos);
}


char * ConfGetNextPair (CONF_position cpos, char **key, char **val)
{
    char *start;
    int   nch;

    for (;;) {
        nch = readln(cpos->_fp, cpos->_buf, CONF_MAX_BUFSIZE);
        if (nch < 0) {
            return 0;
        }
        start = dtrim(dtrim(cpos->_buf, 32), 9);
        if (*start == CONF_NOTE_CHAR) { /* # */
            continue;
        }

        nch = strlen(start);
        if (nch > 2) {
            if (nch <= CONF_MAX_SECNAME && *start==CONF_SEC_BEGIN && *(start+nch-1)==CONF_SEC_END) {
                /* find a section */
                memcpy(cpos->_section, start+1, nch-1);
                cpos->_section[nch-1] = 0;
                cpos->_section[nch-2] = 0;
                continue;
            }

            if (splitpair(start, CONF_SEPARATOR, key, val)==CONF_TRUE) {
                return start;
            }
        }
    }
    return 0;
}


char * ConfGetFirstPair (CONF_position cpos, char **key, char **val)
{
    if (! cpos) {
        return 0;
    }

    rewind(cpos->_fp);
    return ConfGetNextPair(cpos, key, val);
}


const char * ConfGetSection (CONF_position cpos)
{
    return cpos->_section;
}


CONF_RESULT ConfCopySection (CONF_position cpos, char *secName)
{
    if (!cpos->_section) {
        return CONF_RET_ERROR;
    }
    if (!secName) {
        return strlen(cpos->_section)+2;
    }
    memcpy(secName, cpos->_section, strlen(cpos->_section)+2);
    return CONF_RET_SUCCESS;
}


int ConfReadValue (const char *confFile, const char *sectionName, const char *keyName, char *outBuf)
{
    int ret;
    char *str;
    char *key;
    char *val;

    CONF_position cpos = ConfOpenFile(confFile);

    str = ConfGetFirstPair(cpos, &key, &val);
    while (str) {
        if (! sectionName || ! strcmp(ConfGetSection(cpos), sectionName)) {
            if (! strcmp(key, keyName)) {
                ret = (int) strlen(val);

                if (! outBuf) {
                    ConfCloseFile(cpos);
                    return ret;
                }

                memcpy(outBuf, val, ret + 1);
                ConfCloseFile(cpos);
                return ret;
            }
        }

        str = ConfGetNextPair(cpos, &key, &val);
    }

    ConfCloseFile(cpos);
    return 0;
}


CONF_BOOL ConfReadValueRef (const char *confFile, const char *sectionName, const char *keyName, char **ppRefVal)
{
    char *str;
    char *key;
    char *val;

    CONF_position cpos = ConfOpenFile(confFile);

    str = ConfGetFirstPair(cpos, &key, &val);
    while (str) {
        if (! sectionName || ! strcmp(ConfGetSection(cpos), sectionName)) {
            if (! strcmp(key, keyName)) {
                *ppRefVal = val;
                ConfCloseFile(cpos);
                return CONF_TRUE;
            }
        }

        str = ConfGetNextPair(cpos, &key, &val);
    }

    ConfCloseFile(cpos);
    return CONF_FALSE;
}


int ConfReadValueParsed (const char *confFile, const char *family, const char *qualifier, const char *key, char *value)
{
    if (! qualifier) {
        return  ConfReadValue(confFile, family, key, value);
    } else {
        char section[CONF_MAX_SECNAME+4];
        sprintf(section, "%s:%s", family, qualifier);
        return  ConfReadValue(confFile, section, key, value);
    }
}


int ConfGetSectionList (const char *confFile, void **sectionList)
{
    char *key;
    char *val;
    int   numSections = 0;
    *sectionList = 0;

    CONF_position cpos = ConfOpenFile(confFile);
    if (!cpos) {
        return CONF_RET_ERROR;
    }

    /* get count of secions */
    char  secName[CONF_MAX_SECNAME];
    secName[0] = 0;
    while (ConfGetNextPair(cpos, &key, &val) != 0) {
        const char *sec = ConfGetSection(cpos);
        if (strcmp(sec, secName)) {
            ConfCopySection(cpos, secName);
            numSections++;
        }
    }
    ConfCloseFile(cpos);

    if (numSections > 0) {
        int  secNo = 1;
        char **secs = _SectionListAlloc(numSections);

        /* again get all of secions */
        cpos = ConfOpenFile(confFile);
        if (cpos) {
            while (secNo <= numSections && ConfGetNextPair(cpos, &key, &val) != 0) {
                const char *sec = ConfGetSection(cpos);
                if (secNo==1 || strcmp(sec, secs[secNo-1])) {
                    int secSize = ConfCopySection(cpos, 0);
                    secs[secNo] = (char*) ConfMemAlloc(secSize, sizeof(char));
                    ConfCopySection(cpos, secs[secNo]);
                    secNo++;
                }
            }
            ConfCloseFile(cpos);
        }
        *sectionList = (void*) secs;
    }

    return numSections;
}


void ConfSectionListFree (void *sections)
{
    if (sections) {
        char ** secs = (char **) sections;
        int numSections = (int) (uintptr_t) secs[0];
        while(numSections>0) {
            ConfMemFree(secs[numSections--]);
        }
        ConfMemFree(secs);
    }
}


char * ConfSectionListGetAt (void *sections, int secIndex)
{
    char ** secs = (char **) sections;
    int numSections = (int) (uintptr_t) secs[0];
    if (secIndex<0 || secIndex>=numSections) {
        return 0;
    }
    return secs[secIndex+1];
}


int ConfSectionParse (char *sectionName, char **family, char **qualifier)
{
    char *p, *q;

    if (! sectionName) {
        /* null string */
        return -1;
    }

    if (! sectionName[0]) {
        /* empty string */
        return 0;
    }

    *qualifier = 0;
    *family = sectionName;

    p = strchr(sectionName, (char)CONF_SEC_SEMI);
    q = strrchr(sectionName, (char)CONF_SEC_SEMI);

    if (p == q && p) {
        *p++ = 0;

        *qualifier = p;
        return 2;
    }

    return 1;
}
