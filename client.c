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
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include "aranea.h"

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

void client_remove(struct client_t *self) {
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

/** Reset client to initial state
 */
void client_reset(struct client_t *self) {
    self->remote_fd = -1;
    self->local_rfd = -1;
    self->ip[0] = '\0';
    self->data_length = 0;
    self->data_sent = 0;
    self->file_sent = 0;
    self->state = STATE_NONE;
    self->flags = 0;
    memset(&self->request, 0, sizeof(self->request));
    memset(&self->response, 0, sizeof(self->response));
}

static
int get_realpath(const char *url, char *path) {
    int len;

    /* snprintf is slower but safer than strcat/strncpy */
    if (g_config.root != NULL) {
        len = snprintf(path, MAX_PATH_LENGTH, "%s%s", g_config.root, url);
    } else {                            /* chroot */
        len = snprintf(path, MAX_PATH_LENGTH, "%s", url);
    }
    if (url[strlen(url) - 1] == '/') {
        /* url/index.html */
        len = snprintf(path + len, MAX_PATH_LENGTH - len, "%s", WWW_INDEX);
    }
    return len;
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
            self->response.status_code = 403;
            break;
        case ENOENT:
            self->response.status_code = 404;
            break;
        default:
            self->response.status_code = 500;
            break;
        }
        return -1;
    }
    /* get information */
    if (fstat(self->local_rfd, &st) == -1) {
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

#if HAVE_CGI == 1
/**
 * Set response.status_code on error.
 */
static
int client_check_fileexec(struct client_t *self, const char *path) {
    struct stat st;

    if (access(path, X_OK) != 0) {
        self->response.status_code = 403;
        return -1;
    }
    if (stat(path, &st) == -1) {
        A_ERR("stat: %s", strerror(errno));
        self->response.status_code = 500;
        return -1;
    }
    if (S_ISDIR(st.st_mode)) {
        self->response.status_code = 403;
        return -1;
    }
    return 0;
}
#endif  /* HAVE_CGI */

static
int client_check_filemod(struct client_t *self) {
    char date[MAX_DATE_LENGTH];
    int len;

    if (self->request.if_mod_since == NULL) {
        return 1;
    }
    len = strftime(date, sizeof(date), DATE_FORMAT,
            gmtime(&self->response.last_mod));
    if (strncmp(self->request.if_mod_since, date, len) == 0) {
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
        } else {                        /* To is not given */
            /* 4- = 4/95 */
            self->response.content_length -= self->response.content_from - 1;
        }
    } else {                            /* From is not given */
        if (self->request.range_to != NULL) {
            /* -10 = 89/10 */
            pos = strtol(self->request.range_to, NULL, 10);
            if (pos >= self->response.content_length) {
                goto err;
            }
            self->response.content_from = self->response.content_length - pos;
            self->response.content_length = pos;
        } else {
            return 1;                   /* not available */
        }
    }
    A_LOG("from=%ld len=%ld", self->response.content_from,
            self->response.content_length);
    return 0;
err:
    self->response.status_code = 416;
    return -1;
}

