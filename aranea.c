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
#include "aranea.h"
#include "config.h"

static struct server_t server_;
static struct client_t *list_client_ = NULL;
static time_t cur_time_;
static unsigned int flags_ = 0;

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
    struct timeval timeout;
    struct client_t *c, *tc;

    while (!(flags_ & FLAG_QUIT)) {
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        max_rfd = max_wfd = -1;
        SERVER_SETFD_(server_.fd, &rfds, max_rfd);

        cur_time_ = time(NULL);
        A_FOREACH(list_client_, c) {
            if (c->timeout > cur_time_) {
                c->state = STATE_NONE;
                continue;
            }
            switch (c->state) {
            case STATE_RECV_HEADER:
                SERVER_SETFD_(c->remote_fd, &rfds, max_rfd);
                break;
            case STATE_SEND_HEADER:
                SERVER_SETFD_(c->remote_fd, &wfds, max_wfd);
                break;
            case STATE_SEND_CONTENT:
                SERVER_SETFD_(c->remote_fd, &wfds, max_wfd);
                break;
            }
        }
        timeout.tv_sec = TIMEOUT;
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
        cur_time_ = time(NULL);
        if (FD_ISSET(server_.fd, &rfds)) {
            c = server_accept(&server_);
            if (c != NULL) {
                c->timeout = cur_time_ + TIMEOUT;
                client_handle_recvheader(c);
            }
        }
        A_FOREACH_S(list_client_, c, tc) {
            switch (c->state) {
            case STATE_RECV_HEADER:
                if (FD_ISSET(c->remote_fd, &rfds)) {
                    client_handle_recvheader(c);
                }
                break;
            case STATE_SEND_HEADER:
                if (FD_ISSET(c->remote_fd, &wfds)) {
                    client_handle_sendheader(c);
                }
                break;
            case STATE_SEND_CONTENT:
                break;
            }
            if (c->state == STATE_NONE) {
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
    (void)argc;
    (void)argv;

    server_.port = PORT;
    if (server_init(&server_) != 0) {
        return 1;
    }
    server_poll();

    return 0;
}
