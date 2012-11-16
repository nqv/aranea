/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_CLIENTPOOL_H_
#define ARANEA_CLIENTPOOL_H_

#include <aranea/types.h>

/** Create a new client object. The new one is either from client pool
 * or newly allocated.
 */
struct client_t *clientpool_alloc();

/** Push this client into the pool or delete it if the pool is full.
 */
void clientpool_free(struct client_t *cli);

void clientpool_cleanup();

#endif 	/* ARANEA_CLIENTPOOL_H_ */

/* vim: set ts=4 sw=4 expandtab: */
