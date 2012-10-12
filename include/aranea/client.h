/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_CLIENT_H_
#define ARANEA_CLIENT_H_

#include <aranea/types.h>

/** Allocate memory for a client.
 */
struct client_t *client_new();

/** Reset status of the client object to reuse it for another connection.
 */
void client_reset(struct client_t *self);

/** Add this client to the list.
 */
void client_add(struct client_t *self, struct client_t **list);

/** Remove current client from the list which it belongs to.
 */
void client_remove(struct client_t *self);

/** Close connection from the client.
 */
void client_close(struct client_t *self);

/** Called when client starts receiving header
 */
void client_handle_recvheader(struct client_t *self);

/** Called to send header.
 */
void client_handle_sendheader(struct client_t *self);

/** Called to send (static) file.
 */
void client_handle_sendfile(struct client_t *self);

#endif /* ARANEA_CLIENT_H_ */

/* vim: set ts=4 sw=4 expandtab: */
