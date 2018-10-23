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
 * @file: kafkatools_producer.c
 *
 *  refer:
 *    https://github.com/edenhill/librdkafka/blob/master/src/rdkafka.h
 *
 *    https://github.com/edenhill/librdkafka/blob/master/examples/rdkafka_simple_producer.c
 *
 * @author: master@pepstack.com
 *
 * @version: 0.2.6
 *
 * @create: 2018-10-08
 *
 * @update: 2018-10-22 10:56:41
 */

#include "kafkatools.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>


typedef struct kafkatools_producer_t
{
    /* Temporary configuration object */
    rd_kafka_conf_t *conf;

    /* Producer instance handle */
    rd_kafka_t *rkProducer;

    red_black_tree_t  rktopic_tree;

    char errstr[KAFKATOOLS_ERRSTR_SIZE];
} kafkatools_producer_t;


static int rktopic_name_cmp(void *newObject, void *nodeObject)
{
    return strcmp(rd_kafka_topic_name((rd_kafka_topic_t *) newObject), rd_kafka_topic_name((rd_kafka_topic_t *) nodeObject));
}


/*! Callback function prototype for traverse objects */
static void rktopic_object_release(void *object, void *param)
{
    rd_kafka_topic_destroy((rd_kafka_topic_t *) object);
}


static ssize_t pread_len (int fd, unsigned char *buf, size_t len, off_t pos)
{
    ssize_t rc;
    unsigned char *pb = buf;

    while (len != 0 && (rc = pread (fd, (void*) pb, len, pos)) != 0) {
        if (rc == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            } else {
                /* pread error */
                return (-1);
            }
        }

        len -= rc;
        pos += rc;
        pb += rc;
    }

    return (ssize_t) (pb - buf);
}


/**
 * Message delivery report callback.
 *
 * This callback is called exactly once per message, indicating if the message
 *  was succesfully delivered (rkmessage->err == RD_KAFKA_RESP_ERR_NO_ERROR) or
 *  permanently failed delivery (rkmessage->err != RD_KAFKA_RESP_ERR_NO_ERROR).
 *
 * The callback is triggered from rd_kafka_poll() and executes on the
 *  application's thread.
 *
 * The rkmessage is destroyed automatically by librdkafka.
 */
static void kt_msg_cb_default (rd_kafka_t *rk, const rd_kafka_message_t *rkmessage, void *opaque) {
    if (rkmessage->err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        printf("error: Message delivery failed: %s\n", rd_kafka_err2str(rkmessage->err));
    } else {
        // printf("success: Message delivered (%zd bytes, partition %"PRId32")\n", rkmessage->len, rkmessage->partition);
    }
}


/**
 * !! 要求调用者实现此函数 !!
 *
 */
static int on_process_msgline (char *msgline, ssize_t msglen, kafkatools_produce_msg_t *outmsg)
{
    return 0;
}


/***************************** public api *****************************/

const char * kafkatools_get_rdkafka_version (void)
{
    return rd_kafka_version_str();
}


const char * kafkatools_producer_get_errstr (kt_producer producer)
{
    producer->errstr[KAFKATOOLS_ERRSTR_SIZE - 1] = '\0';
    return producer->errstr;
}


