/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_CGI_H_
#define ARANEA_CGI_H_

#include <aranea/types.h>

/** Check if the file has CGI extension.
 */
int cgi_hit(const char *name, const int len);

/** Execute file.
 * HTTP error code is set to client->response.status_code.
 */
int cgi_process(struct client_t *client, const char *path);

#endif /* ARANEA_CGI_H_ */

/* vim: set ts=4 sw=4 expandtab: */
