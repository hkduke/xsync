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
 * @file: server_conf.c
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.4.4
 *
 * @create: 2018-02-02
 *
 * @update: 2018-11-07 10:20:15
 */

#include "server_api.h"
#include "server_conf.h"

#include "../common/readconf.h"
#include "../common/common_util.h"


extern void xs_server_delete (void *pv)
{
    XS_server server = (XS_server) pv;

    LOGGER_TRACE("pause and destroy timer");
    mul_timer_pause();
    if (mul_timer_destroy() != 0) {
        LOGGER_ERROR("mul_timer_destroy failed");
    }

    LOGGER_TRACE("pthread_cond_destroy");
    pthread_cond_destroy(&server->condition);

    LOGGER_TRACE("epollet_conf_uninit");
    epollet_conf_uninit(&server->epconf);

    if (server->pool) {
        LOGGER_DEBUG("threadpool_destroy");
        threadpool_destroy(server->pool, 0);
    }

    if (server->thread_args) {
        int i;
        perthread_data *pdata;

        LOGGER_DEBUG("destroy thread_args");

        for (i = 0; i < server->threads; ++i) {
            pdata = server->thread_args[i];
            server->thread_args[i] = 0;

            LOGGER_DEBUG("thread-%d: RedisConnFree", pdata->threadid);
            RedisConnFree(&pdata->redconn);

            mem_free(pdata);
        }

        mem_free_s((void**) &server->thread_args);
    }

    XS_server_clear_client_sessions(server);

    LOGGER_DEBUG("server: RedisConnFree");
    RedisConnFree(&server->redisconn);

    LOGGER_TRACE("%p", server);

    mem_free(server);
}
