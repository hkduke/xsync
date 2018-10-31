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
 * @file: server_epcb.c
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.3.4
 *
 * @create: 2018-01-29
 *
 * @update: 2018-10-29 10:24:55
 */

#include "server_api.h"
#include "server_conf.h"

#include "../common/common_util.h"
#include "../redisapi/redis_api.h"


int epcb_event_trace (struct epollet_event_t *event)
{
    return 0;
}


int epcb_event_warn (struct epollet_event_t *event)
{
    return 0;
}


int epcb_event_error (struct epollet_event_t *event)
{
    return 0;
}


int epcb_event_new_peer (struct epollet_event_t *event)
{
    // 接受
    return 1;
}


int epcb_event_accept (struct epollet_event_t *event)
{
    return 0;
}


int epcb_event_reject (struct epollet_event_t *event)
{
    return 0;
}


int epcb_event_pollin (struct epollet_event_t *event)
{
    return 0;    
}


int epcb_event_pollout (struct epollet_event_t *event)
{
    return 0;
}
