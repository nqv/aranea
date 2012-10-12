/* Aranea
 * Copyright (c) 2012, Nguyen Quoc Viet
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_CGI_H_
#define ARANEA_CGI_H_

#include <aranea/types.h>

int is_cgi(const char *name, const int len);
int cgi_gen_env(const struct request_t *req, char **env);

#endif /* ARANEA_CGI_H_ */

/* vim: set ts=4 sw=4 expandtab: */
