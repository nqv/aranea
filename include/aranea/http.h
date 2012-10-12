/* Aranea
 * Copyright (c) 2012, Nguyen Quoc Viet
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_HTTP_H_
#define ARANEA_HTTP_H_

#include <aranea/types.h>

int http_parse(struct request_t *self, char *data, int len);
void http_decode_url(char *url);
int http_gen_header(struct response_t *self, char *data, int len,
        const unsigned int flags);
int http_gen_errorpage(struct response_t *self, char *data, int len);
void http_sanitize_url(char *url);
int http_find_headerlength(const char *data, int len);
const char *http_string_status(int code);

#endif /* HTTP_H_ */

/* vim: set ts=4 sw=4 expandtab: */
