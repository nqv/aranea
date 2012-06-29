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
#undef CGI_EXT_LEN_

#define CGI_ADD_ENV_(env, cnt, buf, ...)                            \
    do {                                                            \
        int len;                                                    \
        *env = buf;                                                 \
        len = snprintf(buf, sizeof(g_cgienv) - (buf - g_cgienv),    \
                __VA_ARGS__);                                       \
        buf += len;                                                 \
        ++env;                                                      \
        ++cnt;                                                      \
    } while (0)

/**
 * Data are saved in g_cgienv
 */
int cgi_gen_env(const struct request_t *req, char **env) {
    int cnt;
    char *buf;

    cnt = 0;
    buf = g_cgienv;
    CGI_ADD_ENV_(env, cnt, buf, "%s=%s", "REQUEST_METHOD", req->method);
    CGI_ADD_ENV_(env, cnt, buf, "%s=%s", "REQUEST_URI", req->url);
    if (req->query_string) {
        CGI_ADD_ENV_(env, cnt, buf, "%s=%s", "QUERY_STRING", req->query_string);
    } else {
        CGI_ADD_ENV_(env, cnt, buf, "%s=", "QUERY_STRING");
    }
    if (req->content_type) {
        CGI_ADD_ENV_(env, cnt, buf, "%s=%s", "CONTENT_TYPE", req->content_type);
    }
    if (req->content_length) {
        CGI_ADD_ENV_(env, cnt, buf, "%s=%s", "CONTENT_LENGTH", req->content_length);
    }
    return cnt;
}
#undef CGI_ADD_ENV_

/* vim: set ts=4 sw=4 expandtab: */
