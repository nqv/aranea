/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/sendfile.h>

#include <aranea/aranea.h>

/** Check if the system call is blocked.
 *  EAGAIN is returned if fd is non-blocking, otherwise EINTR may arise.
 */
#define CHECK_NONBLOCKING_ERROR(rv, cl, method)                             \
    do {                                                                    \
        if (rv <= 0) {                                                      \
            if (rv == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {    \
                A_LOG("client: %d %s blocked", client->remote_fd, method);  \
            } else {                                                        \
                A_ERR("client: %d %s %s", client->remote_fd, method, strerror(errno)); \
                client->state = STATE_NONE;                                 \
            }                                                               \
            return;                                                         \
        }                                                                   \
    } while (0)

/** Read header from socket
 */
void state_recv_header(struct client_t *client) {
    ssize_t len;

    /* Just peek the data to find the end of the header */
    if (client->request.header_length <= 0) {
        len = recv(client->remote_fd, client->data, sizeof(client->data),
                MSG_PEEK);
        CHECK_NONBLOCKING_ERROR(len, client, "peek");
        /* Check header termination */
        if (len > 4) {
            client->request.header_length = http_find_headerlength(client->data,
                    len);
            if (client->request.header_length < 0) {
                /* Not found, check if data size is too large */
                if (len >= MAX_REQUEST_LENGTH) {
                    client->response.status_code = HTTP_STATUS_ENTITYTOOLARGE;
                    client->data_length = http_gen_errorpage(&client->response,
                            client->data, sizeof(client->data));
                    client->state = STATE_SEND_HEADER;
                }
            }
        }
    }
    /* Already peeked, pull data from the socket until reaching this position */
    if (client->request.header_length > 0) {
        len = recv(client->remote_fd, client->data + client->data_length,
                client->request.header_length - client->data_length, 0);
        CHECK_NONBLOCKING_ERROR(len, client, "recv");
        client->data_length += len;
        if (client->data_length >= client->request.header_length) {
            client_process(client);
        }
    }
    /* Should not wait until next server loop to process this request */
    if (client->state == STATE_SEND_HEADER) {
        state_send_header(client);
    }
}

/** Send reply (header) via socket
 */
void state_send_header(struct client_t *client) {
    ssize_t len;

    len = send(client->remote_fd, client->data + client->data_sent,
                client->data_length - client->data_sent, 0);
    CHECK_NONBLOCKING_ERROR(len, client, "send");
    client->data_sent += len;

    /* check if finish sending header */
    if (client->data_sent >= client->data_length) {
        client->data_length = client->data_sent = 0;
        if (client->local_rfd == -1) {
            client->state = STATE_NONE;
        } else {
            client->state = STATE_SEND_FILE;
        }
    }
}

void state_send_file(struct client_t *client) {
    ssize_t len;
    off_t offset;

    offset = client->response.content_from + client->file_sent;
    len = sendfile(client->remote_fd, client->local_rfd, &offset,
            client->response.content_length - client->file_sent);
    CHECK_NONBLOCKING_ERROR(len, client, "sendfile");
    client->file_sent += len;
    if (client->file_sent >= client->response.content_length) {
        client->state = STATE_NONE;
    }
}

/* vim: set ts=4 sw=4 expandtab: */
