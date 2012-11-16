/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#include <stdlib.h>
#include <string.h>
#include <aranea/aranea.h>

/* save allocated clients to reduce number of malloc call */
static struct client_t *poolclient_ = NULL;
static int poolclient_len_ = 0;

/** Get client from pool if possible
 */
struct client_t *clientpool_alloc() {
    struct client_t *cli;

    if (poolclient_ != NULL) {
        cli = poolclient_;
        client_remove(cli);
        --poolclient_len_;
    } else {
        cli = malloc(sizeof(struct client_t));
    }
    return cli;
}

void clientpool_free(struct client_t *cli) {
    if (poolclient_len_ > NUM_CACHED_CONN) {
        free(cli);                          /* simply discard it */
    } else {
        client_add(cli, &poolclient_);
        ++poolclient_len_;
    }
}

void clientpool_cleanup() {
	struct client_t *c, *tc;

    for (c = poolclient_; c != NULL; ) {
        tc = c;
        c = c->next;
        free(tc);
    }
}

/* vim: set ts=4 sw=4 expandtab: */
