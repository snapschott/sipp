/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Author : Richard GAYRAUD - 04 Nov 2003
 *           Olivier Jacques
 *           From Hewlett Packard Company.
 *           Shriram Natarajan
 *           Peter Higginson
 *           Eric Miller
 *           Venkatesh
 *           Enrico Hartung
 *           Nasir Khan
 *           Lee Ballard
 *           Guillaume Teissier from FTR&D
 *           Wolfgang Beck
 *           Venkatesh
 *           Vlad Troyanker
 *           Charles P Wright from IBM Research
 *           Amit On from Followap
 *           Jan Andres from Freenet
 *           Ben Evans from Open Cloud
 *           Marc Van Diest from Belgacom
 *           Stefan Esser
 *           Andy Aicken
 *           Walter Doekes
 */

#include <string.h>
#include <stdlib.h>

#include "sipp.hpp"

#include "screen.hpp"
#include "strings.hpp"
#include "sip_parser.hpp"

/*************************** Mini SIP parser ***************************/

char * get_peer_tag(const char *msg)
{
    static char   tag[MAX_HEADER_LEN];
    const char  * to_hdr;
    const char  * ptr;
    int           tag_i = 0;

    /* Find start of header */
    to_hdr = internal_find_header(msg, "To", "t", true);
    if (!to_hdr) {
        ERROR("No valid To: header in reply");
    }

    /* Skip past display-name */
    /* FIXME */

    /* Skip past LA/RA-quoted addr-spec if any */
    ptr = internal_hdrchr(to_hdr, '>');
    if (!ptr) {
        /* Maybe an addr-spec without quotes */
        ptr = to_hdr;
    }

    /* Find tag in this header */
    ptr = internal_find_param(ptr, "tag");
    if (!ptr) {
        return NULL;
    }

    while ((*ptr != ' ')  &&
            (*ptr != ';')  &&
            (*ptr != '\t') &&
            (*ptr != '\r') &&
            (*ptr != '\n') &&
            (*ptr)) {
        tag[tag_i++] = *(ptr++);
    }
    tag[tag_i] = '\0';

    return tag;
}

char * get_header_content(const char* message, const char * name)
{
    return get_header(message, name, true);
}

/* If content is true, we only return the header's contents. */
char * get_header(const char* message, const char * name, bool content)
{
    /* non reentrant. consider accepting char buffer as param */
    static char last_header[MAX_HEADER_LEN * 10];
    char *src, *src_orig, *dest, *start, *ptr;
    /* Are we searching for a short form header? */
    bool short_form = false;
    bool first_time = true;
    char header_with_newline[MAX_HEADER_LEN + 1];

    /* returns empty string in case of error */
    last_header[0] = '\0';

    if((!message) || (!strlen(message))) {
        return last_header;
    }

    /* for safety's sake */
    if (NULL == name || NULL == strrchr(name, ':')) {
        WARNING("Can not search for header (no colon): %s", name ? name : "(null)");
        return last_header;
    }

    src_orig = strdup(message);

    do {
        /* We want to start from the beginning of the message each time
         * through this loop, because we may be searching for a short form. */
        src = src_orig;

        snprintf(header_with_newline, MAX_HEADER_LEN, "\n%s", name);
        dest = last_header;

        while((src = strcasestr2(src, header_with_newline))) {
            if (content || !first_time) {
                /* Just want the header's content, so skip over the header
                 * and newline */
                src += strlen(name) + 1;
            } else {
                /* Just skip the newline */
                src++;
            }
            first_time = false;
            ptr = strchr(src, '\n');

            /* Multiline headers always begin with a tab or a space
             * on the subsequent lines. Skip those lines. */
            while((ptr) &&
                    ((*(ptr+1) == ' ' ) ||
                     (*(ptr+1) == '\t')    )) {
                ptr = strchr(ptr + 1, '\n');
            }

            if(ptr) {
                *ptr = 0;
            }
            // Add "," when several headers are present
            if (dest != last_header) {
                /* Remove trailing whitespaces, tabs, and CRs */
                while ((dest > last_header) &&
                        ((*(dest-1) == ' ')  ||
                         (*(dest-1) == '\r') ||
                         (*(dest-1) == '\n') ||
                         (*(dest-1) == '\t'))) {
                    *(--dest) = 0;
                }

                dest += sprintf(dest, ",");
            }
            dest += sprintf(dest, "%s", src);
            if(ptr) {
                *ptr = '\n';
            }

            src++;
        }
        /* We found the header. */
        if(dest != last_header) {
            break;
        }
        /* We didn't find the header, even in its short form. */
        if (short_form) {
            free(src_orig);
            return last_header;
        }

        /* We should retry with the short form. */
        short_form = true;
        if (!strcasecmp(name, "call-id:")) {
            name = "i:";
        } else if (!strcasecmp(name, "contact:")) {
            name = "m:";
        } else if (!strcasecmp(name, "content-encoding:")) {
            name = "e:";
        } else if (!strcasecmp(name, "content-length:")) {
            name = "l:";
        } else if (!strcasecmp(name, "content-type:")) {
            name = "c:";
        } else if (!strcasecmp(name, "from:")) {
            name = "f:";
        } else if (!strcasecmp(name, "to:")) {
            name = "t:";
        } else if (!strcasecmp(name, "via:")) {
            name = "v:";
        } else {
            /* There is no short form to try. */
            free(src_orig);
            return last_header;
        }
    } while (1);

    *(dest--) = 0;

    /* Remove trailing whitespaces, tabs, and CRs */
    while ((dest > last_header) &&
            ((*dest == ' ') || (*dest == '\r')|| (*dest == '\t'))) {
        *(dest--) = 0;
    }

    /* Remove leading whitespaces */
    for (start = last_header; *start == ' '; start++);

    /* remove enclosed CRs in multilines */
    /* don't remove enclosed CRs for multiple headers (e.g. Via) (Rhys) */
    while((ptr = strstr(last_header, "\r\n")) != NULL
            && (   *(ptr + 2) == ' '
                   || *(ptr + 2) == '\r'
                   || *(ptr + 2) == '\t') ) {
        /* Use strlen(ptr) to include trailing zero */
        memmove(ptr, ptr+1, strlen(ptr));
    }

    /* Remove illegal double CR characters */
    while((ptr = strstr(last_header, "\r\r")) != NULL) {
        memmove(ptr, ptr+1, strlen(ptr));
    }
    /* Remove illegal double Newline characters */
    while((ptr = strstr(last_header, "\n\n")) != NULL) {
        memmove(ptr, ptr+1, strlen(ptr));
    }

    free(src_orig);
    return start;
}

