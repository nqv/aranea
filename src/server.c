/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
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

#include <aranea/aranea.h>

/**
 * Close, detach and free client
 */
static
void forget_client(struct client_t *c) {
    client_close(c);
    client_detach(c);
    clientpool_free(c);
}

int server_init(struct server_t *self) {
    struct addrinfo hints, *info, *p;
    int enable = 1;
    int fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family     = AF_UNSPEC;
    hints.ai_socktype   = SOCK_STREAM;
    hints.ai_flags      = AI_PASSIVE;

    fd = getaddrinfo(NULL, self->port, &hints, &info);
    if (fd != 0) {
        A_ERR("getaddrinfo: %s\n", gai_strerror(fd));
        return -1;
    }
    /* loop through all the results and bind to the first we can */
    for (p = info; p != NULL; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == -1) {
            A_ERR("socket: %s", strerror(errno));
            continue;
        }
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
            close(fd);
            A_ERR("setsockopt: %s", strerror(errno));
            return -1;
        }
        if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(fd);
            A_ERR("bind: %s", strerror(errno));
            continue;
        }
        break;
    }
    if (p == NULL) {
        A_ERR("bind: %s", strerror(errno));
        return -1;
    }
    freeaddrinfo(info);

    if (listen(fd, MAX_CONN) == -1) {
        close(fd);
        A_ERR("listen: %s", strerror(errno));
        return -1;
    }
    A_LOG("listen %s", self->port);
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
    int flags;
    struct client_t *c;

    len = sizeof(addr);
    fd = accept(self->fd, (struct sockaddr *)&addr, &len);
    if (fd == -1) {
        A_ERR("accept: %s", strerror(errno));
        return NULL;
    }
    /* set socket to non-blocking */
    flags = fcntl(fd, F_GETFL, NULL);
    if (flags == -1) {
        A_ERR("fcntl: F_GETFL %s", strerror(errno));
        goto err;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        A_ERR("fcntl: F_SETFL O_NONBLOCK %s", strerror(errno));
        goto err;
    }
#if HAVE_TCPCORK == 1
    flags = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_CORK, &flags,
            sizeof(flags)) == -1) {
        goto err;
    }
#endif
    c = clientpool_alloc();
    if (c == NULL) {
        goto err;
    }
    client_init(c);
    /* save client information */
    c->remote_fd = fd;
    c->state = STATE_RECV_HEADER;
    inet_ntop(addr.ss_family, SERVER_GETINADDR_(&addr), c->ip, sizeof(c->ip));
    A_LOG("accept %d %s", fd, c->ip);
    return c;
err:
    close(fd);
    return NULL;
}
#undef SERVER_GETINADDR_

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
    for (c = self->clients; c != NULL; ) {
        if (g_curtime > c->timeout) {
            A_LOG("timeout client %d", c->remote_fd);
            tc = c;
            c = c->next;
            forget_client(tc);
            continue;
        }
        switch (c->state) {
        case STATE_RECV_HEADER:
            SERVER_SETFD_(c->remote_fd, &rfds, max_rfd);
            break;
        case STATE_SEND_HEADER:
        case STATE_SEND_FILE:
            SERVER_SETFD_(c->remote_fd, &wfds, max_wfd);
            break;
        }
        c = c->next;
    }
    /* Poll timeout (to check quit flag) */
    timeout.tv_sec = SERVER_TIMEOUT;
    timeout.tv_usec = 0;
    for (;;) {
        num_fd = select((max_rfd > max_wfd) ? (max_rfd + 1) : (max_wfd + 1),
                &rfds, (max_wfd != -1) ? &wfds : NULL,
                NULL, &timeout);
        if (num_fd > 0) {
            break;
        }
        if (num_fd < 0) {
            /* Ignore Interrupted system call which caused by ending of a
               child process */
            if (errno == EINTR) {
                continue;
            } else {
                A_ERR("select: %s", strerror(errno));
                sleep(1);
            }
        }
        return;
    }
    g_curtime = time(NULL);
    chk_time = g_curtime + CLIENT_TIMEOUT;
    if (FD_ISSET(self->fd, &rfds)) {
        c = server_accept(self);
        if (c != NULL) {
            client_add(c, &self->clients);
            c->timeout = chk_time;
            /* Read header straightway */
            state_recv_header(c);
            if (c->state == STATE_NONE) {
                forget_client(c);
            }
        }
        --num_fd;
    }
    for (c = self->clients; num_fd > 0 && c != NULL; ) {
        switch (c->state) {
        case STATE_NONE:
            break;
        case STATE_RECV_HEADER:
            if (FD_ISSET(c->remote_fd, &rfds)) {
                c->timeout = chk_time;
                state_recv_header(c);
                --num_fd;
            }
            break;
        case STATE_SEND_HEADER:
            if (FD_ISSET(c->remote_fd, &wfds)) {
                c->timeout = chk_time;
                state_send_header(c);
                --num_fd;
            }
            break;
        case STATE_SEND_FILE:
            if (FD_ISSET(c->remote_fd, &wfds)) {
                c->timeout = chk_time;
                state_send_file(c);
                --num_fd;
            }
            break;
        default:
            A_LOG("client: %d invalid state %d", c->remote_fd, c->state);
            c->state = STATE_NONE;
            break;
        }
        if (c->state == STATE_NONE) {
            /* finished connection */
            A_LOG("close client %d", c->remote_fd);
            tc = c;
            c = c->next;
            forget_client(tc);
        } else {
            c = c->next;
        }
    }
}

/** Close all "unused" FDs
 * Called in child process after forked.
 */
void server_close_fds() {
    struct client_t *c;

    close(g_server.fd);
    for (c = g_server.clients; c != NULL; c = c->next) {
        close(c->remote_fd);
        if (c->local_rfd != -1) {
            close(c->local_rfd);
        }
    }
}

/* vim: set ts=4 sw=4 expandtab: */
