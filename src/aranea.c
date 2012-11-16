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

#include <aranea/aranea.h>

/* global vars */
/** Current time: used in HTTP date and server timeout checking */
time_t g_curtime;
/** System settings */
struct config_t g_config;
/** General purpose buffer */
char g_buff[GBUFF_LENGTH];
/** Server socket */
struct server_t g_server;
/* Authentication */
#if HAVE_AUTH == 1
struct auth_t *g_auth = NULL;
#endif


static unsigned int flags_ = 0;

static
void print_help(const char *name) {
    printf("Usage: %s [ARGUMENTS]\n", name);

    printf("Arguments:\n%s",
            "  -d                   Run as daemon (background) mode\n"
            "  -r DOCUMENT_ROOT     Server root (absolute path)\n"
            "  -p PORT              Server listening port\n"
#if HAVE_AUTH == 1
            "  -f AUTH_FILE         Authentication file\n"
#endif
            );

    printf("Version: %s (AUTH=%d CGI=%d CHROOT=%d VFORK=%d)\n",
            ARANEA_VERSION, HAVE_AUTH, HAVE_CGI, HAVE_CHROOT, HAVE_VFORK);

    exit(0);
}

static
void parse_options(int argc, char **argv) {
    int i;
    char sw;    /* current switch */

    /* default settings */
    g_server.port = PORT;
    g_config.root = ".";                /* current dir */

    if (argc > 1) {         /* at least one argument */
        sw = '\0';
        for (i = 1; i < argc; ++i) {
            if (argv[i][0] == '-') {
                sw = argv[i][1];
                /* switches that have no additional argument */
                switch (sw) {
                case 'h':
                    print_help(argv[0]);
                    sw = '\0';
                    break;
                case 'd':
                    flags_ |= FLAG_DAEMON;
                    sw = '\0';
                    break;
                }
            } else {
                switch (sw) {
                case 'r':
                    g_config.root = argv[i];
                    break;
                case 'p':
                    g_server.port = atoi(argv[i]);
                    break;
#if HAVE_AUTH == 1
                case 'f':
                    g_config.auth_file = argv[i];
                    break;
#endif
                }
                sw = '\0';
            }
        }
    }
}

/** Initialize from user configurations
 */
static
int init_conf() {
    /* Parse authentication file */
#if HAVE_AUTH == 1
    if (g_config.auth_file) {
        if (auth_parsefile(g_config.auth_file) != 0) {
            return -1;
        }
    }
#endif

    /* Change root to www directory */
#if HAVE_CHROOT == 1
    if (chroot(g_config.root) != 0) {
        A_ERR("Could not chroot to %s", g_config.root);
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

#if HAVE_AUTH == 1
    auth_cleanup();
#endif

    for (c = g_server.clients; c != NULL; ) {
        tc = c;
        c = c->next;
        client_close(tc);
        free(tc);
    }
    clientpool_cleanup();
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
    parse_options(argc, argv);

    if (flags_ & FLAG_DAEMON) {
        daemonize(argv);
    }
    /* User configuration */
    if (init_conf() != 0) {
        return 1;
    }
    /* initialize signal handlers */
    if (init_signal() != 0) {
        return 1;
    }
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
