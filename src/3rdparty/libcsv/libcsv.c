/*
libcsv - parse and write csv data
Copyright (C) 2007  Robert Gamble

    available at http://libcsv.sf.net

    Original available under the terms of the GNU LGPL2, and according
    to those terms, relicensed under the GNU GPL2 for inclusion in Tellico */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#if ___STDC_VERSION__ >= 199901L
#  include <stdint.h>
#else
#  define SIZE_MAX ((size_t)-1) /* C89 doesn't have stdint.h or SIZE_MAX */
#endif

#include "libcsv.h"

#define VERSION "2.0.0"

#define ROW_NOT_BEGUN           0
#define FIELD_NOT_BEGUN         1
#define FIELD_BEGUN             2
#define FIELD_MIGHT_HAVE_ENDED  3

/*
  Explanation of states
  ROW_NOT_BEGUN    There have not been any fields encountered for this row
  FIELD_NOT_BEGUN  There have been fields but we are currently not in one
  FIELD_BEGUN      We are in a field
  FIELD_MIGHT_HAVE_ENDED
                   We encountered a double quote inside a quoted field, the
                   field is either ended or the quote is literal
*/

#define MEM_BLK_SIZE 128

#define SUBMIT_FIELD(p) \
  do { \
   if (!quoted) \
     entry_pos -= spaces; \
   if (cb1) \
     cb1(p->entry_buf, entry_pos, data); \
   pstate = FIELD_NOT_BEGUN; \
   entry_pos = quoted = spaces = 0; \
 } while (0)

#define SUBMIT_ROW(p, c) \
  do { \
    if (cb2) \
      cb2(c, data); \
    pstate = ROW_NOT_BEGUN; \
    entry_pos = quoted = spaces = 0; \
  } while (0)

#define SUBMIT_CHAR(p, c) ((p)->entry_buf[entry_pos++] = (c))

static char *csv_errors[] = {"success",
                             "error parsing data while strict checking enabled",
                             "memory exhausted while increasing buffer size",
                             "data size too large",
                             "invalid status code"};

int
csv_error(struct csv_parser *p)
{
  return p->status;
}

char *
csv_strerror(int status)
{
  if (status >= CSV_EINVALID || status < 0)
    return csv_errors[CSV_EINVALID];
  else
    return csv_errors[status];
}

int
csv_opts(struct csv_parser *p, unsigned char options)
{
  if (p == NULL)
    return -1;

  p->options = options;
  return 0;
}

int
csv_init(struct csv_parser **p, unsigned char options)
{
  /* Initialize a csv_parser object returns 0 on success, -1 on error */
  if (p == NULL)
    return -1;

  if ((*p = malloc(sizeof(struct csv_parser))) == NULL)
    return -1;

  if ( ((*p)->entry_buf = malloc(MEM_BLK_SIZE)) == NULL ) {
    free(*p);
    return -1;
  }
  (*p)->pstate = ROW_NOT_BEGUN;
  (*p)->quoted = 0;
  (*p)->spaces = 0;
  (*p)->entry_pos = 0;
  (*p)->entry_size = MEM_BLK_SIZE;
  (*p)->status = 0;
  (*p)->options = options;
  (*p)->quote_char = CSV_QUOTE;
  (*p)->delim_char = CSV_COMMA;
  (*p)->is_space = NULL;
  (*p)->is_term = NULL;

  return 0;
}

void
csv_free(struct csv_parser *p)
{
  /* Free the entry_buffer and the csv_parser object */
  if (p == NULL)
    return;

  if (p->entry_buf)
    free(p->entry_buf);

  free(p);
  return;
}

int
csv_fini(struct csv_parser *p, void (*cb1)(char *, size_t, void *), void (*cb2)(char c, void *), void *data)
{
  /* Finalize parsing.  Needed, for example, when file does not end in a newline */
  int quoted = p->quoted;
  int pstate = p->pstate;
  size_t spaces = p->spaces;
  size_t entry_pos = p->entry_pos;

  if (p == NULL)
    return -1;


  if (p->pstate == FIELD_BEGUN && p->quoted && p->options & CSV_STRICT && p->options & CSV_STRICT_FINI) {
    p->status = CSV_EPARSE;
    return -1;
  }

  switch (p->pstate) {
    case FIELD_MIGHT_HAVE_ENDED:
      p->entry_pos -= p->spaces + 1;  /* get rid of spaces and original quote */
    case FIELD_NOT_BEGUN:
    case FIELD_BEGUN:
      quoted = p->quoted, pstate = p->pstate;
      spaces = p->spaces, entry_pos = p->entry_pos;
      SUBMIT_FIELD(p);
      SUBMIT_ROW(p, 0);
    case ROW_NOT_BEGUN: /* Already ended properly */
      ;
  }

  p->spaces = p->quoted = p->entry_pos = p->status = 0;
  p->pstate = ROW_NOT_BEGUN;

  return 0;
}

