/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef __net_h__
#define __net_h__

#include "dns.h"

extern int net_connect(char *, int, doc *);
extern int net_bindport(abs_addr *, int, int);
extern int net_accept(int);
extern int net_host_to_in_addr(char *, abs_addr *);
extern int sock_maybe_in_fdset(fd_set * set, int sock); /* [i_a] apr/2007 fixup for Winsock2 FD_SET handling */

#endif
