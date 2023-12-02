/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _times_h_
#define _times_h_

extern struct tm *new_tm(const struct tm *t);
extern time_t scntime(const char *timestr, bool_t sanitize);
extern time_t time_ftp_scn(const char *timestr, bool_t sanitize);
extern time_t time_scn_cmd(const char *timestr);
extern time_t tl_mktime(struct tm *tm);
extern time_t tl_mkgmtime(struct tm *tm);

#endif
