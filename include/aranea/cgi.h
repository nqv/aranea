/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_CGI_H_
#define ARANEA_CGI_H_

#include <aranea/types.h>

/** Check if the file has CGI extension.
 */
int is_cgi(const char *name, const int len);

/** Generate CGI environment from HTTP request.
 */
int cgi_gen_env(const struct request_t *req, char **env);

#endif /* ARANEA_CGI_H_ */

/* vim: set ts=4 sw=4 expandtab: */
