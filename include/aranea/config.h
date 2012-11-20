/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#ifndef ARANEA_CONFIG_H_
#define ARANEA_CONFIG_H_

#define ARANEA_NAME                 "Aranea"
#define ARANEA_VERSION              "0.1"
#define SERVER_ID                   ARANEA_NAME "/" ARANEA_VERSION
#define HTTP_VERSION                "HTTP/1.1"

#define MAX_REQUEST_LENGTH          1024
#define MAX_PATH_LENGTH             512         /* PATH_MAX */
#define MAX_CGIENV_LENGTH           1024
#define MAX_CGIENV_ITEM             10
#define MAX_CONN                    10
#define NUM_CACHED_CONN             4

#define MAX_IP_LENGTH               40
#define MAX_DATE_LENGTH             32      /* */
#define DATE_FORMAT                 "%a, %d %b %Y %H:%M:%S GMT"

/* Currently, support only plain password */
#define AUTH_REALM                  SERVER_ID
#define MAX_AUTHPATH_LENGTH         128
#define MAX_AUTHUSER_LENGTH         32
#define MAX_AUTHPASS_LENGTH         32

#define WWW_INDEX                   "index.html"
#define PORT                        8080
#define SERVER_TIMEOUT              60          /* sec */
#define CLIENT_TIMEOUT              60          /* sec */

#define CGI_EXT                     ".cgi"
#define INDEX_NAME                  "index.html"

/* CGI environments */
#define CGI_DOCUMENT_ROOT
#define CGI_REQUEST_METHOD
#define CGI_REQUEST_URI
#define CGI_HTTP_COOKIE

/* Features supported */
#ifndef HAVE_AUTH
# define HAVE_AUTH                  0
#endif
#ifndef HAVE_CGI
# define HAVE_CGI                   0
#endif
#ifndef HAVE_CHROOT
# define HAVE_CHROOT                0
#endif
#ifndef HAVE_VFORK
# define HAVE_VFORK                 0
#endif
#ifndef HAVE_TCPCORK
# define HAVE_TCPCORK               0
#endif

#endif /* ARANEA_CONFIG_H_ */

/* vim: set ts=4 sw=4 expandtab: */
