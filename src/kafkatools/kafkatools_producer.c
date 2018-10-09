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
#include "kafkatools.h"

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
        printf("success: Message delivered (%zd bytes, partition %"PRId32")\n", rkmessage->len, rkmessage->partition);
    }
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


int kafkatools_producer_create (const char *brokers, kafkatools_msg_cb msg_cb, void *msg_opaque, kt_producer *outproducer)
{
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
     */
    if (rd_kafka_conf_set(producer->conf, "bootstrap.servers", brokers, producer->errstr, KAFKATOOLS_ERRSTR_SIZE) != RD_KAFKA_CONF_OK) {
        free(producer);
        return KAFKATOOLS_ERROR;
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

        /* Destroy the producer instance */
        rd_kafka_destroy(rkProducer);
    }

    rbtree_clean(&producer->rktopic_tree);

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



/*
int kafkatools_open_readonly_file (const char *pathfile, off_t start_position)
{
    int fd;

    fd = open(pathfile, O_RDONLY | O_NOATIME);
    if (fd == -1) {
        perror("open\n");
        return (-1);
    }

    if (lseek(fd, position, SEEK_SET) != position) {
        perror("lseek\n");
        close(fd);
        return (-1);
    }

    return fd;
    
    off_t offset = start_position;

    ssize_t cbread = pread(fd, (void *) buf, size_t nbyte, offset);

}
*/

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
static void dr_msg_cb (rd_kafka_t *rk, const rd_kafka_message_t *rkmessage, void *opaque) {
    if (rkmessage->err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        printf("error: Message delivery failed: %s\n", rd_kafka_err2str(rkmessage->err));
    } else {
        printf("success: Message delivered (%zd bytes, partition %"PRId32")\n", rkmessage->len, rkmessage->partition);
    }
}


int kafkatools_rdkafka_init(const char *brokers, void *msg_opaque, char *errstr, ssize_t errsize)
{
    int result;

    /* Temporary configuration object */
    rd_kafka_conf_t *conf;

    /* Producer instance handle */
    rd_kafka_t *rkProducer;

    /* Topic object */
    rd_kafka_topic_t *rkTopic;

    /*
     * Create Kafka client configuration place-holder
     */
    conf = rd_kafka_conf_new();

    /* Set bootstrap broker(s) as a comma-separated list of
     *  host or host:port (default port 9092).
     *  librdkafka will use the bootstrap brokers to acquire the full
     *  set of brokers from the cluster.
     */
    if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers, errstr, errsize) != RD_KAFKA_CONF_OK) {
        printf("rd_kafka_conf_set error: %s\n", errstr);
        return (-1);
    }

    /* Set the delivery report callback.
     * This callback will be called once per message to inform the application
     *  if delivery succeeded or failed. See dr_msg_cb() above.
     */
    rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);

    /*
     * Create producer instance.
     *
     * NOTE: rd_kafka_new() takes ownership of the conf object
     *       and the application must not reference it again after
     *       this call.
     */
    rkProducer = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, errsize);
    if (! rkProducer) {
        printf("rd_kafka_new error: %s\n", errstr);
        return (-1);
    }

    /* Create topic object that will be reused for each message produced.
     *
     * Both the producer instance (rd_kafka_t) and topic objects (topic_t)
     * are long-lived objects that should be reused as much as possible.
     */
    rkTopic = rd_kafka_topic_new(rkProducer, "test_topic", NULL);
    if (! rkTopic) {
        snprintf(errstr, errsize, "rd_kafka_topic_new(topic=%s) error: %s", "test_topic", rd_kafka_err2str(rd_kafka_last_error()));
        errstr[errsize - 1] = 0;

        printf("%s\n", errstr);

        rd_kafka_destroy(rkProducer);
        return (-1);
    }

    while (1) {
        char msgbuf[] = "hello world";
        int msglen = 10;

        /*
         * Send/Produce message.
         * This is an asynchronous call, on success it will only enqueue the message
         *  on the internal producer queue.
         * The actual delivery attempts to the broker are handled by background threads.
         * The previously registered delivery report callback (dr_msg_cb) is used to
         *  signal back to the application when the message has been delivered (or failed).
         */

retry:
        result = rd_kafka_produce(rkTopic,     /* Topic object */
                RD_KAFKA_PARTITION_UA,      /* Use builtin partitioner to select partition*/
                RD_KAFKA_MSG_F_COPY,        /* Make a copy of the payload. */
                msgbuf, msglen,             /* Message payload (value) and length */
                NULL, 0,                    /* Optional key and its length for partition */
                msg_opaque                  /* msg_opaque is an optional application-provided per-message opaque
                                             *  pointer that will provided in the delivery report callback (`dr_cb`) for
                                             *  referencing this message.
                                             */
            );

        if (result == -1) {
            /* Poll to handle delivery reports */
            if (rd_kafka_last_error() == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
                /* If the internal queue is full, wait for messages to be delivered and then retry.
                 * The internal queue represents both messages to be sent and messages that have
                 *  been sent or failed, awaiting their delivery report callback to be called.
                 *
                 * The internal queue is limited by the configuration property:
                 *      queue.buffering.max.messages
                 */
                rd_kafka_poll(rkProducer, 1000 /* block wait for max 1000ms */);

                // goto retry;
                break;
            } else {
                snprintf(errstr, errsize, "rd_kafka_produce failed: %s (topic=%s)",
                    rd_kafka_err2str(rd_kafka_last_error()), rd_kafka_topic_name(rkTopic));

                errstr[errsize - 1] = 0;

                printf("%s\n", errstr);
                
                /* Destroy topic object */
                rd_kafka_topic_destroy(rkTopic);

                /* Destroy the producer instance */
                rd_kafka_destroy(rkProducer);

                return (-1);
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
        rd_kafka_poll(rkProducer, 0 /* non-blocking */);
    }

    /* Wait for final messages to be delivered or fail.
     * rd_kafka_flush() is an abstraction over rd_kafka_poll() which
     * waits for all messages to be delivered.
     */
    rd_kafka_flush(rkProducer, -1);

    /* Destroy topic object */
    rd_kafka_topic_destroy(rkTopic);

    /* Destroy the producer instance */
    rd_kafka_destroy(rkProducer);

    return 0;
}