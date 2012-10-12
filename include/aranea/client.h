/* Aranea
 * Copyright (c) 2012, Nguyen Quoc Viet
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_CLIENT_H_
#define ARANEA_CLIENT_H_

#include <aranea/types.h>

struct client_t *client_new();
void client_reset(struct client_t *self);
void client_add(struct client_t *self, struct client_t **list);
void client_remove(struct client_t *self);
void client_close(struct client_t *self);
void client_handle_recvheader(struct client_t *self);
void client_handle_sendheader(struct client_t *self);
void client_handle_sendfile(struct client_t *self);

#endif /* ARANEA_CLIENT_H_ */

/* vim: set ts=4 sw=4 expandtab: */
