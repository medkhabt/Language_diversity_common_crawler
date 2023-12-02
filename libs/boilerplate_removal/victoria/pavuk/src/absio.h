/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _absio_h_
#define _absio_h_

#ifdef __QNX__
#define tl_sock_read(fd, buf, size, ssld) tl_unix_read(fd, buf, size, ssld)
#define tl_sock_nbread(fd, buf, size, ssld) tl_unix_nbread(fd, buf, size, ssld)
#define tl_sock_write(fd, buf, size, ssld) tl_unix_write(fd, buf, size, ssld)
#define tl_file_read(fd, buf, size) read(fd, buf, size)
#define tl_file_write(fd, buf, size) write(fd, buf, size)
#elif defined __BEOS__
/* [i_a] BeOS patch changed; VERY similar to Win32 needs BTW */
#define tl_sock_read(fd, buf, size, ssld) tl_unix_recv(fd, buf, size, ssld)
#define tl_sock_nbread(fd, buf, size, ssld) tl_unix_nbrecv(fd, buf, size, ssld)
#define tl_sock_write(fd, buf, size, ssld) tl_unix_send(fd, buf, size, ssld)
#define tl_file_read(fd, buf, size) read(fd, buf, size)
#define tl_file_write(fd, buf, size) write(fd, buf, size)
#elif defined(WIN32) || defined(__WIN32)
#define tl_sock_read(fd, buf, size, ssld) tl_unix_read(fd, buf, size, ssld)
#define tl_sock_nbread(fd, buf, size, ssld) tl_unix_nbread(fd, buf, size, ssld)
#define tl_sock_write(fd, buf, size, ssld) tl_unix_write(fd, buf, size, ssld)
#define tl_file_read(fd, buf, size) read(fd, buf, size)
#define tl_file_write(fd, buf, size) write(fd, buf, size)
#else
#define tl_sock_read(fd, buf, size, ssld) tl_unix_read(fd, buf, size, ssld)
#define tl_sock_nbread(fd, buf, size, ssld) tl_unix_nbread(fd, buf, size, ssld)
#define tl_sock_write(fd, buf, size, ssld) tl_unix_write(fd, buf, size, ssld)
#define tl_file_read(fd, buf, size) tl_unix_read(fd, buf, size, NULL)
#define tl_file_write(fd, buf, size) tl_unix_write(fd, buf, size, NULL)
#endif

/* [i_a] BeOS patch changed; VERY similar to Win32 needs BTW */
extern int tl_unix_read(int, char *, size_t, void *);
extern int tl_unix_nbread(int, char *, size_t, void *);
extern int tl_unix_write(int, char *, size_t, void *);
extern int tl_selectr(int, int);
extern int tl_selectw(int, int);

#endif
