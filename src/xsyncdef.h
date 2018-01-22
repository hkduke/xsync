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
* xsyncdef.h
*   common definitions for xsync-client and xsync-server
*
* Last Updated: 2018-01-21
* Last Updated: 2018-01-21
*/
#ifndef XSYNC_DEF_H_
#define XSYNC_DEF_H_

#if defined(__cplusplus)
extern "C"
{
#endif

#define XSYNC_CLIENTID_MAXLEN    40

/**
 * each client thread has
 */
#define XSYNC_SERVER_MAXID       255

#define XSYNC_HOSTNAME_MAXLEN    128

#if defined(__cplusplus)
}
#endif

#endif /* XSYNC_DEF_H_ */
