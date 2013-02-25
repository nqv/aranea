/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <aranea/aranea.h>

#define CGI_EXT_LEN_                ((int)sizeof(CGI_EXT) - 1)
/** Buffer for CGI environment variables */
#define CGI_BUFF                    g_buff

int cgi_hit(const char *name, const int len) {
    if (len > CGI_EXT_LEN_) {
        if (memcmp(name + len - CGI_EXT_LEN_, CGI_EXT, CGI_EXT_LEN_) == 0) {
            return 1;
        }
    }
    return 0;
}

/** Check if file is executable.
 * HTTP error code is set to client->response.status_code.
 */
static
int cgi_is_executable(const char *path, struct client_t *client) {
    struct stat st;

    if (access(path, X_OK) != 0) {
        client->response.status_code = HTTP_STATUS_FORBIDDEN;
        return -1;
    }
    if (stat(path, &st) == -1) {
        A_ERR("stat: %s", strerror(errno));
        client->response.status_code = HTTP_STATUS_SERVERERROR;
        return -1;
    }
    if (S_ISDIR(st.st_mode)) {
        client->response.status_code = HTTP_STATUS_FORBIDDEN;
        return -1;
    }
    return 0;
}

#define CGI_ADD_ENV_(env, cnt, buf, ...)                            \
    do {                                                            \
        *env = buf;                                                 \
        len = sizeof(CGI_BUFF) - (buf - CGI_BUFF);                  \
        if (len > 0) {                                              \
            len = snprintf(buf, len, __VA_ARGS__);                  \
            buf += len + 1; /* skip NULL */                         \
            ++env;                                                  \
            ++cnt;                                                  \
        }                                                           \
    } while (0)

/** Generate CGI environment from HTTP request.
 * Values are saved in g_buff (g_cgienv)
 */
static
int cgi_gen_env(const struct request_t *req, char **env) {
    int cnt, len;
    char *buf;

    cnt = 0;
    buf = CGI_BUFF;

#ifdef CGI_DOCUMENT_ROOT
    CGI_ADD_ENV_(env, cnt, buf, "DOCUMENT_ROOT=%s", g_config.root);
#endif
#ifdef CGI_REQUEST_METHOD
    CGI_ADD_ENV_(env, cnt, buf, "REQUEST_METHOD=%s", req->method);
#endif
#ifdef CGI_REQUEST_URI
    CGI_ADD_ENV_(env, cnt, buf, "REQUEST_URI=%s", req->url);
#endif
    if (req->query_string) {
        CGI_ADD_ENV_(env, cnt, buf, "QUERY_STRING=%s", req->query_string);
    }
    if (req->header[HEADER_CONTENTTYPE]) {
        CGI_ADD_ENV_(env, cnt, buf, "CONTENT_TYPE=%s", req->header[HEADER_CONTENTTYPE]);
    }
    if (req->header[HEADER_CONTENTLENGTH]) {
        CGI_ADD_ENV_(env, cnt, buf, "CONTENT_LENGTH=%s", req->header[HEADER_CONTENTLENGTH]);
    }
#ifdef CGI_HTTP_COOKIE
    if (req->header[HEADER_COOKIE]) {
        CGI_ADD_ENV_(env, cnt, buf, "HTTP_COOKIE=%s", req->header[HEADER_COOKIE]);
    }
#endif
    *env = NULL;
    return cnt;
}

#if HAVE_VFORK == 1
# define FORK_()        vfork()
# define EXIT_(x)       _exit(x)
#else
# define FORK_()        fork()
# define EXIT_(x)       exit(x)
#endif  /* HAVE_VFORK */

/** Execute file.
 * HTTP error code is set to client->response.status_code.
 */
static
int cgi_exec(const char *path, struct client_t *client) {
    char *argv[2];
    char *envp[MAX_CGIENV_ITEM];
    pid_t pid;
    int newio;

    /* set socket back to blocking */
    newio = fcntl(client->remote_fd, F_GETFL, NULL);
    if (newio == -1
            || fcntl(client->remote_fd, F_SETFL, newio & (~O_NONBLOCK)) == -1) {
        A_ERR("fcntl: F_SETFL O_NONBLOCK %s", strerror(errno));
        client->response.status_code = HTTP_STATUS_SERVERERROR;
        return -1;
    }
    pid = FORK_();
    if (pid < 0) {
        client->response.status_code = HTTP_STATUS_SERVERERROR;
        return -1;
    }
    if (pid == 0) {                             /* child */
        /* Generate CGI parameters before touching to the buffer */
        cgi_gen_env(&client->request, envp);

        /* Send minimal header */
        client->response.status_code = HTTP_STATUS_OK;
        client->data_length = http_gen_header(&client->response, client->data,
                sizeof(client->data), 0);
        if (send(client->remote_fd, client->data, client->data_length, 0) < 0) {
            EXIT_(1);
        }
        /* Tie CGI's stdin to the socket */
        if (client->flags & CLIENT_FLAG_POST) {
            if (dup2(client->remote_fd, STDIN_FILENO) < 0) {
                EXIT_(1);
            }
        }
        /* Tie CGI's stdout to the socket */
        if (dup2(client->remote_fd, STDOUT_FILENO) < 0) {
            EXIT_(1);
        }
        /* close unused FDs */
        server_close_fds();
        /* No error log */
        newio = open("/dev/null", O_WRONLY);
        if (newio != STDERR_FILENO) {
            dup2(newio, STDERR_FILENO);
            close(newio);
        }
        /* Execute cgi script */
        argv[0] = (char *)path;
        argv[1] = NULL;
        execve(path, argv, envp);
        EXIT_(1);                               /* exec error */
    }
    /* parent */
    client->state = STATE_NONE;                   /* Remove this client */
    return 0;
}

int cgi_process(struct client_t *client, const char *path) {
    if (cgi_is_executable(path, client) != 0) {
        return -1;
    }
    if (client->flags & CLIENT_FLAG_HEADERONLY) {
        client->response.status_code = HTTP_STATUS_OK;
        client->data_length = http_gen_header(&client->response, client->data,
                sizeof(client->data), HTTP_FLAG_END);
        client->state = STATE_SEND_HEADER;
        return 0;
    }
    return cgi_exec(path, client);
}

/* vim: set ts=4 sw=4 expandtab: */
