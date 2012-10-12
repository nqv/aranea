/* Aranea
 * Copyright (c) 2012, Nguyen Quoc Viet
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_SERVER_H_
#define ARANEA_SERVER_H_

#include <aranea/types.h>

int server_init(struct server_t *self);
struct client_t *server_accept(struct server_t *self);
void server_poll(struct server_t *self);
void server_close_fds();

#endif /* ARANEA_SERVER_H_ */

/* vim: set ts=4 sw=4 expandtab: */
