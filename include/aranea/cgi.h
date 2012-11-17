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

/** Check if file is executable.
 * HTTP error code is set to client->response.status_code.
 */
int cgi_is_executable(const char *path, struct client_t *client);

/** Generate CGI environment from HTTP request.
 * Values are saved in g_buff
 */
int cgi_gen_env(const struct request_t *req, char **env);

/** Execute file.
 * HTTP error code is set to client->response.status_code.
 */
int cgi_exec(const char *path, struct client_t *client);

#endif /* ARANEA_CGI_H_ */

/* vim: set ts=4 sw=4 expandtab: */
