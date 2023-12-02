/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _lfname_h_
#define _lfname_h_

#ifdef HAVE_REGEX
#ifdef HAVE_PCRE_REGEX
#include <pcre.h>
#endif
#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#ifdef HAVE_V8_REGEX
#ifndef NSUBEXP
#include <regexp.h>
#endif
#endif
#endif
#endif

#include "url.h"

typedef enum
{
  LFNAME_UNKNOWN,
#ifdef HAVE_REGEX
  LFNAME_REGEX,
#endif
  LFNAME_FNMATCH
} lfname_type;

typedef struct
{
  lfname_type type;
#ifdef HAVE_REGEX
#ifdef HAVE_POSIX_REGEX
  regex_t preg;
  regmatch_t *pmatch;
#endif
#ifdef HAVE_V8_REGEX
  regexp *preg;
#endif
#ifdef HAVE_GNU_REGEX
  struct re_pattern_buffer preg;
  struct re_registers pmatch;
#endif
#ifdef HAVE_PCRE_REGEX
  pcre *preg;
  pcre_extra *preg_extra;
  int pmatch_nr;
  int *pmatch;
#endif
#ifdef HAVE_TRE_REGEX
  regex_t preg;
  regmatch_t *pmatch;
#endif
#endif
  char *matchstr;
  char *transstr;
} lfname;

/* need to export for jsbind.c */
struct lfname_lsp_interp
{
  url *urlp;
  const char *urlstr;
  const char *scheme;
  const char *passwd;
  const char *user;
  const char *host;
  const char *domain;
  const char *port;
  const char *path;
  const char *name;
  const char *basename;
  const char *extension;
  const char *query;
  const char *post_query;
  const char *deflt;
  const char *mime_type;
  const char *basic_mime_type;
  const char *mime_type_attribs;
  const char *mime_type_ext;
  lfname *orig;
};

extern lfname *lfname_new(lfname_type type, const char *mpt, const char *str);
extern void lfname_free(lfname *lfnamep);
extern char *lfname_get_by_url(url *urlp, const char *urlstr, const char *local_name, const char *mime_type, lfname *lfnamep);
extern int lfname_match(lfname *lfnamep, const char *urlstr);
extern int lfname_check_pattern(lfname_type type, const char *str);
extern int lfname_check_rule(const char *str);
extern const char *lfname_interp_get_macro(struct lfname_lsp_interp *interp, int macro);
extern int lfname_check_macro(int macro);
#ifdef HAVE_REGEX
extern char *lfname_re_sub(lfname *lfnamep, const char *urlstr, int nr);
#endif


#endif
