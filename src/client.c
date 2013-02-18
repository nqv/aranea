/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <aranea/aranea.h>

#define CLIENT_CLOSEFD_(fd)     \
    do {                        \
        close(fd);              \
        fd = -1;                \
    } while (0)

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

void client_detach(struct client_t *self) {
    if (self->next != NULL) {
        self->next->prev = self->prev;
    }
    *(self->prev) = self->next;
}

void client_close(struct client_t *self) {
    if (self->remote_fd != -1) {
        CLIENT_CLOSEFD_(self->remote_fd);
    }
    if (self->local_rfd != -1) {
        CLIENT_CLOSEFD_(self->local_rfd);
    }
}

/** Set client to initial state
 */
void client_init(struct client_t *self) {
    self->remote_fd = -1;
    self->local_rfd = -1;
    self->ip[0] = '\0';
    self->state = STATE_NONE;
    client_reset(self);
}

/** Reset client but keep connection status
 */
void client_reset(struct client_t *self) {
    self->data_length = 0;
    self->data_sent = 0;
    self->file_sent = 0;
    self->flags = 0;
    memset(&self->request, 0, sizeof(self->request));
    memset(&self->response, 0, sizeof(self->response));
}

/** Open and get file information
 * Set response.status_code on error.
 */
static
int client_open_file(struct client_t *self, const char *path) {
    struct stat st;

    self->local_rfd = open(path, O_RDONLY | O_NONBLOCK);
    if (self->local_rfd == -1) {
        A_ERR("open: %s %s", path, strerror(errno));
        switch (errno) {
        case EACCES:
            self->response.status_code = HTTP_STATUS_FORBIDDEN;
            break;
        case ENOENT:
            self->response.status_code = HTTP_STATUS_NOTFOUND;
            break;
        default:
            self->response.status_code = HTTP_STATUS_SERVERERROR;
            break;
        }
        return -1;
    }
    /* get information */
    if (fstat(self->local_rfd, &st) == -1) {
        A_ERR("stat: %s", strerror(errno));
        self->response.status_code = HTTP_STATUS_SERVERERROR;
        goto err;
    }
    /* make sure it's a regular file */
    if (S_ISDIR(st.st_mode) || !S_ISREG(st.st_mode)) {
        A_ERR("not a regular file 0x%x", st.st_mode);
        self->response.status_code = HTTP_STATUS_FORBIDDEN;
        goto err;
    }
    self->response.last_mod = st.st_mtime;
    self->response.total_length = st.st_size;
    self->response.content_length = st.st_size;
    self->response.content_type = mimetype_get(path);
    self->response.content_from = 0;
    return 0;

err:
    CLIENT_CLOSEFD_(self->local_rfd);
    return -1;
}

static
int client_check_filemod(struct client_t *self) {
    char date[MAX_DATE_LENGTH];
    int len;

    if (self->request.header[HEADER_IFMODIFIEDSINCE] == NULL) {
        return 1;
    }
    len = strftime(date, sizeof(date), DATE_FORMAT,
            gmtime(&self->response.last_mod));
    if (strncmp(self->request.header[HEADER_IFMODIFIEDSINCE], date, len) == 0) {
        return 0;
    }
    return -1;
}

/**
 * Convert FROM-TO to FROM/TOTAL
 * Set response.status_code on error.
 */
static
int client_check_filerange(struct client_t *self) {
    off_t pos;

    A_LOG("rangefrom=%s rangeto=%s", self->request.range_from,
            self->request.range_to);
    if (self->request.range_from != NULL) {
        self->response.content_from = strtol(self->request.range_from, NULL, 10);

        if (self->response.content_from >= self->response.content_length) {
            goto err;
        }
        if (self->request.range_to != NULL) {
            /* 0-99 = 0/100 */
            pos = strtol(self->request.range_to, NULL, 10);
            if (pos < self->response.content_from) {
                goto err;
            }
            self->response.content_length = pos - self->response.content_from + 1;
        } else {                                /* To is not given */
            /* 4- = 4/95 */
            self->response.content_length -= self->response.content_from - 1;
        }
    } else {                                    /* From is not given */
        if (self->request.range_to != NULL) {
            /* -10 = 89/10 */
            pos = strtol(self->request.range_to, NULL, 10);
            if (pos >= self->response.content_length) {
                goto err;
            }
            self->response.content_from = self->response.content_length - pos;
            self->response.content_length = pos;
        } else {
            return 1;                           /* not available */
        }
    }
    A_LOG("from=%ld len=%ld", self->response.content_from,
            self->response.content_length);
    return 0;
err:
    self->response.status_code = HTTP_STATUS_RANGENOTSATISFIABLE;
    return -1;
}

