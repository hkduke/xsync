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
 * @file: kafkatools.h
 *   How to use:
 *     http://www.yolinux.com/TUTORIALS/LibraryArchives-StaticAndDynamic.html
 *
 * @author: master@pepstack.com
 *
 * @version: 0.1.4
 *
 * @create: 2018-10-08 16:17:00
 * @update: 2018-10-11 12:04:32
 *
 */

#ifndef KAFKATOOLS_H_INCLUDED
#define KAFKATOOLS_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#define KAFKATOOLS_SUCCESS    0
#define KAFKATOOLS_ERROR    (-1)

#define KAFKATOOLS_ERRSTR_SIZE  256

/**
 * The C API is also documented in rdkafka.h
 */
#include <librdkafka/rdkafka.h>

#include "red_black_tree.h"

#define KAFKATOOLS_MSG_CB_DEFAULT  ((kafkatools_msg_cb)((void*) (uintptr_t) (int) (-1)))

typedef void (*kafkatools_msg_cb) (rd_kafka_t *rk,  const rd_kafka_message_t *rkmessage, void *opaque);

typedef struct kafkatools_producer_t * kt_producer;

typedef struct rd_kafka_topic_t * kt_topic;


typedef struct kafkatools_produce_msg_t
{
    /* Topic object */
    kt_topic topic;

    /* Use builtin partitioner (RD_KAFKA_PARTITION_UA) to select partition */
    int32_t partition;

    /* Message payload (msg) and length(msglen) */
    ssize_t msglen;
    char *msgbuf;

    /* Optional key and its length */
    ssize_t keylen;
    char *key;

    /* Message opaque, provided in delivery report callback as msg_opaque */
    void *opaque;
} kafkatools_produce_msg_t;


typedef struct kafkatools_producer_api_t
{
    void *handle;

    kt_producer producer;

    const char * (* kt_get_rdkafka_version) (void);
    const char * (* kt_producer_get_errstr) (kt_producer);
    int (* kt_producer_create) (const char **, const char **, kafkatools_msg_cb, void *, kt_producer *);
    void (* kt_producer_destroy) (kt_producer);
    kt_topic (* kt_get_topic) (kt_producer, const char *);
    const char * (* kt_topic_name) (const kt_topic);
    int (*kt_produce_message_sync) (kt_producer, const char *, int, kt_topic, int, int);
} kafkatools_producer_api_t;


extern const char * kafkatools_get_rdkafka_version (void);

extern const char * kafkatools_producer_get_errstr (kt_producer producer);

extern int kafkatools_producer_create (const char **prop_names, const char **prop_values, kafkatools_msg_cb msg_cb, void *msg_opaque, kt_producer *producer);

extern void kafkatools_producer_destroy (kt_producer producer);

extern kt_topic kafkatools_get_topic (kt_producer producer, const char *topic_name);

extern const char * kafkatools_topic_name (const kt_topic topic);

extern int kafkatools_produce_message_sync (kt_producer producer, const char *message, int chlen, kt_topic topic, int partition, int timout_ms);

extern int kafkatools_producer_process_msgfile (kt_producer producer, const char *msgfile, const char *linebreak, off_t position);


#if defined(__cplusplus)
}
#endif

#endif /* KAFKATOOLS_H_INCLUDED */
