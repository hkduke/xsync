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
 * @version: 0.1.0
 *
 * @create: 2018-10-08 16:17:00
 * @update:
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


typedef struct kafkatools_msgline_t
{
    char *msgfile;

    kt_topic topic;

    int partition;

    off_t position;
    ssize_t cbsize;

    char *msgline;
} kafkatools_msgline_t;


extern const char * kafkatools_get_rdkafka_version (void);

extern const char * kafkatools_producer_get_errstr (kt_producer producer);

extern int kafkatools_producer_create (const char *brokers, kafkatools_msg_cb msg_cb, void *msg_opaque, kt_producer *producer);

extern void kafkatools_producer_destroy (kt_producer producer);

extern kt_topic kafkatools_get_topic (kt_producer producer, const char *topic_name);

extern const char * kafkatools_topic_name (const kt_topic topic);

/*
extern int kafkatools_producer_process_msgfile (const char *msgfile, const char *linebreak, ssize_t off_t position);
*/

////////////////////////////del
int kafkatools_rdkafka_init(const char *brokers, void *msg_opaque, char *errmsg, ssize_t sizemsg);

// kafkatools_produce_from_file ();
//const char * kafkatools_on_line_string(char *linestr, int chlen, int cbsize, const char *colsep, const char *linebreak);


#if defined(__cplusplus)
}
#endif

#endif /* KAFKATOOLS_H_INCLUDED */