#if HAVE_CGI == 1
static
int client_process_cgi(struct client_t *self, const char *path) {
    char *argv[2];
    char *envp[MAX_CGIENV_ITEM];
    int fds[2];
    pid_t pid;
    int i, o, e;                /* new stdin, stdout, stderr */

    if (pipe(fds) == -1) {
        A_ERR("pipe %s", strerror(errno));
        self->response.status_code = 500;
        return -1;
    }
#if 0
    /* set socket back to blocking */
    if (fcntl(self->remote_fd, F_SETFL, 0) == -1) {
        A_ERR("fcntl: F_SETFL O_NONBLOCK %s", strerror(errno));
        self->response.status_code = 500;
        return -1;
    }
#endif
#if HAVE_VFORK == 1
    pid = vfork();
#else
    pid = fork();
#endif  /* HAVE_VFORK */
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        self->response.status_code = 500;
        return -1;
    }
    if (pid == 0) {                 /* child */
        {   /* close unused fds */
            struct client_t *c;
            close(fds[0]);
            close(g_server.fd);
            for (c = g_listclient; c != NULL; c = c->next) {
                close(c->remote_fd);
                if (c->local_rfd != -1) {
                    close(c->local_rfd);
                }
            }
        }
        /* TODO: pipe POST and close all other sockets */
        /* new stdout = pipe[1] */
        o = fds[1];                 /* write end */
        e = open("/dev/null", O_WRONLY);
        if (o != STDOUT_FILENO) {
            dup2(o, STDOUT_FILENO);
            close(o);
        }
        if (e != STDERR_FILENO) {
            dup2(e, STDERR_FILENO);
            close(e);
        }
        /* exec */
        argv[0] = (char *)path;
        argv[1] = NULL;
        i = cgi_gen_env(&self->request, envp);
        envp[i] = NULL;
        execve(path, argv, envp);
        _exit(1);                   /* exec error */
    }
    /* parent */
    close(fds[1]);
    self->local_rfd = fds[0];       /* read end */
    self->response.status_code = 200;
    /* send minimal header */
    self->data_length = http_gen_header(&self->response, self->data,
            sizeof(self->data), 0);
    A_LOG("minimal cgi header %d", self->data_length);
    self->flags = CLIENT_FLAG_CGI;
    return 0;
}
#endif  /* HAVE_CGI */

/**
 * Response header is generated if ok.
 */
static
int client_process_get(struct client_t *self, const int header_only) {
    int len;
    char path[MAX_PATH_LENGTH];

    /* clean up */
    http_decode_url(self->request.url);
    http_sanitize_url(self->request.url);
    /* get path in fs */
    len = get_realpath(self->request.url, path);
    A_LOG("path=%s", path);
#if HAVE_CGI == 1
    if (is_cgi(path, len)) {
        if (client_check_fileexec(self, path) != 0) {
            return -1;
        }
        if (header_only) {
            self->response.status_code = 200;
            self->data_length = http_gen_header(&self->response, self->data,
                    sizeof(self->data), HTTP_FLAG_END);
            return 0;
        }
        return client_process_cgi(self, path);
    }
#endif  /* HAVE_CGI */
    /* open file */
    if (client_open_file(self, path) != 0) {
        return -1;
    }
    /* generate header */
    if (client_check_filemod(self) == 0) {
        self->response.status_code = 304;
        self->data_length = http_gen_header(&self->response, self->data,
                sizeof(self->data), 0);
        return 0;
    }
    len = client_check_filerange(self);
    if (len < 0) {
        CLIENT_CLOSEFD_(self->local_rfd);
        return -1;
    }
    if (len == 0) {
        self->response.status_code = 206;
        self->data_length = http_gen_header(&self->response, self->data,
                sizeof(self->data),
                HTTP_FLAG_CONTENT | HTTP_FLAG_RANGE | HTTP_FLAG_END);
        return 0;
    }
    if (header_only) {
        if (self->local_rfd != -1) {
            CLIENT_CLOSEFD_(self->local_rfd);
        }
    }
    self->response.status_code = 200;
    self->data_length = http_gen_header(&self->response, self->data,
            sizeof(self->data), HTTP_FLAG_CONTENT | HTTP_FLAG_END);
    return 0;
}

#if HAVE_HTTPPOST == 1
static
int client_process_post(struct client_t *self) {
    (void)self;
    return 0;
}
#endif  /* HAVE_HTTPPOST */

/** Parse header and set state for the client
 */
static
void client_process(struct client_t *self) {
    int ret;
    if (http_parse(&self->request, self->data, self->data_length) != 0
            || self->request.method == NULL || self->request.url == NULL
            || self->request.version == NULL) {
        self->response.status_code = 400;
        ret = -1;
    } else if (strcmp(self->request.method, "GET") == 0) {
        ret = client_process_get(self, 0);
    } else if (strcmp(self->request.method, "HEAD") == 0) {
        ret = client_process_get(self, 1);
#if HAVE_HTTPPOST == 1
    } else if (strcmp(self->request.method, "POST") == 0) {
        ret = client_process_post(self);
#endif  /* HAVE_HTTPPOST */
    } else {
        self->response.status_code = 501;
        ret = -1;
    }
    if (ret != 0) {
        self->data_length = http_gen_errorpage(&self->response, self->data,
                sizeof(self->data));
    }
    /* always reply header */
    self->state = STATE_SEND_HEADER;
}

