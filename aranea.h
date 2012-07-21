/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_H_
#define ARANEA_H_

#include <sys/types.h>
#include "config.h"

#define A_QUOTE(x)              #x
#define A_TOSTR(x)              A_QUOTE(x)
#define A_SRC                   __FILE__ ":" A_TOSTR(__LINE__)
#define A_UNUSED(x)             _ ## x __attribute__((unused))
#define A_SIZEOF(x)             (sizeof(x) / sizeof((x)[0]))

#ifdef DEBUG
# define A_ERR(fmt, ...)        fprintf(stderr, "*%s\t\t" fmt "\n", A_SRC, __VA_ARGS__)
# define A_LOG(fmt, ...)        fprintf(stdout, "-%s\t\t" fmt "\n", A_SRC, __VA_ARGS__)
#else
# define A_ERR(fmt, ...)        fprintf(stderr, fmt "\n", __VA_ARGS__)
# define A_LOG(fmt, ...)
#endif

#define MAX_IP_LENGTH           40
#define MAX_DATE_LENGTH         32      /* */
#define DATE_FORMAT             "%a, %d %b %Y %H:%M:%S GMT"

enum {
    /* 2xx */
    HTTP_STATUS_OK              = 200,
    HTTP_STATUS_PARTIALCONTENT  = 206,
    /* 3xx */
    HTTP_STATUS_MOVEDPERMANENTLY = 301,
    HTTP_STATUS_NOTMODIFIED     = 304,
    /* 4xx */
    HTTP_STATUS_BADREQUEST      = 400,
    HTTP_STATUS_FORBIDDEN       = 403,
    HTTP_STATUS_NOTFOUND        = 404,
    HTTP_STATUS_ENTITYTOOLARGE  = 413,
    HTTP_STATUS_RANGENOTSATISFIABLE = 416,
    /* 5xx */
    HTTP_STATUS_SERVERERROR     = 500,
    HTTP_STATUS_NOTIMPLEMENTED  = 501,
};

enum {
    STATE_NONE                  = 0,
    STATE_RECV_HEADER,          /* read request from socket */
    STATE_SEND_HEADER,          /* write response to socket */
    STATE_SEND_FILE,            /* write file to socket */
};

enum {
    FLAG_QUIT                   = 1 << 0,
};

enum {
    HTTP_FLAG_END               = 1 << 0,
    HTTP_FLAG_CONTENT           = 1 << 1,
    HTTP_FLAG_RANGE             = 1 << 2,
};

enum {
    CLIENT_FLAG_HEADERONLY      = 1 << 0,
    CLIENT_FLAG_POST            = 1 << 1,
};

/**
 * Pointed to request buffer
 */
struct request_t {
    char *method;               /* required */
    char *url;                  /* required */
    char *query_string;
    char *version;              /* required */
    char *connection;
    char *content_type;
    char *content_length;
    char *range_from;
    char *range_to;
    char *if_mod_since;
    char *cookie;
    ssize_t header_length;
};

struct response_t {
    int status_code;
    const char *content_type;
    off_t total_length;
    off_t content_length;
    off_t content_from;
    time_t last_mod;
};

struct mimetype_t {
    const char *ext;
    const char *type;
};

struct config_t {
	const char *root;
};

struct client_t {
    int remote_fd;      /**< Socket descriptor */
    int local_rfd;      /**< Reading file/pipe descriptor */
    time_t timeout;
    int state;
    char ip[MAX_IP_LENGTH];

    struct request_t request;
    char data[MAX_REQUEST_LENGTH];
    ssize_t data_length;
    ssize_t data_sent;

    struct response_t response;
    off_t file_sent;

    unsigned int flags;
    struct client_t *next;
    struct client_t **prev;
};

struct server_t {
    int fd;
    int port;
    struct client_t *clients;
};

/* server.c */
int server_init(struct server_t *self);
struct client_t *server_accept(struct server_t *self);
void server_poll(struct server_t *self);
void server_close_fds();

/* client.c */
struct client_t *client_new();
void client_reset(struct client_t *self);
void client_add(struct client_t *self, struct client_t **list);
void client_remove(struct client_t *self);
void client_close(struct client_t *self);
void client_handle_recvheader(struct client_t *self);
void client_handle_sendheader(struct client_t *self);
void client_handle_sendfile(struct client_t *self);

/* http.c */
int http_parse(struct request_t *self, char *data, int len);
void http_decode_url(char *url);
int http_gen_header(struct response_t *self, char *data, int len,
        const unsigned int flags);
int http_gen_errorpage(struct response_t *self, char *data, int len);
void http_sanitize_url(char *url);
int http_find_headerlength(const char *data, int len);
const char *http_string_status(int code);

/* mimetype.c */
const char *mimetype_get(const char *filename);

/* cgi.c */
int is_cgi(const char *name, const int len);
int cgi_gen_env(const struct request_t *req, char **env);

struct client_t *alloc_client();
void detach_client(struct client_t *cli);

/* global variables */
extern time_t g_curtime;
extern struct config_t g_config;
extern char g_cgienv[MAX_CGIENV_LENGTH];
extern struct client_t *g_listclient;
extern struct server_t g_server;

#endif /* ARANEA_H_ */

/* vim: set ts=4 sw=4 expandtab: */
