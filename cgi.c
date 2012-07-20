/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#include <stdio.h>
#include <string.h>
#include "aranea.h"

#define CGI_EXT_LEN_                ((int)sizeof(CGI_EXT) - 1)
int is_cgi(const char *name, const int len) {
    if (len > CGI_EXT_LEN_) {
        if (memcmp(name + len - CGI_EXT_LEN_, CGI_EXT, CGI_EXT_LEN_) == 0) {
            return 1;
        }
    }
    return 0;
}

#define CGI_ADD_ENV_(env, cnt, buf, ...)                            \
    do {                                                            \
        *env = buf;                                                 \
        len = sizeof(g_cgienv) - (buf - g_cgienv);                  \
        if (len > 0) {                                              \
            len = snprintf(buf, len, __VA_ARGS__);                  \
            buf += len + 1; /* skip NULL */                         \
            ++env;                                                  \
            ++cnt;                                                  \
        }                                                           \
    } while (0)

/**
 * Data are saved in g_cgienv
 */
int cgi_gen_env(const struct request_t *req, char **env) {
    int cnt, len;
    char *buf;

    cnt = 0;
    buf = g_cgienv;
    CGI_ADD_ENV_(env, cnt, buf, "REQUEST_METHOD=%s", req->method);
    CGI_ADD_ENV_(env, cnt, buf, "REQUEST_URI=%s", req->url);
    CGI_ADD_ENV_(env, cnt, buf, "QUERY_STRING=%s",
            (req->query_string) ? req->query_string : "");

    if (req->content_type) {
        CGI_ADD_ENV_(env, cnt, buf, "CONTENT_TYPE=%s", req->content_type);
    }
    if (req->content_length) {
        CGI_ADD_ENV_(env, cnt, buf, "CONTENT_LENGTH=%s", req->content_length);
    }
    *env = NULL;
    return cnt;
}

/* vim: set ts=4 sw=4 expandtab: */