char * get_first_line(const char * message)
{
    /* non reentrant. consider accepting char buffer as param */
    static char last_header[MAX_HEADER_LEN * 10];
    const char * src;

    /* returns empty string in case of error */
    memset(last_header, 0, sizeof(last_header));

    if((!message) || (!strlen(message))) {
        return last_header;
    }

    src = message;

    int i=0;
    while (*src) {
        if((*src=='\n')||(*src=='\r')) {
            break;
        } else {
            last_header[i]=*src;
        }
        i++;
        src++;
    }

    return last_header;
}

char * get_call_id(char *msg)
{
    static char call_id[MAX_HEADER_LEN];
    char * ptr1, * ptr2, * ptr3, backup;
    bool short_form;

    call_id[0] = '\0';

    short_form = false;

    ptr1 = strstr(msg, "Call-ID:");
    if(!ptr1) {
        ptr1 = strstr(msg, "Call-Id:");
    }
    if(!ptr1) {
        ptr1 = strstr(msg, "Call-id:");
    }
    if(!ptr1) {
        ptr1 = strstr(msg, "call-Id:");
    }
    if(!ptr1) {
        ptr1 = strstr(msg, "call-id:");
    }
    if(!ptr1) {
        ptr1 = strstr(msg, "CALL-ID:");
    }
    // For short form, we need to make sure we start from beginning of line
    // For others, no need to
    if(!ptr1) {
        ptr1 = strstr(msg, "\r\ni:");
        short_form = true;
    }
    if(!ptr1) {
        WARNING("(1) No valid Call-ID: header in reply '%s'", msg);
        return call_id;
    }

    if (short_form) {
        ptr1 += 4;
    } else {
        ptr1 += 8;
    }

    while((*ptr1 == ' ') || (*ptr1 == '\t')) {
        ptr1++;
    }

    if(!(*ptr1)) {
        WARNING("(2) No valid Call-ID: header in reply");
        return call_id;
    }

    ptr2 = ptr1;

    while((*ptr2) &&
            (*ptr2 != ' ') &&
            (*ptr2 != '\t') &&
            (*ptr2 != '\r') &&
            (*ptr2 != '\n')) {
        ptr2 ++;
    }

    if(!*ptr2) {
        WARNING("(3) No valid Call-ID: header in reply");
        return call_id;
    }

    backup = *ptr2;
    *ptr2 = 0;
    if ((ptr3 = strstr(ptr1, "///")) != 0) ptr1 = ptr3+3;
    strcpy(call_id, ptr1);
    *ptr2 = backup;
    return (char *) call_id;
}

