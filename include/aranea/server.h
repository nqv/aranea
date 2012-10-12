/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_SERVER_H_
#define ARANEA_SERVER_H_

#include <aranea/types.h>

/** Initialize server listening socket.
 */
int server_init(struct server_t *self);

/** Accept new connection, return the client object to handle it.
 */
struct client_t *server_accept(struct server_t *self);

/** Listen and handle connections.
 */
void server_poll(struct server_t *self);

/** Close all opening FDs.
 */
void server_close_fds();

#endif /* ARANEA_SERVER_H_ */

/* vim: set ts=4 sw=4 expandtab: */
