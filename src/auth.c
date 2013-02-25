/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <aranea/aranea.h>

/* Authentication */
static struct auth_t *auth_ = NULL;

/* Just use g_buff for the buffer */
#define AUTH_BUF_               g_buff

static
int b64_decode(const char *src, char *dest, int sz) {
    int i;
    int idx;
    unsigned int c;
    unsigned int pattern;

    i = 0;
    idx = 0;
    pattern = 0;

    while (('\0' != (c = *src)) && (sz > 3)) {
        src++;
        /* Look up for index value */
        if ((c >= 'A') && (c <= 'Z')) {
            c = c - 'A' + 0;
        } else if ((c >= 'a') && (c <= 'z')) {
            c = c - 'a' + 26;
        } else if ((c >= '0') && (c <= '9')) {
            c = c - '0' + 52;
        } else if (c == '+') {
            c = 62;
        } else if (c == '/') {
            c = 63;
        } else if (c == '=') {
            c = 0;
        } else {
            /* Skip this */
            continue;
        }
        /* Build the pattern */
        pattern = (pattern << 6) | c;

        if (i == 3) {
            dest[idx++] = (char) (pattern >> 16);
            dest[idx++] = (char) (pattern >> 8);
            dest[idx++] = (char) (pattern);
            pattern = 0;
            i = 0;
            sz -= 3;
        } else {
            i++;
        }
    }
    dest[idx] = '\0';
    return idx;
}

/** Copy string from source to the delimiter position
 */
static
int auth_copydelim(const char **src, char delim, char *dest, int sz) {
    const char *pos;    /* delimiter position */
    int len;

    pos = strchr(*src, delim);
    if (pos == NULL) {
        return -1;
    }
    len = pos - *src;
    if (len == 0) {
        dest[0] = '\0';
    } else if (len > 0) {
        if (len > (sz - 1)) {
            len = (sz - 1);
        }
        memcpy(dest, *src, len);
        dest[len] = '\0';
        /* Change source position */
        *src = pos + 1;
    }
    return len;
}

static
int auth_parse(struct auth_t *self, const char *line) {
    /* Path */
    self->path_length = auth_copydelim(&line, ':', self->path, sizeof(self->path));
    if (self->path_length <= 0) {
        return -1;
    }
    /* User */
    if (auth_copydelim(&line, ':', self->user, sizeof(self->user)) <= 0) {
        return -1;
    }
    /* Password */
    if (auth_copydelim(&line, '\n', self->pass, sizeof(self->pass)) <= 0) {
        return -1;
    }
    return 0;
}

static
void auth_add(struct auth_t *self) {
    /* Always push into the first place, so that the last record has the
     * highest priority */
    self->next = auth_;
    auth_ = self;
    A_LOG("Add auth path=%s, user=%s", self->path, self->user);
}

/** Search if the path is covered by auth */
static
struct auth_t *auth_find(const char *path) {
    struct auth_t *auth;
    int len;    /* url length */

    len = strlen(path);

    for (auth = auth_; auth != NULL; auth = auth->next) {
        if ((len > auth->path_length)
                && (strncmp(path, auth->path, auth->path_length) == 0)) {
            break;
        }
    }
    return auth;
}

static
int auth_verify(struct auth_t *self, const char *credential) {
    int len;
    char *user, *pass;

    len = b64_decode(credential, g_buff, sizeof(g_buff));
    A_LOG("verify: %s -> %s", credential, g_buff);
    if (len > 0) {
        user = g_buff;
        pass = strchr(g_buff, ':');
        if (pass != NULL) {
            *pass = '\0';
            ++pass;
            if ((strcmp(user, self->user) == 0)
                    && strcmp(pass, self->pass) == 0) {
                return 0;
            }
        }
    }
    return -1;
}

int auth_parsefile(const char *path) {
    FILE *f;
    struct auth_t *auth;

    f = fopen(path, "r");
    if (f == NULL) {
        A_ERR("Could not open file %s", path);
        return -1;
    }
    /* Read line by line */
    while (fgets(AUTH_BUF_, sizeof(AUTH_BUF_), f) != NULL) {
        /* Skip comment or empty lines */
        if (AUTH_BUF_[0] == '#' || AUTH_BUF_[0] == '\n') {
            continue;
        }
        auth = malloc(sizeof(struct auth_t));
        if (auth == NULL) {
            fclose(f);
            A_ERR("Out of memory: %s", "auth_t");
            return -1;
        }
        if (auth_parse(auth, AUTH_BUF_) != 0) {
            fclose(f);
            A_ERR("Invalid authentication file format: %s", AUTH_BUF_);
            return -1;
        }
        auth_add(auth);
    }
    fclose(f);
    return 0;
}

/** HTTP header: "Authorization: Basic " + base64("user:pass")
 * Return 0 if authorization is not required
 *       -1 if it is required
 */
static
int auth_check(struct request_t *request) {
    struct auth_t *auth;

    auth = auth_find(request->url);
    if (auth != NULL) {
        /* Need to check user/pass */
        if (request->header[HEADER_AUTHORIZATION] == NULL
                || strncasecmp(request->header[HEADER_AUTHORIZATION],
                        "Basic ", 6) != 0) {
            return -1;
        }
        return auth_verify(auth, request->header[HEADER_AUTHORIZATION] + 6);
    }
    return 0;
}

int auth_process(struct client_t *client) {
    if (auth_ == NULL) {
        return 0;
    }
    if (auth_check(&client->request) != 0) {
        client->response.status_code = HTTP_STATUS_AUTHORIZATIONREQUIRED;
        client->response.realm = AUTH_REALM;
        return -1;
    }
    return 0;
}

void auth_cleanup() {
    struct auth_t *auth, *next;

    if (auth_ != NULL) {
        auth = auth_;
        while (auth != NULL) {
            next = auth->next;
            free(auth);
            auth = next;
        }
        auth_ = NULL;
    }
}

/* vim: set ts=4 sw=4 expandtab: */
