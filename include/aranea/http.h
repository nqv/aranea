/* Aranea
 * Copyright (c) 2012, Nguyen Quoc Viet
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_HTTP_H_
#define ARANEA_HTTP_H_

#include <aranea/types.h>

/** Parse HTTP headers.
 */
int http_parse(struct request_t *self, char *data, int len);

/** Get the path from url in the request.
 */
void http_decode_url(char *url);

/** Generate HTTP headers for response.
 */
int http_gen_header(struct response_t *self, char *data, int len,
        const unsigned int flags);

/** Generate HTTP content for error page.
 */
int http_gen_errorpage(struct response_t *self, char *data, int len);

/** Make url safe by removing relative path.
 */
void http_sanitize_url(char *url);

/** Find the length of request header.
 */
int http_find_headerlength(const char *data, int len);

/** Get the status message from HTTP code.
 */
const char *http_string_status(int code);

#endif /* ARANEA_HTTP_H_ */

/* vim: set ts=4 sw=4 expandtab: */
