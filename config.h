/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_CONFIG_H_
#define ARANEA_CONFIG_H_

#define SERVER_NAME                 "Aranea/0.0.1"
#define HTTP_VERSION                "HTTP/1.1"

#define MAX_REQUEST_LENGTH          1024
#define MAX_PATH_LENGTH             512         /* PATH_MAX */
#define MAX_CGIENV_LENGTH           1024
#define MAX_CGIENV_ITEM             10
#define MAX_CONN                    10
#define NUM_CACHED_CONN             4

#define WWW_INDEX                   "index.html"
#define PORT                        8080
#define SERVER_TIMEOUT              60          /* sec */
#define CLIENT_TIMEOUT              60          /* sec */

#define CGI_EXT                     ".cgi"
#define INDEX_NAME                  "index.html"

/* features supported */
#define HAVE_VFORK                  1           /* use vfork */
#define HAVE_CGI                    1
#define HAVE_HTTPPOST               1           /* support HTTP POST */

#endif /* ARANEA_CONFIG_H_ */

/* vim: set ts=4 sw=4 expandtab: */
