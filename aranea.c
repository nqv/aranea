/* Aranea
 * Copyright (c) 2011, Nguyen Quoc Viet
 * See LICENSE file for copyright and license details.
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <signal.h>
#include <wait.h>
#include "aranea.h"

/* global vars */
time_t g_curtime;
struct config_t g_config;
char g_cgienv[MAX_CGIENV_LENGTH];

static struct server_t server_;
static struct client_t *list_client_ = NULL;
static unsigned int flags_ = 0;

static
void handle_signal(int sig) {
    switch (sig) {
    case SIGCHLD:
        /* wait until no more zoombie children */
        while (waitpid(-1, NULL, WNOHANG) > 0);
        break;
    }
}

#define SERVER_SETFD_(fd, set, max)     \
    do {                                \
        FD_SET(fd, set);                \
        if (fd > max) {                 \
            max = fd;                   \
        }                               \
    } while (0)
static
void server_poll() {
    fd_set rfds, wfds;
    int max_rfd, max_wfd;
    int rv;
    time_t chk_time;
    struct timeval timeout;
    struct client_t *c, *tc;

    while (!(flags_ & FLAG_QUIT)) {
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        max_rfd = max_wfd = -1;
        SERVER_SETFD_(server_.fd, &rfds, max_rfd);

        g_curtime = time(NULL);
        A_FOREACH(list_client_, c) {
            if (g_curtime > c->timeout) {
                A_LOG("client: timedout %d", c->remote_fd);
                c->state = STATE_NONE;
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
                SERVER_SETFD_(c->local_fd, &rfds, max_wfd);
                break;
            }
        }
        timeout.tv_sec = SERVER_TIMEOUT;
        timeout.tv_usec = 0;
        rv = select((max_rfd > max_wfd) ? (max_rfd + 1) : (max_wfd + 1),
                &rfds, (max_wfd != -1) ? &wfds : NULL,
                NULL, &timeout);
        if (rv == 0) {
            continue;
        }
        if (rv == -1) {
            A_ERR("server: select %s", strerror(errno));
            sleep(1);
            continue;
        }
        A_LOG("server: select %d", rv);
        g_curtime = time(NULL);
        chk_time = g_curtime + CLIENT_TIMEOUT;
        if (FD_ISSET(server_.fd, &rfds)) {
            c = server_accept(&server_);
            if (c != NULL) {
                client_add(c, &list_client_);
                c->timeout = chk_time;
                client_handle_recvheader(c);
            }
        }
        A_FOREACH_S(list_client_, c, tc) {
            A_LOG("client: %d state=%d", c->remote_fd, c->state);
            switch (c->state) {
            case STATE_NONE:
                break;
            case STATE_RECV_HEADER:
                if (FD_ISSET(c->remote_fd, &rfds)) {
                    c->timeout = chk_time;
                    client_handle_recvheader(c);
                }
                break;
            case STATE_SEND_HEADER:
                if (FD_ISSET(c->remote_fd, &wfds)) {
                    c->timeout = chk_time;
                    client_handle_sendheader(c);
                }
                break;
            case STATE_SEND_FILE:
                if (FD_ISSET(c->remote_fd, &wfds)) {
                    c->timeout = chk_time;
                    client_handle_sendfile(c);
                }
                break;
            case STATE_RECV_PIPE:
                if (FD_ISSET(c->local_fd, &rfds)) {
                    c->timeout = chk_time;
                    client_handle_recvpipe(c);
                }
                break;
            case STATE_SEND_PIPE:
                if (FD_ISSET(c->remote_fd, &wfds)) {
                    c->timeout = chk_time;
                    client_handle_sendpipe(c);
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
                client_close(c);
                client_remove(c);
                client_destroy(c);
            }
        }
    }
}
#undef SERVER_SETFD_

int main(int argc, char **argv) {
    /* parse arguments */
    if (argc < 2) {
        g_config.root = ".";                /* current dir */
    } else {
        g_config.root = argv[1];
    }
    {   /* init signal*/
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = &handle_signal;
        if (sigaction(SIGCHLD, &sa, NULL) < 0) {
            A_ERR("sigaction %s", strerror(errno));
            return 1;
        }
    }
    server_.port = PORT;
    if (server_init(&server_) != 0) {
        return 1;
    }
    server_poll();
    {   /* clean up */
        struct client_t *c, *tc;
        A_FOREACH_S(list_client_, c, tc) {
            client_close(c);
            client_destroy(c);
        }
    }
    return 0;
}

/* vim: set ts=4 sw=4 expandtab: */
