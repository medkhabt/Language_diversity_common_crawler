/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _tr_h_
#define _tr_h_

extern char *tr_chr_chr(const char *fset, const char *tset, const char *str);
extern char *tr_str_str(const char *s1, const char *s2, const char *str);
extern char *tr_del_chr(const char *set, const char *str);
extern char *tr(const char *str);

#endif
