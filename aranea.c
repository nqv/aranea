/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
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
int parse_arguments(int argc, char **argv) {
    if (argc < 2) {
        g_config.root = ".";                /* current dir */
    } else {
        g_config.root = argv[1];
    }
#if 0
    if (chdir(g_config.root) != 0) {
        return -1;
    }
#endif
    return 0;
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
int init_signal() {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = &handle_signal;

    if (sigaction(SIGCHLD, &sa, NULL) < 0
            || sigaction(SIGQUIT, &sa, NULL) < 0) {
        A_ERR("sigaction %s", strerror(errno));
        return -1;
    }
    return 0;
}

static
void cleanup() {
    struct client_t *c, *tc;

    for (c = g_server.clients; c != NULL; ) {
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

static
void daemonize(char **argv) {
    pid_t pid;

    if (getppid() == 1) {       /* already a daemon */
        return;
    }
#if HAVE_VFORK == 1             /* uClinux */
    pid = vfork();
    if (pid < 0) {
        A_ERR("fork error %d", pid);
        _exit(1);               /* fork error */
    }
    if (pid > 0) {
        _exit(0);               /* exit the original process */
    }
    /* fork again in the child process */
    pid = vfork();
    if (pid < 0) {
        A_ERR("fork error %d", pid);
        _exit(1);               /* fork error */
    }
    if (pid > 0) {
        _exit(0);               /* exit the child process from the 1st fork */
    }
    /* child of child process */
    if (setsid() < 0) {         /* new session */
        A_ERR("setsid error %d", pid);
        _exit(1);
    }
    /* the parent is now init process, run the program again */
    execve(argv[0], argv, NULL);
    _exit(1);
#else                           /* normal fork */
    (void)argv;                 /* unused */

    pid = fork();               /* fork off the parent process */
    if (pid < 0) {
        A_ERR("fork error %d", pid);
        exit(1);                /* fork error */
    }
    if (pid > 0) {
        exit(0);                /* exit the parent process */
    }
    /* child process */
    if (setsid() < 0) {         /* new session for the child process */
        A_ERR("setsid error %d", pid);
        exit(1);
    }
#endif  /* HAVE_VFORK */
}

int main(int argc, char **argv) {
    /* parse command line arguments */
    if (parse_arguments(argc, argv) != 0) {
        return 1;
    }
    if (flags_ & FLAG_DAEMON) {
        daemonize(argv);
    }
    /* initialize signal handlers */
    if (init_signal() != 0) {
        return 1;
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
