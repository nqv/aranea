/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_AUTH_H_
#define ARANEA_AUTH_H_

#include <aranea/types.h>

/** Parse authentication file. Format for each record (line):
 * PATH:USERNAME:PASSWORD
 */
int auth_parsefile(const char *path);

/** Note: this modify g_buff
 */
int auth_process(struct client_t *client);

void auth_cleanup();

#endif /* ARANEA_AUTH_H_ */

/* vim: set ts=4 sw=4 expandtab: */
