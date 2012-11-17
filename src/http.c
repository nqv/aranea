/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <aranea/aranea.h>

static
const char * const REQUEST_HEADER_NAMES[NUM_REQUEST_HEADER] = {
#if HAVE_AUTH == 1
        "Authorization",
#endif
        "Connection",
        "Content-Length",
        "Content-Range",
        "Content-Type",
        "Cookie",
        "If-Modified-Since",
};

static
int hex_to_int(const char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return 0;
}

/**
 * Only get the first range if a list is given
 */
static
void http_parse_range(struct request_t *self, char *val) {
    char *delim;
    if (strncmp(val, "bytes=", 6) != 0) {
        A_ERR("ranges are not in bytes: %s", val);
        return;
    }
    val += 6;
    delim = strchr(val, '-');
    if (delim == NULL) {
        A_ERR("no hyphen: %s", val);
        return;
    }
    if (delim == val) {         /* range_from is not given */
        self->range_from = NULL;
    } else {
        *delim = '\0';
        self->range_from = val;
    }
    self->range_to = delim + 1;
    delim = strchr(self->range_to, ',');
    if (delim != NULL) {
        *delim = '\0';          /* only the first range in the list */
    }
}

static
void http_save_header(struct request_t *self, char *key, char *val) {
    int i;

    for (i = 0; i < NUM_REQUEST_HEADER; ++i) {
        if (strcasecmp(key, REQUEST_HEADER_NAMES[i]) == 0) {
            self->header[i] = val;
            break;
        }
    }
    if (i == HEADER_CONTENTRANGE) {
        http_parse_range(self, val);
    }
}