unsigned long int get_cseq_value(char *msg)
{
    char *ptr1;


    // no short form for CSeq:
    ptr1 = strstr(msg, "\r\nCSeq:");
    if(!ptr1) {
        ptr1 = strstr(msg, "\r\nCSEQ:");
    }
    if(!ptr1) {
        ptr1 = strstr(msg, "\r\ncseq:");
    }
    if(!ptr1) {
        ptr1 = strstr(msg, "\r\nCseq:");
    }
    if(!ptr1) {
        WARNING("No valid Cseq header in request %s", msg);
        return 0;
    }

    ptr1 += 7;

    while((*ptr1 == ' ') || (*ptr1 == '\t')) {
        ++ptr1;
    }

    if(!(*ptr1)) {
        WARNING("No valid Cseq data in header");
        return 0;
    }

    return strtoul(ptr1, NULL, 10);
}

unsigned long get_reply_code(char *msg)
{
    while((msg) && (*msg != ' ') && (*msg != '\t')) msg ++;
    while((msg) && ((*msg == ' ') || (*msg == '\t'))) msg ++;

    if ((msg) && (strlen(msg)>0)) {
        return atol(msg);
    } else {
        return 0;
    }
}

const char *internal_find_header(const char *msg, const char *name, const char *shortname,
        bool content)
{
    /* http://www.in2eps.com/fo-abnf/tk-fo-abnf-sip.html */

    const char *ptr;
    int namelen = strlen(name);
    int shortnamelen = shortname ? strlen(shortname) : 0;

    /* Seek past request/response line */
    ptr = strchr(msg, '\n');
    if (!ptr || ptr == msg || ptr[-1] != '\r') {
        return NULL;
    }
    ++ptr;

    while (1) {
        int is_short = 0;
        /* TODO: should not be "case"-str here.. do we want to be strict? */
        if (strncasecmp(ptr, name, namelen) == 0 ||
                (shortname && (is_short = 1) &&
                    strncasecmp(msg, shortname, shortnamelen) == 0)) {
            const char *tmp = ptr + (is_short ? strlen(shortname) : strlen(name));
            while (*tmp == ' ' || *tmp == '\t') {
                ++tmp;
            }
            if (*tmp == ':') {
                /* Found */
                if (content) {
                    /* We just want the content */
                    ptr = internal_skip_lws(tmp + 1);
                }
                break;
            }
        }

        /* Seek to next line, but not past EOH */
        ptr = strchr(ptr, '\n');
        if (!ptr || ptr[-1] != '\r' || (ptr[1] == '\r' && ptr[2] == '\n')) {
            return NULL;
        }
        ++ptr;
    }

    return ptr;
}

const char *internal_hdrchr(const char *ptr, const char needle)
{
    if (*ptr == '\n') {
        return NULL; /* stray LF */
    }

    while (1) {
        if (*ptr == '\0') {
            return NULL;
        } else if (*ptr == needle) {
            return ptr;
        } else if (*ptr == '\n') {
            if (ptr[-1] == '\r' && ptr[1] != ' ' && ptr[1] != '\t') {
                return NULL; /* end of header */
            }
        }
        ++ptr;
    }

    return NULL; /* never gets here */
}

const char *internal_find_param(const char *ptr, const char *name)
{
    int namelen = strlen(name);

    while (1) {
        ptr = internal_hdrchr(ptr, ';');
        if (!ptr) {
            return NULL;
        }
        ++ptr;

        ptr = internal_skip_lws(ptr);
        if (!ptr || !*ptr) {
            return NULL;
        }

        /* TODO: should not be "case"-str here.. do we want to be strict? */
        if (strncasecmp(ptr, name, namelen) == 0 && *(ptr + namelen) == '=') {
            ptr += namelen + 1;
            return ptr;
        }
    }

    return NULL; /* never gets here */
}

const char *internal_skip_lws(const char *ptr)
{
    while (1) {
        while (*ptr == ' ' || *ptr == '\t') {
            ++ptr;
        }
        if (ptr[0] == '\r' && ptr[1] == '\n') {
            if (ptr[2] == ' ' || ptr[2] == '\t') {
                ptr += 3;
                continue;
            }
            return NULL; /* end of this header */
        }
        return ptr;
    }
    return NULL; /* never gets here */
}
