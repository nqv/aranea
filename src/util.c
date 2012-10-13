/* Aranea
 * Copyright (c) 2011-2012, Quoc-Viet Nguyen
 * See LICENSE file for copyright and license details.
 */

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

/* vim: set ts=4 sw=4 expandtab: */