static
char *http_parse_querystring(char *url) {
    char *p;

    p = strchr(url, '?');
    if (p == NULL) {
        return NULL;
    }
    *p = '\0';
    return p + 1;
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
    if (*data != '/') {
        A_ERR("url does not start with / %.*s", len, data);
        return NULL;
    }
    end = memchr(data + 1, ' ', len);       /* already know the first char */
    if (end == NULL) {
        A_ERR("no url %d: %.*s", len, len, data);
        return NULL;
    }
    *end = '\0';
    self->url = data;
    /* query string in url */
    self->query_string = http_parse_querystring(self->url);
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

static
int http_put_headerdate(struct response_t *self, char *data, int sz) {
    struct tm *tm;
    (void)self;

    tm = gmtime(&g_curtime);
    return strftime(data, sz, "Date: " DATE_FORMAT "\r\n", tm);
}

/** Insert version, server and date
 */
static
int http_put_headerstatus(struct response_t *self, char *data, int sz) {
    return snprintf(data, sz,
            HTTP_VERSION " %d %s\r\n"
            "Server: " SERVER_ID "\r\n",
            self->status_code, http_string_status(self->status_code));
}

/** Insert Accept-...
 */
static
int http_put_headeraccept(struct response_t *self, char *data, int sz) {
    (void)self;
    return snprintf(data, sz, "Accept-Ranges: bytes\r\n");
}

/** Insert Content-Type, Content-Length and Last-Modified.
 */
static
int http_put_headercontent(struct response_t *self, char *data, int sz) {
    int len, i;

    len = 0;
    if (self->content_length >= 0) {
        i = snprintf(data, sz, "Content-Length: %ld\r\n",
                self->content_length);
        sz -= i;
        if (sz < 0) {
            return -1;
        }
        len += i;
    }
    if (self->content_type != NULL) {
        i = snprintf(data + len, sz, "Content-Type: %s\r\n",
                self->content_type);
        sz -= i;
        if (sz < 0) {
            return -1;
        }
        len += i;
    }
    if (self->last_mod >= 0) {
        i = strftime(data + len, sz, "Last-Modified: " DATE_FORMAT "\r\n",
                gmtime(&self->last_mod));
        sz -= i;
        if (sz < 0) {
            return -1;
        }
        len += i;
    }
    return len;
}

/** Insert Content-Range.
 */
static
int http_put_headerrange(struct response_t *self, char *data, int sz) {
    off_t to;

    to = self->content_from + self->content_length - 1;
    return snprintf(data, sz, "Content-Range: bytes %ld-%ld/%ld\r\n",
            self->content_from, to, self->total_length);
}

#define HTTP_PUT_HEADER_(flag, callback, len, sz)                   \
    if (flags & flag) {                                             \
        i = callback(self, data + len, sz);                         \
        sz -= i;                                                    \
        if (sz < 0) {                                               \
            return -1;                                              \
        }                                                           \
        len += i;                                                   \
    }

int http_gen_header(struct response_t *self, char *data, int sz,
        const unsigned int flags) {
    int len, i;

    len = http_put_headerstatus(self, data, sz);
    sz -= len;
    if (sz < 0) {
        return -1;
    }
    HTTP_PUT_HEADER_(HTTP_FLAG_DATE, http_put_headerdate, len, sz);
    HTTP_PUT_HEADER_(HTTP_FLAG_ACCEPT, http_put_headeraccept, len, sz);
    HTTP_PUT_HEADER_(HTTP_FLAG_CONTENT, http_put_headercontent, len, sz);
    HTTP_PUT_HEADER_(HTTP_FLAG_RANGE, http_put_headerrange, len, sz);

    if (flags & HTTP_FLAG_END) {
        i = snprintf(data + len, sz, "\r\n");
        if (i < 0) {
            return -1;
        }
        len += i;
    }
    return len;
}

int http_gen_errorpage(struct response_t *self, char *data, int sz) {
    int len, i;

    len = http_put_headerstatus(self, data, sz);
    sz -= len;
    if (sz < 0) {
        return -1;
    }
#if HAVE_AUTH == 1
    if (self->realm != NULL) {
        i = snprintf(data + len, sz,
                "WWW-Authenticate: Basic realm=\"%s\"\r\n", self->realm);
        sz -= i;
        if (sz < 0) {
            return -1;
        }
        len += i;
    }
#endif
    i = snprintf(data + len, sz,
            "Content-Type: text/html\r\n\r\n");
    sz -= i;
    if (sz < 0) {
        return -1;
    }
    len += i;
    i = snprintf(data + len, sz,
            "<html><head><title>%d</title></head><body>"
            "<h1>%d %s</h1><hr />" SERVER_ID
            "</body></html>",
            self->status_code, self->status_code,
            http_string_status(self->status_code));
    return len + i;
}

void http_decode_url(char *url) {
    char *out;
    char c;

    out = url;
    while ('\0' != (c = *url)) {
        if (c == '%' && *(url + 1) != '\0' && *(url + 2) != '\0') {
            *out = (char)(hex_to_int(*(url + 1)) * 16 + hex_to_int(*(url + 2)));
            ++out;
            url += 2;
        } else {
            *out = c;
            ++out;
            ++url;
        }
    }
    *out = '\0';
}

/** Make url safe
 * Do not allow file start with '.'.
 * Replace "/\.+" to "/"
 */
void http_sanitize_url(char *url) {
    char *out;
    char c;

    out = url;
    for (;;) {
        c = *url;
        switch (c) {
        case '/':
            if (out != url) {
                *out = c;
            }
            ++out;
            ++url;
            while ('.' == *url) {
                ++url;
            }
            break;
        case '\0':
            if (out != url) {
                *out = c;
            }
            return;
        default:
            if (out != url) {
                *out = c;
            }
            ++out;
            ++url;
            break;
        }
    }
}

int http_get_realpath(const char *url, char *path) {
    int len;

    /* snprintf is slower but safer than strcat/strncpy */
#if HAVE_CHROOT == 1                    /* already chroot */
    len = snprintf(path, MAX_PATH_LENGTH, "%s", url);
#else
    len = snprintf(path, MAX_PATH_LENGTH, "%s%s", g_config.root, url);
#endif
    /* append default index file */
    if (path[len - 1] == '/') {
        /* url/index.html */
        len += snprintf(path + len, MAX_PATH_LENGTH - len, "%s", WWW_INDEX);
    }
    return len;
}

/** Find the length of request header by looking for the header termination
 * (\r\n\r\n or \n\n)
 */
int http_find_headerlength(const char *data, int len) {
    int sz = 0;
    const char *crlf = data;

    for (;;) {
        crlf = memchr(crlf, '\n', len - sz);
        if (crlf == NULL) {
            break;
        }
        sz = crlf - data;
        if (sz > 0 && sz <= (len - 3) && *(crlf - 1) == '\r'
                && *(crlf + 1) == '\r' && *(crlf + 2) == '\n') {
            /* \r\n\r\n */
            return sz + 3;
        }
        if (sz <= (len - 1) && *(crlf + 1) == '\n') {
            /* \n\n */
            return sz + 2;
        }
        /* continue */
        ++crlf;
        ++sz;
        if (sz >= len) {
            break;
        }
    }
    return -1;
}

const char *http_string_status(int code) {
    if (code < 300) {               /* 2xx */
        switch (code) {
        case HTTP_STATUS_OK:
            return "OK";
        case HTTP_STATUS_PARTIALCONTENT:
            return "Partial Content";
        }
    } else if (code < 400) {        /* 3xx */
        switch (code) {
        case HTTP_STATUS_MOVEDPERMANENTLY:
            return "Moved Permanently";
        case 302:
            return "Moved Temporarily ";
        case 303:
            return "See Other";
        case HTTP_STATUS_NOTMODIFIED:
            return "Not Modified";
        }
    } else if (code < 500) {        /* 4xx */
        switch (code) {
        case HTTP_STATUS_BADREQUEST:
            return "Bad Request";
        case HTTP_STATUS_AUTHORIZATIONREQUIRED:
            return "Authorization Required";
        case HTTP_STATUS_FORBIDDEN:
            return "Forbidden";
        case HTTP_STATUS_NOTFOUND:
            return "Not Found";
        case HTTP_STATUS_ENTITYTOOLARGE:
            return "Request Entity Too Large";
        case HTTP_STATUS_RANGENOTSATISFIABLE:
            return "Requested Range Not Satisfiable";
        }
    } else {
        switch (code) {             /* 5xx */
        case HTTP_STATUS_SERVERERROR:
            return "Server Error";
        case HTTP_STATUS_NOTIMPLEMENTED:
            return "Not Implemented";
        }
    }
    A_ERR("unknown code %d", code);
    return "Unknown";
}

/* vim: set ts=4 sw=4 expandtab: */
