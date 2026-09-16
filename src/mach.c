#include "mach.h"

#include <stdlib.h>
#include <string.h>
#include <CoreFoundation/CoreFoundation.h>

mach_port_t mach_get_bs_port(const char *bs_name) {
    if (!bs_name) return 0;

    mach_port_t bs_port = MACH_PORT_NULL;
    if (task_get_special_port(mach_task_self(), TASK_BOOTSTRAP_PORT,
                              &bs_port) != KERN_SUCCESS)
        return 0;

    mach_port_t port = MACH_PORT_NULL;
    if (bootstrap_look_up(bs_port, bs_name, &port) != KERN_SUCCESS)
        return 0;

    return port;
}

void mach_receive_message(mach_port_t port, uint32_t *len, char **data,
                          bool timeout) {
    *len = 0;
    *data = NULL;

    uint32_t max = 1 << 16;
    char *buf = malloc(max);
    if (!buf) return;

    mach_msg_header_t *header = (mach_msg_header_t *)buf;

    mach_msg_return_t result;
    if (timeout) {
        result = mach_msg(header, MACH_RCV_MSG | MACH_RCV_TIMEOUT, 0, max,
                          port, 100, MACH_PORT_NULL);
    } else {
        result = mach_msg(header, MACH_RCV_MSG, 0, max, port,
                          MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    }

    if (result != MACH_MSG_SUCCESS) {
        free(buf);
        return;
    }

    mach_msg_body_t *body = (mach_msg_body_t *)(header + 1);
    mach_msg_ool_descriptor_t *desc =
        (mach_msg_ool_descriptor_t *)(body + 1);

    if (body->msgh_descriptor_count >= 1 && desc->address && desc->size > 0) {
        *len = (uint32_t)desc->size;
        *data = malloc(*len + 1);
        if (*data) {
            memcpy(*data, desc->address, *len);
            (*data)[*len] = '\0';
        }
    }

    mach_msg_destroy(header);
    free(buf);
}

char *mach_send_message(mach_port_t port, char *message,
                        uint32_t len, bool await_response) {
    if (!message || !port) return NULL;

    mach_port_t task = mach_task_self();
    mach_port_t response_port = MACH_PORT_NULL;

    if (await_response) {
        if (mach_port_allocate(task, MACH_PORT_RIGHT_RECEIVE,
                               &response_port) != KERN_SUCCESS)
            return NULL;
        if (mach_port_insert_right(task, response_port, response_port,
                                   MACH_MSG_TYPE_MAKE_SEND) != KERN_SUCCESS) {
            mach_port_deallocate(task, response_port);
            return NULL;
        }
    }

    struct mach_oob_message {
        mach_msg_header_t header;
        mach_msg_body_t body;
        mach_msg_ool_descriptor_t descriptor;
    };

    struct mach_oob_message msg;
    memset(&msg, 0, sizeof(msg));
    msg.header.msgh_remote_port = port;
    msg.header.msgh_size = (mach_msg_size_t)sizeof(msg);
    msg.header.msgh_id = (uint32_t)getpid();

    if (await_response) {
        msg.header.msgh_local_port = response_port;
        msg.header.msgh_bits =
            MACH_MSGH_BITS_SET(MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MAKE_SEND,
                               0, MACH_MSGH_BITS_COMPLEX);
    } else {
        msg.header.msgh_bits =
            MACH_MSGH_BITS_SET(MACH_MSG_TYPE_COPY_SEND, 0, 0,
                               MACH_MSGH_BITS_COMPLEX);
    }

    msg.body.msgh_descriptor_count = 1;
    msg.descriptor.address = message;
    msg.descriptor.size = len;
    msg.descriptor.copy = MACH_MSG_VIRTUAL_COPY;
    msg.descriptor.deallocate = false;
    msg.descriptor.type = MACH_MSG_OOL_DESCRIPTOR;

    mach_msg(&msg.header, MACH_SEND_MSG,
             (mach_msg_size_t)sizeof(msg), 0, MACH_PORT_NULL,
             MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);

    if (!await_response)
        return NULL;

    uint32_t rlen = 0;
    char *rdata = NULL;
    mach_receive_message(response_port, &rlen, &rdata, true);
    if (!rdata) rdata = strdup("");

    mach_msg_destroy(&msg.header);
    mach_port_mod_refs(task, response_port, MACH_PORT_RIGHT_RECEIVE, -1);
    mach_port_deallocate(task, response_port);
    return rdata;
}