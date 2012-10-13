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

#define MAX_IP_LENGTH               40
#define MAX_DATE_LENGTH             32      /* */
#define DATE_FORMAT                 "%a, %d %b %Y %H:%M:%S GMT"

/* Currently, support only plain password */
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
#define CGI_QUERY_STRING
#define CGI_CONTENT_TYPE
#define CGI_CONTENT_LENGTH
#define CGI_HTTP_COOKIE


#endif /* ARANEA_CONFIG_H_ */

/* vim: set ts=4 sw=4 expandtab: */
