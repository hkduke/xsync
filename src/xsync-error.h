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
 * xsync-error.h
 *   Definitions for xsync-client and xsync-server
 *
 * author: master@pepstack.com
 *
 * create: 2018-01-29
 * update: 2018-01-29
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

#define XS_SUCCESS    0
#define XS_NOERROR    XS_SUCCESS

#define XS_ERROR    (-1)

#define XS_EARG     (-10)
#define XS_EFILE    (-11)



typedef int XS_BOOL;

#define XS_TRUE       1
#define XS_FALSE      0


#if defined(__cplusplus)
}
#endif

#endif /* XSYNC_ERROR_H_ */
