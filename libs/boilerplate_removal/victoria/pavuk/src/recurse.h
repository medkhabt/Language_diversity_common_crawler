/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _recurse_h_
#define _recurse_h_

#include "doc.h"
#include "dllist.h"

extern int process_document(doc *, int);
extern int download_single_doc(url *);
extern void recurse(int);
extern void get_urls_to_resume(char *);
extern void get_urls_to_synchronize(char *, dllist **);

extern void dump_url(url * urlp);
extern void dump_ftp_list(dllist *);
extern void dump_urls_list(dllist * sublist, doc * docu);
extern void dump_cfg_formdata(const char *urlstr, dllist * formdata);
extern void dump_formlist(const char *urlstr, dllist * formlist);
extern char *cvt_url_info2diag_str(url_info * ui);
extern void cvt_url2diag_str(url * urlp, char **urldbgstr_ref, char **urlstr_ref);


#endif
