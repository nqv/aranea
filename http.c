/* Aranea
 * Copyright (c) 2011, Nguyen Quoc Viet
 * See LICENSE file for copyright and license details.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "aranea.h"

/**
 * Only get the first range if a list is given
 */
static
void http_parse_range(struct request_t *self, char *val) {
    char *delim;
    if (strncmp(val, "bytes ", 6) != 0) {
        A_ERR("ranges are not in bytes: %s", val);
        return;
    }
    val += 6;
    delim = strchr(val, '-');
    if (delim == NULL) {
        A_ERR("no hyphen: %s", val);
        return;
    }
    if (delim == val) {  /* range_from is not given */
        self->range_from = NULL;
    } else {
        self->range_from = val;
    }
    self->range_to = delim + 1;
}

static
void http_save_header(struct request_t *self, char *key, char *val) {
    if (strcasecmp(key, "Content-Type") == 0) {
        self->content_type = val;
    } else if (strcasecmp(key, "Content-Length") == 0) {
        self->content_length = val;
    } else if (strcasecmp(key, "Connection") == 0) {
        self->connection = val;
    } else if (strcasecmp(key, "Content-Range") == 0) {
        http_parse_range(self, val);
    } else if (strcasecmp(key, "If-Modified-Since") == 0) {
        self->if_mod_since = val;
    } else {
        A_LOG("header not supported: %s %s", key, val);
    }
}


/** Parse the initial request line
 * Also modify data of client (padding NULL)
 * @return position of the last char parsed
 *         NULL on error
 */
static
char *http_parse_request(struct request_t *self, char *data, int len) {
    char *end;

    /* method */
    end = memchr(data, ' ', len);
    if (end == NULL) {
        A_ERR("no method %d: %.*s", len, len, data);
        return NULL;
    }
    *end = '\0';
    self->method = data;
    /* url */
    len -= end - data + 1;
    if (len <= 0) {
        A_ERR("no url %d", len);
        return NULL;
    }
    data = end + 1;
    end = memchr(data, ' ', len);
    if (end == NULL) {
        A_ERR("no url %d: %.*s", len, len, data);
        return NULL;
    }
    *end = '\0';
    self->url = data;
    /* version */
    len -= end - data + 1;
    if (len <= 0) {
        A_ERR("no version %d", len);
        return NULL;
    }
    data = end + 1;
    end = memchr(data, '\n', len);
    if (end == NULL) {
        A_ERR("no version %d: %.*s", len, len, data);
        return NULL;
    }
    if (*(end - 1) == '\r') {           /* \r\n */
        *(end - 1) = '\0';
    } else {
        *end = '\0';
    }
    self->version = data;
    return end + 1;
}

/** Parse each line of the header
 */
static
char *http_parse_header(struct request_t *self, char *data, int len) {
    char *val, *end;

    /* search for "key: value" */
    val = memchr(data, ':', len);
    if (val == NULL) {
        A_ERR("no delimiter %d: %.*s", len, len, data);
        return NULL;
    }
    *val = '\0';
    ++val;
    while (*val == ' ') {           /* skip spaces */
        ++val;
    }
    len -= val - data;
    if (len <= 0) {
        A_ERR("no val %d", len);
        return NULL;
    }
    end = memchr(val, '\n', len);
    if (end == NULL) {
        A_ERR("no line ending %d: %.*s", len, len, val);
        return NULL;
    }
    if (*(end - 1) == '\r') {       /* \r\n */
        *(end - 1) = '\0';
    } else {
        *end = '\0';
    }
    /* check if the header is supported */
    http_save_header(self, data, val);
    return end + 1;
}

int http_parse(struct request_t *self, char *data, int len) {
    char *p;

    /* get request line */
    p = http_parse_request(self, data, len);
    if (p == NULL) {
        return -1;
    }
    len -= p - data;
    data = p;
    /* get other headers until end of the request */
    while ((len > 0) && !((data[0] == '\n')
            || (len >= 2 && data[0] == '\r' && data[1] == '\n'))) {
        p = http_parse_header(self, data, len);
        if (p == NULL) {
            return -1;
        }
        len -= p - data;
        data = p;
    }
    return 0;
}

const char *http_get_status_string(int code) {
    switch (code) {
    case 200:
        return "OK";
    case 301:
        return "Moved Permanently";
    case 302:
        return "Moved Temporarily ";
    case 303:
        return "See Other";
    case 400:
        return "Bad Request";
    case 404:
        return "Not Found";
    case 500:
        return "Server Error";
    case 501:
        return "Not Implemented";
    }
    A_ERR("unknown code %d", code);
    return "Unknown";
}

/* vim: set ts=4 sw=4 expandtab: */
