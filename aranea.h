/* Aranea
 * Copyright (c) 2011, Nguyen Quoc Viet
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_H_
#define ARANEA_H_

#define A_QUOTE(x)              #x
#define A_TOSTR(x)              A_QUOTE(x)
#define A_SRC                   __FILE__ ":" A_TOSTR(__LINE__)
#define A_INLINE                inline
#define A_UNUSED(x)             _ ## x __attribute__((unused))
#define A_SIZEOF(x)             (sizeof(x) / sizeof((x)[0]))
#define A_FOREACH(head, var)    \
    for ((var) = (head); (var) != NULL; (var) = (var)->next)
#define A_FOREACH_S(head, var, tmp)   \
    for ((var) = (head); ((var) != NULL) && ((tmp = (var)->next), 1); (var) = (tmp))


#ifdef DEBUG
# define A_ERR(fmt, ...)        fprintf(stderr, "*%s\t\t" fmt "\n", A_SRC, __VA_ARGS__)
# define A_LOG(fmt, ...)        fprintf(stdout, "-%s\t\t" fmt "\n", A_SRC, __VA_ARGS__)
#else
# define A_ERR(fmt, ...)        fprintf(stderr, fmt "\n", __VA_ARGS__)
# define A_LOG(fmt, ...)
#endif

#define MAX_REQUEST_LENGTH      2048
#define MAX_IP_LENGTH           40
#define BACKLOG                 10

enum {
    STATE_NONE                  = 0,
    STATE_RECV_HEAD,
    STATE_SEND_HEAD,
    STATE_SEND_CONTENT,
};

enum {
    METHOD_GET                  = 0,
    METHOD_HEAD,
    METHOD_POST,
};

enum {
    FLAG_QUIT                   = 1 << 0,
};

struct request_t {
    const char *method;
    const char *url;
    const char *version;
    const char *connection;
    const char *content_type;
    const char *content_length;
    const char *range_from;
    const char *range_to;
    const char *if_mod_since;
};

struct response_t {
    int status_code;
};

struct client_t {
    int remote_fd;      /**< Socket descriptor */
    int local_fd;       /**< File descriptor */
    time_t timeout;
    int state;
    int method;
    char ip[MAX_IP_LENGTH];
    char data[MAX_REQUEST_LENGTH];
    int data_length;

    struct request_t request;
    struct response_t response;

    struct client_t *next;
    struct client_t **prev;
};

struct server_t {
    int fd;
    int port;
    struct client_t *clients;
};

/* server.c */
int server_init(struct server_t *self);
struct client_t *server_accept(struct server_t *self);

/* client.c */
struct client_t *client_new();
void client_destroy(struct client_t *self);
void client_add(struct client_t *self, struct client_t **list);
void client_remove(struct client_t *self);
void client_close(struct client_t *self);
void client_handle_recvhead(struct client_t *self);
void client_handle_sendhead(struct client_t *self);
void client_handle_recvfile(struct client_t *self);
void client_handle_sendfile(struct client_t *self);

/* http.c */
int http_parse(struct request_t *self, char *data, int len);
const char *http_get_status_string(int code);

#endif /* ARANEA_H_ */

/* vim: set ts=4 sw=4: */
