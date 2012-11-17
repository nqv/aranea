/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_STATE_H_
#define ARANEA_STATE_H_

#include <aranea/types.h>

/** Handler of receiving header.
 */
void state_recv_header(struct client_t *client);

/** Handler of sending header.
 */
void state_send_header(struct client_t *client);

/** Handler of sending (static) file.
 */
void state_send_file(struct client_t *client);

#endif  /* ARANEA_STATE_H_ */

/* vim: set ts=4 sw=4 expandtab: */