/**
 * Response header is generated if ok.
 */
static
int client_respond(struct client_t *self) {
    int len;
    char path[MAX_PATH_LENGTH];

    /* clean up */
    http_decode_url(self->request.url);
    http_sanitize_url(self->request.url);

#if HAVE_AUTH == 1
    if (auth_check(&self->request) != 0) {
        self->response.status_code = HTTP_STATUS_AUTHORIZATIONREQUIRED;
        self->response.realm = AUTH_REALM;
        return -1;
    }
#endif

    /* get path in fs */
    len = http_get_realpath(self->request.url, path);

#if HAVE_CGI == 1
    if (cgi_hit(path, len)) {
        if (cgi_is_executable(path, self) != 0) {
            return -1;
        }
        if (self->flags & CLIENT_FLAG_HEADERONLY) {
            self->response.status_code = HTTP_STATUS_OK;
            self->data_length = http_gen_header(&self->response, self->data,
                    sizeof(self->data), HTTP_FLAG_END);
            self->state = STATE_SEND_HEADER;
            return 0;
        }
        return cgi_exec(path, self);
    }
#endif  /* HAVE_CGI */

    /* in case user-agent would prefer keeping this connection (but not
     * for cgi (POST) request) */
    if (self->request.header[HEADER_CONNECTION] != NULL
            && strcmp("keep-alive", self->request.header[HEADER_CONNECTION]) == 0) {
        self->flags |= CLIENT_FLAG_KEEPALIVE;
    }
    /* open file */
    if (client_open_file(self, path) != 0) {
        return -1;
    }
    /* generate header */
    if (client_check_filemod(self) == 0) {
        CLIENT_CLOSEFD_(self->local_rfd);
        self->response.status_code = HTTP_STATUS_NOTMODIFIED;
        self->data_length = http_gen_header(&self->response, self->data,
                sizeof(self->data), HTTP_FLAG_END);
        self->state = STATE_SEND_HEADER;
        return 0;
    }
    len = client_check_filerange(self);
    if (len < 0) {
        CLIENT_CLOSEFD_(self->local_rfd);
        return -1;
    }
    if (len == 0) {
        self->response.status_code = HTTP_STATUS_PARTIALCONTENT;
        self->data_length = http_gen_header(&self->response, self->data,
                sizeof(self->data), HTTP_FLAG_ACCEPT | HTTP_FLAG_CONTENT
                | HTTP_FLAG_RANGE | HTTP_FLAG_END);
        self->state = STATE_SEND_HEADER;
        return 0;
    }
    if (self->flags & CLIENT_FLAG_HEADERONLY) {
        CLIENT_CLOSEFD_(self->local_rfd);
    }
    self->response.status_code = HTTP_STATUS_OK;
    self->data_length = http_gen_header(&self->response, self->data,
            sizeof(self->data), HTTP_FLAG_ACCEPT | HTTP_FLAG_CONTENT
            | HTTP_FLAG_END);
    self->state = STATE_SEND_HEADER;
    return 0;
}

/** Parse header and set state for the client
 */
void client_process(struct client_t *self) {
    int ret;
    if (http_parse(&self->request, self->data, self->data_length) != 0
            || self->request.method == NULL || self->request.url == NULL
            || self->request.version == NULL) {
        self->response.status_code = HTTP_STATUS_BADREQUEST;
        ret = -1;
    } else if (strcmp(self->request.method, "GET") == 0) {
        ret = client_respond(self);
    } else if (strcmp(self->request.method, "HEAD") == 0) {
        self->flags |= CLIENT_FLAG_HEADERONLY;
        ret = client_respond(self);
    } else if (strcmp(self->request.method, "POST") == 0) {
        self->flags |= CLIENT_FLAG_POST;
        ret = client_respond(self);
    } else {
        self->response.status_code = HTTP_STATUS_NOTIMPLEMENTED;
        ret = -1;
    }
    if (ret != 0) {
        self->data_length = http_gen_errorpage(&self->response, self->data,
                sizeof(self->data));
        self->state = STATE_SEND_HEADER;
    }
}

/* vim: set ts=4 sw=4 expandtab: */
