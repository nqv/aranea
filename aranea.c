/* Aranea
 * Copyright (c) 2011, Nguyen Quoc Viet
 * See LICENSE file for copyright and license details.
 */

#include <stdio.h>
#include <stdlib.h>
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
struct client_t *g_listclient = NULL;
struct server_t g_server;

/* save allocated clients to reduce number of malloc call */
struct client_t *poolclient_ = NULL;
int poolclient_len_ = 0;
static unsigned int flags_ = 0;

/** Get client from pool if possible
 */
struct client_t *alloc_client() {
    struct client_t *cli;

    if (poolclient_ != NULL) {
        cli = poolclient_;
        client_remove(cli);
        --poolclient_len_;
    } else {
        cli = malloc(sizeof(struct client_t));
    }
    return cli;
}

void detach_client(struct client_t *cli) {
    if (poolclient_len_ > NUM_CACHED_CONN) {
        free(cli);                          /* simply discard it */
    } else {
        client_add(cli, &poolclient_);
        ++poolclient_len_;
    }
}

static
void handle_signal(int sig) {
    switch (sig) {
    case SIGCHLD:
        /* wait until no more zombie children */
        while (waitpid(-1, NULL, WNOHANG) > 0);
        break;
    case SIGQUIT:
        flags_ |= FLAG_QUIT;
        break;
    }
}

static
void cleanup() {
    struct client_t *c, *tc;

    for (c = g_listclient; c != NULL; ) {
        tc = c;
        c = c->next;
        client_close(tc);
        free(tc);
    }
    for (c = poolclient_; c != NULL; ) {
        tc = c;
        c = c->next;
        free(tc);
    }
}

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
        if (sigaction(SIGCHLD, &sa, NULL) < 0
                || sigaction(SIGQUIT, &sa, NULL) < 0) {
            A_ERR("sigaction %s", strerror(errno));
            return 1;
        }
    }
    g_server.port = PORT;
    if (server_init(&g_server) != 0) {
        return 1;
    }
    /* main loop */
    while (!(flags_ & FLAG_QUIT)) {
        server_poll(&g_server);
    }
    cleanup();
    return 0;
}

/* vim: set ts=4 sw=4 expandtab: */