int kafkatools_producer_create (const char **prop_names, const char **prop_values, kafkatools_msg_cb msg_cb, void *msg_opaque, kt_producer *outproducer)
{
    int i;

    rd_kafka_conf_res_t res;

    kafkatools_producer_t *producer = (kt_producer) malloc(sizeof(*producer));

    bzero(producer, sizeof(*producer));

    /*
     * Create Kafka client configuration place-holder
     */
    producer->conf = rd_kafka_conf_new();
    if (! producer->conf) {
        free(producer);
        return KAFKATOOLS_ERROR;
    }

    /* Set bootstrap broker(s) as a comma-separated list of
     *  host or host:port (default port 9092).
     *  librdkafka will use the bootstrap brokers to acquire the full
     *  set of brokers from the cluster.
     *
     *  char *names[] = {
     *      "bootstrap.servers",
     *      "socket.timeout.ms",
     *      0
     *  };
     *
     *  char *values[] = {
     *     "localhost:9092,localhost2:9092",
     *     "1000"
     *  };
     */
    i = 0;
    while (i < 256 && prop_names[i]) {
        res = rd_kafka_conf_set(producer->conf, prop_names[i], prop_values[i], producer->errstr, KAFKATOOLS_ERRSTR_SIZE);
        if (res != RD_KAFKA_CONF_OK) {
            rd_kafka_conf_destroy(producer->conf);
            free(producer);
            return KAFKATOOLS_ERROR;
        }

        ++i;
    }

    /* Set the delivery report callback.
     * This callback will be called once per message to inform the application
     *  if delivery succeeded or failed. See dr_msg_cb() above.
     */
    if (msg_cb == KAFKATOOLS_MSG_CB_DEFAULT) {
        rd_kafka_conf_set_dr_msg_cb(producer->conf, kt_msg_cb_default);
    } else {
        rd_kafka_conf_set_dr_msg_cb(producer->conf, msg_cb);
    }

    /*
     * Retrieves the opaque pointer previously set with:
     *   rd_kafka_conf_set_opaque()
     */
    rd_kafka_conf_set_opaque(producer->conf, msg_opaque);

    /*
     * Create producer instance.
     *
     * NOTE: rd_kafka_new() takes ownership of the conf object
     *       and the application must not reference it again after
     *       this call.
     */
    producer->rkProducer = rd_kafka_new(RD_KAFKA_PRODUCER, producer->conf, producer->errstr, KAFKATOOLS_ERRSTR_SIZE);
    if (! producer->rkProducer) {
        rd_kafka_conf_destroy(producer->conf);
        free(producer);
        return KAFKATOOLS_ERROR;
    }

    rbtree_init(&producer->rktopic_tree, (fn_comp_func*) rktopic_name_cmp);

    *outproducer = producer;

    return KAFKATOOLS_SUCCESS;
}


void kafkatools_producer_destroy (kt_producer producer)
{
    if (producer->rkProducer) {
        rd_kafka_t *rkProducer = producer->rkProducer;

        producer->rkProducer = 0;

        rbtree_traverse(&producer->rktopic_tree, rktopic_object_release, 0);

        rbtree_clean(&producer->rktopic_tree);

        /* Destroy the producer instance */
        rd_kafka_destroy(rkProducer);
    }

    free(producer);
}


kt_topic kafkatools_get_topic (kt_producer producer, const char *topic_name)
{
    red_black_node_t *node;

    /* Topic handles are refcounted internally and calling rd_kafka_topic_new()
     *  again with the same topic name will return the previous topic handle
     *  without updating the original handle's configuration.
     *
     * Applications must eventually call rd_kafka_topic_destroy() for each
     *  succesfull call to rd_kafka_topic_new() to clear up resources.
     *
     * returns the new topic handle or NULL on error.
     */
    rd_kafka_topic_t *rktopic;

    rktopic = rd_kafka_topic_new(producer->rkProducer, topic_name, NULL);

    if (! rktopic) {
        snprintf(producer->errstr, KAFKATOOLS_ERRSTR_SIZE, "rd_kafka_topic_new(topic=%s) fail: %s",
            topic_name, rd_kafka_err2str(rd_kafka_last_error()));
        return NULL;
    }

    node = rbtree_find(&producer->rktopic_tree, (void*) rktopic);
    if (! node) {
        // 如果 tree 没有 topic, 插入新 topic
        int is_new_node;

        node = rbtree_insert_unique(&producer->rktopic_tree, (void *)rktopic, &is_new_node);
        if (! node || ! is_new_node) {
            // 必须成功
            printf("application error: should never run to this!\n");
            exit(-1);
        }
    } else {
        // 如果 tree 中已经存在 topic, 释放 topic 引用计数
        rd_kafka_topic_destroy(rktopic);
    }

    // do not call rd_kafka_topic_destroy() for below object!
    return (kt_topic) rktopic;
}


const char * kafkatools_topic_name (const kt_topic topic)
{
    return rd_kafka_topic_name((const rd_kafka_topic_t *) topic);
}


