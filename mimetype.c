/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "aranea.h"

/* Default mimetype mappings */
static
const struct mimetype_t DEFAULT_MIME_TYPES[] = {
    {   "avi",      "video/x-msvideo" },
    {   "bin",      "application/octet-stream"      },
    {   "bz2",      "application/x-bzip2"           },
    {   "css",      "text/css"                      },
    {   "csv",      "text/csv"                      },
    {   "doc",      "application/msword"            },
    {   "dtd",      "text/xml"                      },
    {   "dump",     "application/octet-stream"      },
    {   "ps",       "application/postscript"        },
    {   "exe",      "application/octet-stream"      },
    {   "gif",      "image/gif"                     },
    {   "gz",       "application/x-gzip"            },
    {   "htm",      "text/html"                     },
    {   "html",     "text/html"                     },
    {   "jpe",      "image/jpeg"                    },
    {   "jpeg",     "image/jpeg"                    },
    {   "jpg",      "image/jpeg"                    },
    {   "js",       "text/javascript"               },
    {   "latex",    "application/x-latex"           },
    {   "midi",     "audio/midi"                    },
    {   "mp3",      "audio/mpeg"                    },
    {   "mpeg",     "video/mpeg"                    },
    {   "o",        "application/octet-stream"      },
    {   "pdf",      "application/pdf"               },
    {   "png",      "image/png"                     },
    {   "ppt",      "application/powerpoint"        },
    {   "ra",       "audio/x-pn-realaudio"          },
    {   "ram",      "audio/x-pn-realaudio"          },
    {   "rm",       "audio/x-pn-realaudio"          },
    {   "rtf",      "application/rtf"               },
    {   "swf",      "application/x-shockwave-flash" },
    {   "tar",      "application/x-tar"             },
    {   "tex",      "application/x-tex"             },
    {   "tgz",      "application/x-gzip"            },
    {   "tif",      "image/tiff"                    },
    {   "tiff",     "image/tiff"                    },
    {   "txt",      "text/plain"                    },
    {   "wav",      "audio/wav"                     },
    {   "xbm",      "image/x-xbitmap"               },
    {   "xml",      "text/xml"                      },
    {   "xpm",      "image/x-xpixmap"               },
    {   "zip",      "application/zip"               },
};

static
int mimetype_find_def(const char *ext) {
    int l, m, h, r;

    h = A_SIZEOF(DEFAULT_MIME_TYPES) - 1;   /* high */
    l = 0;                                  /* low */
    while (h >= l) {
        m = (h + l) / 2;
        r = strcasecmp(DEFAULT_MIME_TYPES[m].ext, ext);
        if (r < 0) {
            l = m + 1;
        } else if (r > 0) {
            h = m - 1;
        } else {
            return m;
        }
    }
    return -1;
}

const char *mimetype_get(const char *name) {
    int i;

    name = strrchr(name, '.');
    if (name != NULL) {
        i = mimetype_find_def(name + 1);
        if (i >= 0) {
            return DEFAULT_MIME_TYPES[i].type;
        }
    }
    /* Not found */
    return "text/plain";
}

/* vim: set ts=4 sw=4 expandtab: */
