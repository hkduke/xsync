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
 * @file: perthread_data.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.4.2
 *
 * @create:
 *
 * @update: 2018-11-07 10:20:15
 */

#ifndef PERTHREAD_DATA_H_INCLUDED
#define PERTHREAD_DATA_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../common/common_util.h"

#include "../kafkatools/kafkatools.h"
#include "../luacontext/luacontext.h"

#include "server_conn.h"

#include "../common/red_black_tree.h"


typedef struct perthread_data
{
    int    threadid;

    void  *xclient;

    lua_context luactx;

    int kafka_producer_ready;
    struct  kafkatools_producer_api_t kt_producer_api;

    xs_server_conn_t *server_conns[XSYNC_SERVER_MAXID + 1];

    /* buffer with size >= 8192 and >= (PATH_MAX x 2) */
    char buffer[XSYNC_BUFSIZE];
} perthread_data;


#define v_type_buf(ptdata)      (ptdata->buffer + 0)
#define v_time_buf(ptdata)      (ptdata->buffer + 10)
#define v_sid_buf(ptdata)       (ptdata->buffer + 38)
#define v_thread_buf(ptdata)    (ptdata->buffer + 45)
#define v_event_buf(ptdata)     (ptdata->buffer + 50)
#define v_clientid_buf(ptdata)  (ptdata->buffer + 100)
#define v_pathid_buf(ptdata)    (ptdata->buffer + 200)
#define v_file_buf(ptdata)      (ptdata->buffer + 300)
#define v_route_buf(ptdata)     (ptdata->buffer + 560)
#define v_path_buf(ptdata)      (ptdata->buffer + 2000)
#define v_eventmsg_buf(ptdata)  (ptdata->buffer + 4000)
#define v_buffer_end(ptdata)    (ptdata->buffer + XSYNC_BUFSIZE)


#define v_type_cb(ptdata)      (v_time_buf(ptdata) - v_type_buf(ptdata))
#define v_time_cb(ptdata)      (v_sid_buf(ptdata) - v_time_buf(ptdata))
#define v_sid_cb(ptdata)       (v_thread_buf(ptdata) - v_sid_buf(ptdata))
#define v_thread_cb(ptdata)    (v_event_buf(ptdata) - v_thread_buf(ptdata))
#define v_event_cb(ptdata)     (v_clientid_buf(ptdata) - v_event_buf(ptdata))
#define v_clientid_cb(ptdata)  (v_pathid_buf(ptdata) - v_clientid_buf(ptdata))
#define v_pathid_cb(ptdata)    (v_file_buf(ptdata) - v_pathid_buf(ptdata))
#define v_file_cb(ptdata)      (v_route_buf(ptdata) - v_file_buf(ptdata))
#define v_route_cb(ptdata)     (v_path_buf(ptdata) - v_route_buf(ptdata))
#define v_path_cb(ptdata)      (v_eventmsg_buf(ptdata) - v_path_buf(ptdata))
#define v_eventmsg_cb(ptdata)  (v_buffer_end(ptdata) - v_eventmsg_buf(ptdata))


__no_warning_unused(static)
int kafka_producer_api_create(kafkatools_producer_api_t *api, const char *libktsofile, const char *prop_names[], const char *prop_values[])
{
    void *handle;
    char *error;

    handle = dlopen(libktsofile, RTLD_LAZY);
    if (! handle) {
        LOGGER_ERROR("dlopen fail: %s (%s)", dlerror(), libktsofile);

        return (-1);
    }

    /* Clear any existing error */
    dlerror();

    api->kt_get_rdkafka_version = dlsym(handle, "kafkatools_get_rdkafka_version");

    if ((error = dlerror()) != NULL) {
        LOGGER_ERROR("dlsym fail: %s", error);

        dlclose(handle);
        return (-1);
    }

    // TODO: 判断 rdkafka 版本
    LOGGER_INFO("kafkatools_get_rdkafka_version: %s", api->kt_get_rdkafka_version());

    api->kt_producer_get_errstr = dlsym(handle, "kafkatools_producer_get_errstr");
    api->kt_producer_create = dlsym(handle, "kafkatools_producer_create");
    api->kt_producer_destroy = dlsym(handle, "kafkatools_producer_destroy");
    api->kt_get_topic = dlsym(handle, "kafkatools_get_topic");
    api->kt_topic_name = dlsym(handle, "kafkatools_topic_name");
    api->kt_produce_message_sync = dlsym(handle, "kafkatools_produce_message_sync");

    if (api->kt_producer_create(prop_names, prop_values, KAFKATOOLS_MSG_CB_DEFAULT, 0, &api->producer) != KAFKATOOLS_SUCCESS) {
        LOGGER_ERROR("kafkatools_producer_create fail");
        dlclose(handle);
        return (-1);
    }

    api->handle = handle;
    return 0;
}


__no_warning_unused(static)
void kafka_producer_api_free(kafkatools_producer_api_t *api)
{
    if (api->handle) {
        void *handle = api->handle;
        api->handle = 0;

        api->kt_producer_destroy(api->producer);

        dlclose(handle);

        bzero(api, sizeof(kafkatools_producer_api_t));
    }
}


/**
 * client_conf.c
 */
extern void * perthread_data_create (XS_client client, int servers, int threadid, const char *taskscriptfile);

extern void perthread_data_free (perthread_data *perdata);


#if defined(__cplusplus)
}
#endif

#endif /* PERTHREAD_DATA_H_INCLUDED */
