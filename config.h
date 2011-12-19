/* Aranea
 * Copyright (c) 2011, Nguyen Quoc Viet
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_CONFIG_H_
#define ARANEA_CONFIG_H_

#define SERVER_NAME                 "Aranea/0.0.1"
#define HTTP_VERSION                "HTTP/1.0"

#define MAX_REQUEST_LENGTH          1024
#define MAX_PATH_LENGTH             512         /* PATH_MAX */

#define WWW_ROOT                    "/var/www"
#define WWW_INDEX                   "index.html"
#define PORT                        8080
#define SERVER_TIMEOUT              60          /* sec */
#define CLIENT_TIMEOUT              60          /* sec */

#define CGI_PREFIX                  "/cgi-bin"
#define INDEX_NAME                  "index.html"

#endif /* ARANEA_CONFIG_H_ */

/* vim: set ts=4 sw=4 expandtab: */