/** Read header from socket
 */
void client_handle_recvheader(struct client_t *self) {
    ssize_t len;

    len = recv(self->remote_fd, self->data + self->data_length,
            sizeof(self->data) - self->data_length, 0);
    if (len <= 0) {
        if (len == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            /* retry later */
            A_LOG("client: %d recv %s", self->remote_fd, strerror(errno));
            return;
        }
        self->state = STATE_NONE;
        return;
    }
    self->data_length += len;
    /* check header termination */
    if (self->data_length > 4) {
        /* compare last 4 chars */
        if (memcmp(&self->data[self->data_length - 4], "\r\n\r\n", 4) == 0
                || memcmp(&self->data[self->data_length - 2], "\n\n", 2) == 0) {
            client_process(self);
        } else if (self->data_length >= MAX_REQUEST_LENGTH) {
            /* not found */
            self->response.status_code = 413;
            self->data_length = http_gen_errorpage(&self->response, self->data,
                    sizeof(self->data));
            self->state = STATE_SEND_HEADER;
        }
    }
    if (self->state == STATE_SEND_HEADER) {
        client_handle_sendheader(self);
    }
}

/** Send reply (header) via socket
 */
void client_handle_sendheader(struct client_t *self) {
    ssize_t len;

#if 0
    if (self->data_length <= 0) {
        A_ERR("client: %d invalid len=%d", self->remote_fd, self->data_length);
        self->state = STATE_NONE;
        return;
    }
#endif
    len = send(self->remote_fd, self->data + self->data_sent,
                self->data_length - self->data_sent, 0);
    if (len <= 0) {
        A_LOG("client: %d send %s", self->remote_fd, strerror(errno));
        if (len == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return;
        }
        self->state = STATE_NONE;
        return;
    }
    self->data_sent += len;

    /* check if finish sending header */
    if (self->data_sent >= self->data_length) {
        self->data_length = self->data_sent = 0;
        if (self->local_rfd == -1) {
            self->state = STATE_NONE;
        } else {
            self->state = (self->flags & CLIENT_FLAG_CGI)
                    ? STATE_RECV_PIPE
                    : STATE_SEND_FILE;
        }
        A_LOG("client: %d finish sending header, state=%d", self->remote_fd,
                self->state);
    }
}

void client_handle_sendfile(struct client_t *self) {
    ssize_t len;
    off_t offset;

    offset = self->response.content_from + self->file_sent;
    len = sendfile(self->remote_fd, self->local_rfd, &offset,
            self->response.content_length - self->file_sent);
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

void client_handle_recvpipe(struct client_t *self) {
    self->data_length = read(self->local_rfd, self->data, sizeof(self->data));
    if (self->data_length <= 0) {       /* cgi finish */
        A_LOG("client: %d recvpipe %s", self->remote_fd, strerror(errno));
        if (self->data_length < 0) {
            A_ERR("recv %s", strerror(errno));
        }
        CLIENT_CLOSEFD_(self->local_rfd);
        self->state = STATE_NONE;
    } else {
        self->data_sent = 0;
        self->state = STATE_SEND_PIPE;
    }
}

void client_handle_sendpipe(struct client_t *self) {
    ssize_t len;

    /* send remaining data */
    len = send(self->remote_fd, self->data + self->data_sent,
        self->data_length - self->data_sent, 0);
    if (len <= 0) {
        A_LOG("client: %d sendpipe %s", self->remote_fd, strerror(errno));
        if (len == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return;
        }
        self->state = STATE_NONE;
        return;
    }
    self->data_sent += len;

    if (self->data_sent >= self->data_length) {
        /* continue reading */
        if (self->local_rfd == -1) {
            self->state = STATE_NONE;
        } else {
            self->state = STATE_RECV_PIPE;
        }
    }
}

#undef CLIENT_CLOSEFD_

/* vim: set ts=4 sw=4 expandtab: */
