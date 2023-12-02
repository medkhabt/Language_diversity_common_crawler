/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _bufio_h_
#define _bufio_h_

typedef struct
{
  int fd;
  int open_flags;
  char *buf;
  int buf_size;
  int buf_start;
  int buf_end;
  int bufio_errno;
  int eof;
  int is_sock;
  void *ssl_hook_data;
  int no_close;

#ifdef HAVE_MT
  pthread_mutex_t *mutex;
#else
  void *mutex;
#endif
  char *filename;               /* [optional] filename. NULL if not available */
  int flags;
} bufio;

#define BUFIO_ADVISED_READLN_BUFSIZE            8192    /* was: 4096 */
#define BUFIO_ADVISED_WRITELN_BUFSIZE           8192


/* flags values */
#define BUFIO_F_APPEND                          0x0001
#define BUFIO_F_IS_LOGFILE                      0x0002
#define BUFIO_F_IS_LOCKED                       0x0004

#define BUFIO_F_WRBUF_DIRTY                     0x0100
#define BUFIO_F_DELAY_WRITE                     0x0200


#if 0
#define bufio_sslread(d, b, l, s) _bufio_read(d, b, l)
#define bufio_sslnbfread(d, b, l, s) _bufio_nbfread(d, b, l)
#define bufio_sslreadln(d, b, l, s) _bufio_readln(d, b, l)
#define bufio_sslwrite(d, b, l, s) _bufio_write(d, b, l)
#endif

#define bufio_getfd(s) ((s)->fd)
#define bufio_is_open(s) ((s) && (bufio_getfd(s) >= 0))
#define bufio_is_sock(s) ((s) && ((s)->is_sock))
#define bufio_get_ssl_hook_data(s) ((s)->ssl_hook_data)



/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#include "bufio.h"
#include "tools.h"
#include "absio.h"
#include "myssl.h"


#define BUFIO_BUF_SIZE                  TL_MAX(4000, BUFIO_ADVISED_READLN_BUFSIZE)
#define BUFIO_LOG_WRITEBUFSIZE          TL_MAX(10000, BUFIO_ADVISED_READLN_BUFSIZE)




extern bufio *bufio_new(bool_t with_mutex, int requested_bufsize);
extern void bufio_free(bufio * desc);

extern bufio *bufio_sock_fdopen(int fd);
extern bufio *bufio_fdopen(int fd);
extern int bufio_open2spec(bufio * fd, int flags, int mask);
extern bufio *bufio_open(const char *filename, int flags);
extern bufio *bufio_copen(const char *filename, int flags, int mask);

extern bufio *bufio_new_sslcon(bufio * sock, void *con);

extern int bufio_close(bufio * desc);
extern int bufio_flush(bufio * desc);
extern int bufio_truncate(bufio * desc, off_t len);
extern off_t bufio_seek(bufio * desc, off_t pos, int mode);

extern int bufio_set_flags(bufio * desc, int flags, int mask);
extern int bufio_set_minbuf(bufio * desc, int size);
extern int bufio_init_mutex(bufio * desc, bool_t has_a_mutex);

extern int bufio_flock(bufio * desc, bool_t lock_mode);
extern int bufio_funlock(bufio * desc);

extern int bufio_lock(bufio * desc, bool_t non_blocking, bufio ** nonlocking_dst_desc);
extern int bufio_release(bufio * desc, bufio ** nonlocking_dst_desc);

extern int bufio_printf(bufio * desc, const char *msg, ...);
extern int bufio_vprintf(bufio * desc, const char *msg, va_list args);
extern int bufio_puts(bufio * desc, const char *str);
extern int bufio_puts_m(bufio * desc, const char *str, ...);
extern int bufio_asciidump(bufio * desc, const char *cbuf, int csize, const char *title);
extern int bufio_write(bufio * desc, const char *buf, size_t size);
extern int bufio_write_m(bufio * desc, const char *buf, size_t size, ...);
extern int bufio_read(bufio * desc, char *buf, size_t size);
extern int bufio_nbfread(bufio * desc, char *buf, size_t size);
extern int bufio_readln(bufio * desc, char *buf, size_t size);
extern void bufio_unread(bufio * desc, char *buf, size_t size);

extern void bufio_reset(bufio * desc);


#endif