void
csv_set_delim(struct csv_parser *p, char c)
{
  if (p) p->delim_char = c;
}

void
csv_set_quote(struct csv_parser *p, char c)
{
  if (p) p->quote_char = c;
}

char
csv_get_delim(struct csv_parser *p)
{
  return p->delim_char;
}

char
csv_get_quote(struct csv_parser *p)
{
  return p->quote_char;
}

void
csv_set_space_func(struct csv_parser *p, int (*f)(char))
{
  if (p) p->is_space = f;
}

void
csv_set_term_func(struct csv_parser *p, int (*f)(char))
{
  if (p) p->is_term = f;
}

static int
csv_increase_buffer(struct csv_parser *p)
{
  size_t to_add = MEM_BLK_SIZE;
  void *vp;
  while ( p->entry_size >= SIZE_MAX - to_add )
    to_add /= 2;
  if (!to_add) {
    p->status = CSV_ETOOBIG;
    return -1;
  }
  while ((vp = realloc(p->entry_buf, p->entry_size + to_add)) == NULL) {
    to_add /= 2;
    if (!to_add) {
      p->status = CSV_ENOMEM;
      return -1;
    }
  }
  p->entry_buf = vp;
  p->entry_size += to_add;
  return 0;
}

size_t
csv_parse(struct csv_parser *p, const char *s, size_t len, void (*cb1)(char *, size_t, void *), void (*cb2)(char c, void *), void *data)
{
  char c;  /* The character we are currently processing */
  size_t pos = 0;  /* The number of characters we have processed in this call */
  char delim = p->delim_char;
  char quote = p->quote_char;
  int (*is_space)(char) = p->is_space;
  int (*is_term)(char) = p->is_term;
  int quoted = p->quoted;
  int pstate = p->pstate;
  size_t spaces = p->spaces;
  size_t entry_pos = p->entry_pos;

  while (pos < len) {
    /* Check memory usage */
    if (entry_pos == p->entry_size)
      if (csv_increase_buffer(p) != 0) {
        p->quoted = quoted, p->pstate = pstate, p->spaces = spaces, p->entry_pos = entry_pos;
        return pos;
      }

    c = s[pos++];
    switch (pstate) {
      case ROW_NOT_BEGUN:
      case FIELD_NOT_BEGUN:
        if (is_space ? is_space(c) : c == CSV_SPACE || c == CSV_TAB) { /* Space or Tab */
          continue;
        } else if (is_term ? is_term(c) : c == CSV_CR || c == CSV_LF) { /* Carriage Return or Line Feed */
          if (pstate == FIELD_NOT_BEGUN) {
            SUBMIT_FIELD(p);
            SUBMIT_ROW(p, c);
          } else {  /* ROW_NOT_BEGUN */
            /* Don't submit empty rows by default */
            if (p->options & CSV_REPALL_NL) {
              SUBMIT_ROW(p, c);
            }
          }
          continue;
        } else if (c == delim) { /* Comma */
          SUBMIT_FIELD(p);
          break;
        } else if (c == quote) { /* Quote */
          pstate = FIELD_BEGUN;
          quoted = 1;
        } else {               /* Anything else */
          pstate = FIELD_BEGUN;
          quoted = 0;
          SUBMIT_CHAR(p, c);
        }
        break;
      case FIELD_BEGUN:
        if (c == quote) {         /* Quote */
          if (quoted) {
            SUBMIT_CHAR(p, c);
            pstate = FIELD_MIGHT_HAVE_ENDED;
          } else {
            /* STRICT ERROR - double quote inside non-quoted field */
            if (p->options & CSV_STRICT) {
              p->status = CSV_EPARSE;
              p->quoted = quoted, p->pstate = pstate, p->spaces = spaces, p->entry_pos = entry_pos;
              return pos-1;
            }
            SUBMIT_CHAR(p, c);
            spaces = 0;
          }
        } else if (c == delim) {  /* Comma */
          if (quoted) {
            SUBMIT_CHAR(p, c);
          } else {
            SUBMIT_FIELD(p);
          }
        } else if (is_term ? is_term(c) : c == CSV_CR || c == CSV_LF) {  /* Carriage Return or Line Feed */
          if (!quoted) {
            SUBMIT_FIELD(p);
            SUBMIT_ROW(p, c);
          } else {
            SUBMIT_CHAR(p, c);
          }
        } else if (!quoted && (is_space? is_space(c) : c == CSV_SPACE || c == CSV_TAB)) { /* Tab or space for non-quoted field */
            SUBMIT_CHAR(p, c);
            spaces++;
        } else {  /* Anything else */
          SUBMIT_CHAR(p, c);
          spaces = 0;
        }
        break;
      case FIELD_MIGHT_HAVE_ENDED:
        /* This only happens when a quote character is encountered in a quoted field */
        if (c == delim) {  /* Comma */
          entry_pos -= spaces + 1;  /* get rid of spaces and original quote */
          SUBMIT_FIELD(p);
        } else if (is_term ? is_term(c) : c == CSV_CR || c == CSV_LF) {  /* Carriage Return or Line Feed */
          entry_pos -= spaces + 1;  /* get rid of spaces and original quote */
          SUBMIT_FIELD(p);
          SUBMIT_ROW(p, c);
        } else if (is_space ? is_space(c) : c == CSV_SPACE || c == CSV_TAB) {  /* Space or Tab */
          SUBMIT_CHAR(p, c);
          spaces++;
        } else if (c == quote) {  /* Quote */
          if (spaces) {
            /* STRICT ERROR - unescaped double quote */
            if (p->options & CSV_STRICT) {
              p->status = CSV_EPARSE;
              p->quoted = quoted, p->pstate = pstate, p->spaces = spaces, p->entry_pos = entry_pos;
              return pos-1;
            }
            spaces = 0;
            SUBMIT_CHAR(p, c);
          } else {
            /* Two quotes in a row */
            pstate = FIELD_BEGUN;
          }
        } else {  /* Anything else */
          /* STRICT ERROR - unescaped double quote */
          if (p->options & CSV_STRICT) {
            p->status = CSV_EPARSE;
            p->quoted = quoted, p->pstate = pstate, p->spaces = spaces, p->entry_pos = entry_pos;
            return pos-1;
          }
          pstate = FIELD_BEGUN;
          spaces = 0;
          SUBMIT_CHAR(p, c);
        }
        break;
     default:
       break;
    }
  }
  p->quoted = quoted, p->pstate = pstate, p->spaces = spaces, p->entry_pos = entry_pos;
  return pos;
}

