/* Aranea
 * Copyright (c) 2011, Nguyen Quoc Viet
 * See LICENSE file for copyright and license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include "aranea.h"

void client_add(struct client_t *self, struct client_t **list) {
    if (*list == NULL) {
        self->next = NULL;
    } else {
        self->next = *list;
        (*list)->prev = &self->next;
    }
    *list = self;
    self->prev = list;
}

void client_remove(struct client_t *self) {
    if (self->next == NULL) {
        self->next->prev = self->prev;
    }
    *(self->prev) = self->next;
}

void client_close(struct client_t *self) {
    if (self->remote_fd != -1) {
        close(self->remote_fd);
    }
    if (self->local_fd != -1) {
        close(self->local_fd);
    }
}

struct client_t *client_new() {
    struct client_t *self;
    self = calloc(1, sizeof(struct client_t));
    if (self == NULL) {
        A_ERR("malloc %d", sizeof(struct client_t));
        return NULL;
    }
    self->remote_fd = -1;
    self->local_fd = -1;
    return self;
}

void client_destroy(struct client_t *self) {
    free(self);
}

static
void client_process_get(struct client_t *self) {
    (void)self;
}

static
void client_process_head(struct client_t *self) {
    (void)self;
}

static
void client_process_post(struct client_t *self) {
    (void)self;
}

static
void client_process(struct client_t *self) {
    memset(&self->request, 0, sizeof(self->request));
    memset(&self->response, 0, sizeof(self->response));
    if (http_parse(&self->request, self->data, self->data_length) != 0
            || self->request.method == NULL || self->request.url == NULL
            || self->request.version == NULL) {
        self->response.status_code = 400;
    } else if (strcmp(self->request.method, "GET") == 0) {
        client_process_get(self);
    } else if (strcmp(self->request.method, "HEAD") == 0) {
        client_process_head(self);
    } else if (strcmp(self->request.method, "POST") == 0) {
        client_process_post(self);
    } else {
        self->response.status_code = 400;
    }
    self->state = STATE_SEND_HEAD;
}

void client_handle_recvhead(struct client_t *self) {
    int len;

    len = recv(self->remote_fd, self->data + self->data_length,
            sizeof(self->data) - self->data_length, 0);
    A_LOG("client: recv %d", len);
    if (len == -1) {
        if (errno == EAGAIN) {
            A_LOG("client: recv blocked %d", self->remote_fd);
            return;
        }
        self->state = STATE_NONE;
    }
    self->data_length += len;
    /* check header termination */
    if (self->data_length > 4) {
        if (memcmp(&self->data[self->data_length - 4], "\r\n\r\n", 4) == 0) {
            client_process(self);
        }
    } else if (self->data_length > 2) {
        if (memcmp(&self->data[self->data_length - 2], "\n\n", 2) == 0) {
            client_process(self);
        }
    }
    if (len >= MAX_REQUEST_LENGTH) {
        self->state = STATE_SEND_HEAD;
    }
    if (self->state == STATE_SEND_HEAD) {
        client_handle_sendhead(self);
    }
}

void client_handle_sendhead(struct client_t *self) {
    (void)self;
}

void client_handle_recvfile(struct client_t *self) {
    (void)self;
}

void client_handle_sendfile(struct client_t *self) {
    (void)self;
}


/* vim: set ts=4 sw=4 expandtab: */