int kafkatools_produce_message_sync (kt_producer producer, const char *message, int chlen, kt_topic topic, int partition, int timout_ms)
{
    int ret;

    ret = rd_kafka_produce( (rd_kafka_topic_t *) topic,   /* Topic object */
            partition,                   /* Use builtin partitioner to select partition*/
            RD_KAFKA_MSG_F_COPY,         /* Make a copy of the payload. */
            (void* ) message, chlen,     /* Message payload (value) and length */
            NULL, 0,                     /* Optional key and its length for partition */
            NULL                         /* msg_opaque is an optional application-provided per-message opaque
                                          *  pointer that will provided in the delivery report callback (`dr_cb`) for
                                          *  referencing this message.
                                          */
        );

    if (ret == -1) {
        /* Poll to handle delivery reports */
        if (rd_kafka_last_error() == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
            /* If the internal queue is full, wait for messages to be delivered and then retry.
             * The internal queue represents both messages to be sent and messages that have
             *  been sent or failed, awaiting their delivery report callback to be called.
             *
             * The internal queue is limited by the configuration property:
             *      queue.buffering.max.messages
             */
            rd_kafka_poll(producer->rkProducer, timout_ms);

            return KAFKATOOLS_SUCCESS;
        } else {
            snprintf(producer->errstr, KAFKATOOLS_ERRSTR_SIZE, "rd_kafka_produce (topic=%s, partition=%d) failed: %s",
                    kafkatools_topic_name(topic),
                    partition,
                    rd_kafka_err2str(rd_kafka_last_error())
                );
            return KAFKATOOLS_ERROR;
        }
    }

    /* Wait for final messages to be delivered or fail.
     *  rd_kafka_flush() is an abstraction over rd_kafka_poll() which
     *  waits for all messages to be delivered.
     */
    rd_kafka_flush(producer->rkProducer, timout_ms);

    return KAFKATOOLS_SUCCESS;
}


int kafkatools_producer_process_msgfile (kt_producer producer, const char *msgfile, const char *linebreak, off_t position)
{
    int fd;
    int ret;
    char buf[4096];
    ssize_t cb;

    kafkatools_produce_msg_t msg;

    // TODO: 初始化 msg

    fd = open(msgfile, O_RDONLY | O_NOATIME);
    if (fd == -1) {
        perror("open\n");
        return KAFKATOOLS_ERROR;
    }

    if (lseek(fd, position, SEEK_SET) != position) {
        perror("lseek\n");
        close(fd);
        return KAFKATOOLS_ERROR;
    }

    while ((cb = pread_len(fd, (unsigned char *) buf, sizeof buf, position)) > 0) {
        ret = on_process_msgline(buf, cb, &msg);

        // TODO: ret

        ret = rd_kafka_produce((rd_kafka_topic_t *) msg.topic,   /* Topic object */
                msg.partition,              /* Use builtin partitioner to select partition*/
                RD_KAFKA_MSG_F_COPY,        /* Make a copy of the payload. */
                msg.msgbuf, msg.msglen,     /* Message payload (value) and length */
                msg.key, msg.keylen,        /* Optional key and its length for partition */
                msg.opaque                  /* msg_opaque is an optional application-provided per-message opaque
                                             *  pointer that will provided in the delivery report callback (`dr_cb`) for
                                             *  referencing this message.
                                             */
            );

        if (ret == -1) {
            /* Poll to handle delivery reports */
            if (rd_kafka_last_error() == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
                /* If the internal queue is full, wait for messages to be delivered and then retry.
                 * The internal queue represents both messages to be sent and messages that have
                 *  been sent or failed, awaiting their delivery report callback to be called.
                 *
                 * The internal queue is limited by the configuration property:
                 *      queue.buffering.max.messages
                 */
                rd_kafka_poll(producer->rkProducer, 1000 /* block wait for max 1000ms */);

                // goto retry;
                break;
            } else {
                snprintf(producer->errstr, KAFKATOOLS_ERRSTR_SIZE, "rd_kafka_produce failed: %s (topic=%s)",
                    rd_kafka_err2str(rd_kafka_last_error()), kafkatools_topic_name(msg.topic));

                goto exit_onerror;
            }
        }

        /* A producer application should continually serve the delivery report queue
         *  by calling rd_kafka_poll() at frequent intervals.
         * Either put the poll call in your main loop, or in a dedicated thread, or
         *  call it after every rd_kafka_produce() call.
         * Just make sure that rd_kafka_poll() is still called during periods where
         *  you are not producing any messages to make sure previously produced messages
         *  have their delivery report callback served (and any other callbacks you register).
         */
        rd_kafka_poll(producer->rkProducer, 0 /* non-blocking */);
    }

    /* Wait for final messages to be delivered or fail.
     *  rd_kafka_flush() is an abstraction over rd_kafka_poll() which
     *  waits for all messages to be delivered.
     */
    rd_kafka_flush(producer->rkProducer, -1);

    close(fd);
    return KAFKATOOLS_SUCCESS;

exit_onerror:

    close(fd);
    return KAFKATOOLS_ERROR;
}
