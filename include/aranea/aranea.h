/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_H_
#define ARANEA_H_

#include <aranea/config.h>
#include <aranea/types.h>
#include <aranea/server.h>
#include <aranea/client.h>
#include <aranea/http.h>
#include <aranea/mimetype.h>
#include <aranea/cgi.h>
#include <aranea/auth.h>
#include <aranea/util.h>

#define A_QUOTE(x)              #x
#define A_TOSTR(x)              A_QUOTE(x)
#define A_SRC                   __FILE__ ":" A_TOSTR(__LINE__)
#define A_UNUSED(x)             _ ## x __attribute__((unused))
#define A_SIZEOF(x)             (sizeof(x) / sizeof((x)[0]))
#define A_MAX(x, y)             ((x) > (y) ? (x) : (y))

#ifdef DEBUG
# define A_ERR(fmt, ...)        fprintf(stderr, "*%s\t\t" fmt "\n", A_SRC, __VA_ARGS__)
# define A_LOG(fmt, ...)        fprintf(stdout, "-%s\t\t" fmt "\n", A_SRC, __VA_ARGS__)
#else
# define A_ERR(fmt, ...)        fprintf(stderr, fmt "\n", __VA_ARGS__)
# define A_LOG(fmt, ...)
#endif

/** General purpose buffer, used by CGI
 */
#define GBUFF_LENGTH            A_MAX(MAX_CGIENV_LENGTH, \
                                    A_MAX(MAX_PATH_LENGTH, MAX_REQUEST_LENGTH))

/* aranea.c */

/** Create a new client object. The new one is either from client pool
 * or newly allocated.
 */
struct client_t *alloc_client();

/** Push this client into the pool or delete it if the pool is full.
 */
void detach_client(struct client_t *cli);

/* global variables */
extern time_t g_curtime;
extern struct config_t g_config;
extern char g_buff[GBUFF_LENGTH];
extern struct server_t g_server;

#if HAVE_AUTH == 1
extern struct auth_t *g_auth;
#endif

#endif /* ARANEA_H_ */

/* vim: set ts=4 sw=4 expandtab: */
