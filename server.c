/* Aranea
 * Copyright (c) 2011, Nguyen Quoc Viet
 * See LICENSE file for copyright and license details.
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
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

    if (listen(fd, MAX_CONN) == -1) {
        close(fd);
        A_ERR("server: listen %s", strerror(errno));
        return -1;
    }
    A_LOG("server: listen %d", self->port);
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
#if 0
        flags = 1;
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flags,
                sizeof(flags)) == -1) {
            goto err;
        }
#endif
    }
    c = alloc_client();
    if (c == NULL) {
        goto err;
    }
    client_reset(c);
    /* save client information */
    c->remote_fd = fd;
    c->state = STATE_RECV_HEADER;
    inet_ntop(addr.ss_family, SERVER_GETINADDR_(&addr), c->ip, sizeof(c->ip));
    A_LOG("server: accept %d %s", fd, c->ip);
    return c;
err:
    close(fd);
    return NULL;
}
#undef SERVER_GETINADDR_

#define SERVER_REMCLIENT_(c)            \
    do {                                \
        client_close(c);                \
        client_remove(c);               \
        detach_client(c);               \
    } while (0)
#define SERVER_SETFD_(fd, set, max)     \
    do {                                \
        FD_SET(fd, set);                \
        if (fd > max) {                 \
            max = fd;                   \
        }                               \
    } while (0)
void server_poll(struct server_t *self) {
    fd_set rfds, wfds;
    int max_rfd, max_wfd;
    int num_fd;
    time_t chk_time;
    struct timeval timeout;
    struct client_t *c, *tc;

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    max_rfd = max_wfd = -1;
    SERVER_SETFD_(self->fd, &rfds, max_rfd);

    g_curtime = time(NULL);
    for (c = g_listclient; c != NULL; ) {
        if (g_curtime > c->timeout) {
            A_LOG("client: timedout %d", c->remote_fd);
            tc = c;
            c = c->next;
            SERVER_REMCLIENT_(tc);
            continue;
        }
        switch (c->state) {
        case STATE_RECV_HEADER:
            SERVER_SETFD_(c->remote_fd, &rfds, max_rfd);
            break;
        case STATE_SEND_HEADER:
        case STATE_SEND_FILE:
        case STATE_SEND_PIPE:
            SERVER_SETFD_(c->remote_fd, &wfds, max_wfd);
            break;
        case STATE_RECV_PIPE:
            SERVER_SETFD_(c->local_rfd, &rfds, max_wfd);
            break;
        }
        c = c->next;
    }
    timeout.tv_sec = SERVER_TIMEOUT;
    timeout.tv_usec = 0;
    num_fd = select((max_rfd > max_wfd) ? (max_rfd + 1) : (max_wfd + 1),
            &rfds, (max_wfd != -1) ? &wfds : NULL,
            NULL, &timeout);
    if (num_fd == 0) {
        return;
    }
    if (num_fd == -1) {
        A_ERR("server: select %s", strerror(errno));
        sleep(1);
        return;
    }
    A_LOG("server: select %d", num_fd);
    g_curtime = time(NULL);
    chk_time = g_curtime + CLIENT_TIMEOUT;
    if (FD_ISSET(self->fd, &rfds)) {
        c = server_accept(self);
        if (c != NULL) {
            client_add(c, &g_listclient);
            c->timeout = chk_time;
            client_handle_recvheader(c);
            if (c->state == STATE_NONE) {
                SERVER_REMCLIENT_(c);
            }
        }
        --num_fd;
    }
    for (c = g_listclient; num_fd > 0 && c != NULL; ) {
        A_LOG("client: %d state=%d", c->remote_fd, c->state);
        switch (c->state) {
        case STATE_NONE:
            break;
        case STATE_RECV_HEADER:
            if (FD_ISSET(c->remote_fd, &rfds)) {
                c->timeout = chk_time;
                client_handle_recvheader(c);
                --num_fd;
            }
            break;
        case STATE_SEND_HEADER:
            if (FD_ISSET(c->remote_fd, &wfds)) {
                c->timeout = chk_time;
                client_handle_sendheader(c);
                --num_fd;
            }
            break;
        case STATE_SEND_FILE:
            if (FD_ISSET(c->remote_fd, &wfds)) {
                c->timeout = chk_time;
                client_handle_sendfile(c);
                --num_fd;
            }
            break;
        case STATE_RECV_PIPE:
            if (FD_ISSET(c->local_rfd, &rfds)) {
                c->timeout = chk_time;
                client_handle_recvpipe(c);
                --num_fd;
            }
            break;
        case STATE_SEND_PIPE:
            if (FD_ISSET(c->remote_fd, &wfds)) {
                c->timeout = chk_time;
                client_handle_sendpipe(c);
                --num_fd;
            }
            break;
        default:
            A_LOG("client: %d invalid state %d", c->remote_fd, c->state);
            c->state = STATE_NONE;
            break;
        }
        if (c->state == STATE_NONE) {
            A_LOG("server: close %d", c->remote_fd);
            /* finished connection */
            struct client_t *tc = c;
            c = c->next;
            SERVER_REMCLIENT_(tc);
        } else {
            c = c->next;
        }
    }
}
#undef SERVER_REMCLIENT_
#undef SERVER_SETFD_

/* vim: set ts=4 sw=4 expandtab: */
