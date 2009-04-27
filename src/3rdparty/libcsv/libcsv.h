/*
libcsv - parse and write csv data
Copyright (C) 2007  Robert Gamble

    available at http://libcsv.sf.net

    Original available under the terms of the GNU LGPL2, and according
    to those terms, relicensed under the GNU GPL2 for inclusion in Tellico */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/


#ifndef LIBCSV_H__
#define LIBCSV_H__
#include <stdlib.h>
#include <stdio.h>

#define CSV_MAJOR 2
#define CSV_MINOR 0
#define CSV_RELEASE 1

/* Error Codes */
#define CSV_SUCCESS 0
#define CSV_EPARSE 1   /* Parse error in strict mode */
#define CSV_ENOMEM 2   /* Out of memory while increasing buffer size */
#define CSV_ETOOBIG 3  /* Buffer larger than SIZE_MAX needed */
#define CSV_EINVALID 4 /* Invalid code,should never be received from csv_error*/


/* parser options */
#define CSV_STRICT 1    /* enable strict mode */
#define CSV_REPALL_NL 2 /* report all unquoted carriage returns and linefeeds */
#define CSV_STRICT_FINI 4 /* causes csv_fini to return CSV_EPARSE if last
                             field is quoted and doesn't containg ending
                             quote */

/* Character values */
#define CSV_TAB    0x09
#define CSV_SPACE  0x20
#define CSV_CR     0x0d
#define CSV_LF     0x0a
#define CSV_COMMA  0x2c
#define CSV_QUOTE  0x22

struct csv_parser {
  int pstate;         /* Parser state */
  int quoted;         /* Is the current field a quoted field? */
  size_t spaces;      /* Number of continious spaces after quote or in a non-quoted field */
  char * entry_buf;   /* Entry buffer */
  size_t entry_pos;   /* Current position in entry_buf (and current size of entry) */
  size_t entry_size;  /* Size of buffer */
  int status;         /* Operation status */
  unsigned char options;
  char quote_char;
  char delim_char;
  int (*is_space)(char);
  int (*is_term)(char);
};

int csv_init(struct csv_parser **p, unsigned char options);
int csv_fini(struct csv_parser *p, void (*cb1)(char *, size_t, void *), void (*cb2)(char, void *), void *data);
void csv_free(struct csv_parser *p);
int csv_error(struct csv_parser *p);
char * csv_strerror(int error);
size_t csv_parse(struct csv_parser *p, const char *s, size_t len, void (*cb1)(char *, size_t, void *), void (*cb2)(char, void *), void *data);
size_t csv_write(char *dest, size_t dest_size, const char *src, size_t src_size);
int csv_fwrite(FILE *fp, const char *src, size_t src_size);
size_t csv_write2(char *dest, size_t dest_size, const char *src, size_t src_size, char quote);
int csv_fwrite2(FILE *fp, const char *src, size_t src_size, char quote);
int csv_opts(struct csv_parser *p, unsigned char options);
void csv_set_delim(struct csv_parser *p, char c);
void csv_set_quote(struct csv_parser *p, char c);
char csv_get_delim(struct csv_parser *p);
char csv_get_quote(struct csv_parser *p);
void csv_set_space_func(struct csv_parser *p, int (*f)(char));
void csv_set_term_func(struct csv_parser *p, int (*f)(char));

#endif
