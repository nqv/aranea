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
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
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
    if (self->next != NULL) {
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
    int len;
    char path[MAX_PATH_LENGTH];
    struct stat st;

    http_decode_url(self->request.url);
    http_sanitize_url(self->request.url);
    A_LOG("url=%s", self->request.url);

    /* get real path */
    /* snprintf is slower but safer than strcat/strncpy */
    if (g_config.root != NULL) {
        len = snprintf(path, sizeof(path), "%s%s", g_config.root,
            self->request.url);
    } else {                            /* chroot */
        len = snprintf(path, sizeof(path), "%s", self->request.url);
    }
    if (self->request.url[strlen(self->request.url) - 1] == '/') {
        /* url/index.html */
        len = snprintf(path + len, sizeof(path) - len, "%s", WWW_INDEX);
    }
    A_LOG("path=%s", path);
    /* open file */
    self->local_fd = open(path, O_RDONLY | O_NONBLOCK);
    if (self->local_fd == -1) {
        A_ERR("open: %s", strerror(errno));
        switch (errno) {
        case EACCES:
            self->response.status_code = 403;
            break;
        case ENOENT:
            self->response.status_code = 404;
            break;
        default:
            self->response.status_code = 500;
            break;
        }
        goto err2;
    }
    /* get information */
    if (fstat(self->local_fd, &st) == -1) {
        A_ERR("stat: %s", strerror(errno));
        self->response.status_code = 500;
        goto err;
    }
    /* make sure it's a regular file */
    if (S_ISDIR(st.st_mode) || !S_ISREG(st.st_mode)) {
        A_ERR("not a regular file 0x%x", st.st_mode);
        self->response.status_code = 403;
        goto err;
    }
    self->response.content_length = st.st_size;
    self->response.content_type = mimetype_get(path);
    self->file_from = 0;
    self->file_sent = 0;
    /* generate header */
    if (self->request.if_mod_since != NULL) {
        A_LOG("ifmodsince %s", self->request.if_mod_since);
    }
    if (self->request.range_from != NULL || self->request.range_to != NULL) {
        A_LOG("rangefrom=%s rangeto=%s", self->request.range_from,
                self->request.range_to);
    }
    self->response.status_code = 200;
    self->data_length = http_gen_header(&self->response, self->data,
            sizeof(self->data));
    self->data_sent = 0;
    return;

err:
    if (self->local_fd != -1) {
        close(self->local_fd);
        self->local_fd = -1;
    }
err2:
    self->data_length = http_gen_errorpage(&self->response, self->data,
            sizeof(self->data));
    self->data_sent = 0;
}

static
void client_process_head(struct client_t *self) {
    (void)self;
}

static
void client_process_post(struct client_t *self) {
    (void)self;
}

/** Parse header and set state for the client
 */
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
    self->state = STATE_SEND_HEADER;
}

/** Read header from socket
 */
void client_handle_recvheader(struct client_t *self) {
    ssize_t len;

    len = recv(self->remote_fd, self->data + self->data_length,
            sizeof(self->data) - self->data_length, 0);
    A_LOG("client: recv %d", len);
    if (len <= 0) {
        if (len == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            /* retry later */
            A_LOG("client: recv blocked %d", self->remote_fd);
            return;
        }
        self->state = STATE_NONE;
        return;
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
        self->state = STATE_SEND_HEADER;
    }
    if (self->state == STATE_SEND_HEADER) {
        client_handle_sendheader(self);
    }
}

/** Send reply (header) via socket
 */
void client_handle_sendheader(struct client_t *self) {
    ssize_t len;

    if (self->data_length <= 0) {
        A_ERR("client: %d invalid len=%d", self->remote_fd, self->data_length);
        self->state = STATE_NONE;
        return;
    }
    len = send(self->remote_fd, self->data + self->data_sent,
                self->data_length - self->data_sent, 0);
    A_LOG("client: %d send %d/%d", self->remote_fd, len, self->data_length);
    if (len <= 0) {
        if (len == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return;
        }
        self->state = STATE_NONE;
        return;
    }
    self->data_sent += len;

    /* check if finish sending header */
    if (self->data_sent >= self->data_length) {
        A_LOG("client: %d finish sending header", self->remote_fd);
        if (self->local_fd == -1) {
            self->state = STATE_NONE;
        } else {
            self->state = STATE_SEND_FILE;
        }
    }
}

void client_handle_recvfile(struct client_t *self) {
    (void)self;
}

void client_handle_sendfile(struct client_t *self) {
    ssize_t len;
    off_t offset;

    offset = self->file_from + self->file_sent;
    len = sendfile(self->remote_fd, self->local_fd, &offset,
            self->response.content_length - self->file_sent);
    A_LOG("client: %d sendfile %d/%ld", self->remote_fd, len,
            self->response.content_length);
    if (len <= 0) {
        if (len == -1 && errno == EAGAIN) {
            A_LOG("client: %d sendfile blocked", self->remote_fd);
        } else {
            self->state = STATE_NONE;
        }
        return;
    }
    self->file_sent += len;
    if (self->file_sent >= self->response.content_length) {
        self->state = STATE_NONE;
    }
}

/* vim: set ts=4 sw=4 expandtab: */
