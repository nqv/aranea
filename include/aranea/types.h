/* Aranea
 * Copyright (c) 2012, Nguyen Quoc Viet
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

#endif /* ARANEA_TYPES_H_ */

/* vim: set ts=4 sw=4 expandtab: */
