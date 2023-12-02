/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _cookie_h_
#define _cookie_h_

#if 0                           /* remove cyclic reference */
#include "url.h"
#else
struct _url;
#endif

/* private */
typedef struct _cookie_entry
{
  struct _cookie_entry *next;
  struct _cookie_entry *prev;
  char *domain;
  char *host;
  char *path;
  time_t expires;
  int secure;
  int flag;
  char *name;
  char *value;
  bool_t loaded;
} cookie_entry;

extern void cookie_deep_free(cookie_entry ** centry);
extern void cookie_free_all(cookie_entry ** centry);
extern cookie_entry *dup_cookie(cookie_entry * centry);
extern void cleanup_cookies(void);

extern int cookie_read_file(const char *);
extern int cookie_update_file(int);
extern char *cookie_get_field(struct _url *);
extern cookie_entry *cookie_parse(const char *field, struct _url *urlp);
extern void store_cookie_in_global_store(cookie_entry * centry);
extern char *cvt_cookie_to_setstr(cookie_entry * centry);
extern int fixup_cookie(cookie_entry * centry, struct _url *urlp);

#endif
