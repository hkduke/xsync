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
* 2. Altered source versions must be plainly marked as such, and must not
*   be misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
***********************************************************************/

/**
 * thread_info.h
 *
 * author: master@pepstack.com
 * refer:
 *   https://bytefreaks.net/programming-2/c-full-example-of-pthread_cond_timedwait
 *
 * create: 2018-09-10
 * update: 2018-02-04
 */
#ifndef THREAD_INFO_H_INCLUDED
#define THREAD_INFO_H_INCLUDED

#if defined(__cplusplus)
extern "C"
{
#endif


__no_warning_unused(static)
void error_pthread_cond_timedwait(int err, char errmsg, int msgsize)
{
    snprintf()
        case ETIMEDOUT:
            fprintf(stderr, "ETIMEDOUT: The time specified by abstime to pthread_cond_timedwait() has passed.");
            break;
        case EINVAL:
            fprintf(stderr, "EINVAL: The value specified by abstime, cond or mutex is invalid.");
            break;
        case EPERM:
            fprintf(stderr, "EPERM: The mutex was not owned by the current thread at the time of the call.");
            break;
        default:
            break;
    }
}


#if defined(__cplusplus)
}
#endif

#endif /* THREAD_INFO_H_INCLUDED */
