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
 * @file: xsync-error.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.3.0
 *
 * @create: 2018-01-24
 *
 * @update: 2018-10-22 10:56:41
 */


#ifndef XSYNC_ERROR_H_
#define XSYNC_ERROR_H_

#if defined(__cplusplus)
extern "C"
{
#endif

#define XS_IS_NULL(p)       ((p) == 0)
#define XS_IS_SUCCESS(r)    ((r) == XS_SUCCESS)
#define XS_HAS_ERROR(r)     ((r) != XS_NOERROR)

#define XS_IS_TRUE(b)       ((b) == XS_TRUE)
#define XS_IS_FALSE(b)      ((b) == XS_FALSE)


typedef void XS_VOID;


typedef int XS_RESULT;

#define XS_SUCCESS                   0
#define XS_NOERROR              XS_SUCCESS

#define XS_ERROR                   (-1)
#define XS_E_OUTMEM                (-4)
#define XS_E_PARAM                (-10)
#define XS_E_FILE                 (-11)
#define XS_E_NOTIMP               (-12)
#define XS_E_APPERR               (-13)
#define XS_E_INOTIFY              (-14)
#define XS_E_POOL                 (-15)


typedef int XS_BOOL;

#define XS_TRUE       1
#define XS_FALSE      0


#if defined(__cplusplus)
}
#endif

#endif /* XSYNC_ERROR_H_ */
