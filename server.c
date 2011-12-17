/* Aranea
 * Copyright (c) 2011, Nguyen Quoc Viet
 * See LICENSE file for copyright and license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "aranea.h"

int server_init(struct server_t *self) {
    struct addrinfo hints, *info, *p;
    int enable = 1;
    char buf[8];
    int fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family     = AF_UNSPEC;
    hints.ai_socktype   = SOCK_STREAM;
    hints.ai_flags      = AI_PASSIVE;

    snprintf(buf, sizeof(buf), "%d", self->port);
    fd = getaddrinfo(NULL, buf, &hints, &info);
    if (fd != 0) {
        A_ERR("getaddrinfo: %s\n", gai_strerror(fd));
        return -1;
    }
    /* loop through all the results and bind to the first we can */
    for (p = info; p != NULL; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == -1) {
            A_ERR("server: socket %s", strerror(errno));
            continue;
        }
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
            close(fd);
            A_ERR("setsockopt %s", strerror(errno));
            return -1;
        }
        if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(fd);
            A_ERR("server: bind %s", strerror(errno));
            continue;
        }
        break;
    }
    if (p == NULL) {
        A_ERR("server: failed to bind %s", strerror(errno));
        return -1;
    }
    freeaddrinfo(info);

    if (listen(fd, BACKLOG) == -1) {
        close(fd);
        A_ERR("server: listen %s", strerror(errno));
        return -1;
    }
    self->fd = fd;
    return 0;
}

/**
 * get sockaddr, IPv4 or IPv6:
 */
#define SERVER_GETINADDR_(sa)                           \
    (((struct sockaddr *)(sa))->sa_family == AF_INET)   \
    ? (void *)&(((struct sockaddr_in *)(sa))->sin_addr)         \
    : (void *)&(((struct sockaddr_in6 *)(sa))->sin6_addr)

struct client_t *server_accept(struct server_t *self) {
    socklen_t len;
    struct sockaddr_storage addr;
    int fd;
    struct client_t *c;

    len = sizeof(addr);
    fd = accept(self->fd, (struct sockaddr *)&addr, &len);
    A_LOG("server: accept %d", fd);
    if (fd == -1) {
        A_ERR("server: accept %s", strerror(errno));
        return NULL;
    }
    {   /* set socket to non-blocking */
        int flags = fcntl(fd, F_GETFL, NULL);
        if (flags == -1) {
            A_ERR("fcntl: F_GETFL %s", strerror(errno));
            goto err;
        }
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            A_ERR("fcntl: F_SETFL O_NONBLOCK %s", strerror(errno));
            goto err;
        }
    }
    c = client_new();
    if (c == NULL) {
        goto err;
    }
    /* save client information */
    c->remote_fd = fd;
    c->state = STATE_RECV_HEAD;
    inet_ntop(addr.ss_family, SERVER_GETINADDR_(&addr), c->ip, sizeof(c->ip));
    return c;
err:
    close(fd);
    return NULL;
}
#undef SERVER_GETINADDR_

/* vim: set ts=4 sw=4 expandtab: */