size_t
csv_write (char *dest, size_t dest_size, const char *src, size_t src_size)
{
  size_t chars = 0;

  if (src == NULL)
    return 0;

  if (dest == NULL)
    dest_size = 0;

  if (dest_size > 0)
    *dest++ = '"';
  chars++;

  while (src_size) {
    if (*src == '"') {
      if (dest_size > chars)
        *dest++ = '"';
      if (chars < SIZE_MAX) chars++;
    }
    if (dest_size > chars)
      *dest++ = *src;
    if (chars < SIZE_MAX) chars++;
    src_size--;
    src++;
  }

  if (dest_size > chars)
    *dest = '"';
  if (chars < SIZE_MAX) chars++;

  return chars;
}

int
csv_fwrite (FILE *fp, const char *src, size_t src_size)
{
  if (fp == NULL || src == NULL)
    return 0;

  if (fputc('"', fp) == EOF)
    return EOF;

  while (src_size) {
    if (*src == '"') {
      if (fputc('"', fp) == EOF)
        return EOF;
    }
    if (fputc(*src, fp) == EOF)
      return EOF;
    src_size--;
    src++;
  }

  if (fputc('"', fp) == EOF) {
    return EOF;
  }

  return 0;
}

size_t
csv_write2 (char *dest, size_t dest_size, const char *src, size_t src_size, char quote)
{
  size_t chars = 0;

  if (src == NULL)
    return 0;

  if (dest == NULL)
    dest_size = 0;

  if (dest_size > 0)
    *dest++ = quote;
  chars++;

  while (src_size) {
    if (*src == quote) {
      if (dest_size > chars)
        *dest++ = quote;
      if (chars < SIZE_MAX) chars++;
    }
    if (dest_size > chars)
      *dest++ = *src;
    if (chars < SIZE_MAX) chars++;
    src_size--;
    src++;
  }

  if (dest_size > chars)
    *dest = quote;
  if (chars < SIZE_MAX) chars++;

  return chars;
}

int
csv_fwrite2 (FILE *fp, const char *src, size_t src_size, char quote)
{
  if (fp == NULL || src == NULL)
    return 0;

  if (fputc(quote, fp) == EOF)
    return EOF;

  while (src_size) {
    if (*src == quote) {
      if (fputc(quote, fp) == EOF)
        return EOF;
    }
    if (fputc(*src, fp) == EOF)
      return EOF;
    src_size--;
    src++;
  }

  if (fputc(quote, fp) == EOF) {
    return EOF;
  }

  return 0;
}
