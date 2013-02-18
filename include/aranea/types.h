/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_TYPES_H_
#define ARANEA_TYPES_H_

#include <sys/types.h>

#include <aranea/config.h>

enum {
    /* 2xx */
    HTTP_STATUS_OK              = 200,
    HTTP_STATUS_PARTIALCONTENT  = 206,
    /* 3xx */
    HTTP_STATUS_MOVEDPERMANENTLY = 301,
    HTTP_STATUS_NOTMODIFIED     = 304,
    /* 4xx */
    HTTP_STATUS_BADREQUEST      = 400,
    HTTP_STATUS_AUTHORIZATIONREQUIRED = 401,
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
    FLAG_DAEMON                 = 1 << 1,
};

/* HTTP headers */
enum {
    HTTP_FLAG_END               = 1 << 0,   /* Termination */
    HTTP_FLAG_ACCEPT            = 1 << 1,   /* Supported features from server */
    HTTP_FLAG_CONTENT           = 1 << 2,   /* Content type/length */
    HTTP_FLAG_RANGE             = 1 << 3,   /* Content range */
    HTTP_FLAG_DATE              = 1 << 4,   /* Server date */
};

enum {
    CLIENT_FLAG_HEADERONLY      = 1 << 0,
    CLIENT_FLAG_POST            = 1 << 1,
    CLIENT_FLAG_KEEPALIVE       = 1 << 2,   /* Do not close connection */
};

/** HTTP request headers.
 * This should match the array name in http.c
 */
enum {
#if HAVE_AUTH == 1
    HEADER_AUTHORIZATION,       /* Authorization */
#endif
    HEADER_CONNECTION,          /* Connection */
    HEADER_CONTENTLENGTH,       /* Content-Length */
    HEADER_CONTENTRANGE,        /* Content-Range */
    HEADER_CONTENTTYPE,         /* Content-Type */
    HEADER_COOKIE,              /* Cookie */
    HEADER_IFMODIFIEDSINCE,     /* If-Modified-Since */
    NUM_REQUEST_HEADER,
};

/**
 * Pointed to request buffer
 */
struct request_t {
    char *header[NUM_REQUEST_HEADER];

    char *method;               /* required */
    char *url;                  /* required */
    char *query_string;
    char *version;              /* required */
    char *range_from;
    char *range_to;
    ssize_t header_length;
};

struct response_t {
    int status_code;
    const char *content_type;
    off_t total_length;
    off_t content_length;
    off_t content_from;
    time_t last_mod;
#if HAVE_AUTH == 1
    const char *realm;
#endif
};

struct mimetype_t {
    const char *ext;
    const char *type;
};

struct auth_t {
    char path[MAX_AUTHPATH_LENGTH];
    int path_length;    /**< To not have to calculate path length again */
    char user[MAX_AUTHUSER_LENGTH];
    char pass[MAX_AUTHPASS_LENGTH];
    struct auth_t *next;
};

struct config_t {
    const char *root;
#if HAVE_AUTH == 1
    const char *auth_file;
#endif
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

#endif /* ARANEA_TYPES_H_ */

/* vim: set ts=4 sw=4 expandtab: */
