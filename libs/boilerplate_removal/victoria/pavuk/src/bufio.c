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


/* WARNING: must be a power of two! */
#define BUFIO_ALLOC_SECTORSIZE          512


#define BUFIO_ROUND2ALLOC_SECTORSIZE(s)                         \
        (((BUFIO_ALLOC_SECTORSIZE - 1) + (s)) & ~(BUFIO_ALLOC_SECTORSIZE - 1))

#define T()                     /* printf("bufio @ %d\n", __LINE__) */


static int _bufio_flush(bufio * desc);
static int _bufio_write(bufio * desc, const char *buf, size_t size);

static int _bufio_tl_write(bufio * desc, char *buf, size_t size)
{
  int retv;
  T();
  if(desc->is_sock)
  {
    retv = tl_sock_write(bufio_getfd(desc), buf, size, bufio_get_ssl_hook_data(desc));
  }
  else
  {
    retv = tl_file_write(bufio_getfd(desc), buf, size);
  }
  T();
  return retv;
}

static int _bufio_tl_read(bufio * desc, char *buf, size_t size)
{
  int retv;
  T();
  if(desc->is_sock)
  {
    retv = tl_sock_read(bufio_getfd(desc), buf, size, bufio_get_ssl_hook_data(desc));
  }
  else
  {
    retv = tl_file_read(bufio_getfd(desc), buf, size);
  }
  T();
  return retv;
}

/*
   will NOT [un?]write buffer content to channel.

   WILL reallocate the buffer to make sure 'leave_space' bytes
   are available, at least if you flush the buffer.

   NEGATIVE leave_space is a special hack: it means: leave space
   BEFORE the current fill, instead of after (following) it.

   returns !0 if you'll need a bufio_flush to get what you
   want (leave_space)
 */
static int _bufio_fixup_buf(bufio * desc, int shift_content, int leave_space)
{
  int retv = 0;
  T();
  ASSERT(desc);
  ASSERT(desc->buf_start <= desc->buf_end);
  ASSERT(desc->buf_start <= desc->buf_size);
  ASSERT(desc->buf_end <= desc->buf_size);
  if(desc->buf_start > 0)
  {
    if(desc->buf_start == desc->buf_end)
    {
      desc->buf_start = 0;
      desc->buf_end = 0;
    }
    else if(shift_content > 0 && desc->buf_start + shift_content > desc->buf_end)
    {
      /* move data if it's less than 'shift_content' remaining bytes anyway */
      ASSERT(desc->buf_start < desc->buf_end);
      memmove(desc->buf + shift_content, desc->buf + desc->buf_start, desc->buf_end - desc->buf_start);
      desc->buf_end -= desc->buf_start - shift_content;
      desc->buf_start = shift_content;
      ASSERT(desc->buf_start < desc->buf_end);
      ASSERT(desc->buf_start < desc->buf_size);
      ASSERT(desc->buf_end <= desc->buf_size);
    }
    else if(leave_space > 0 && (desc->buf_end + leave_space > desc->buf_size || leave_space > desc->buf_size))
    {
      /* move data if we need at least 'leave_space' bytes of extra space for our subsequent actions */
      ASSERT(desc->buf_start < desc->buf_end);
      memmove(desc->buf, desc->buf + desc->buf_start, desc->buf_end - desc->buf_start);
      desc->buf_end -= desc->buf_start;
      desc->buf_start = 0;
      ASSERT(desc->buf_start < desc->buf_end);
      ASSERT(desc->buf_start < desc->buf_size);
      ASSERT(desc->buf_end <= desc->buf_size);
    }
    else if(leave_space < 0 && desc->buf_start < -leave_space)
    {
      ASSERT(desc->buf_start < desc->buf_end);
      if(leave_space + desc->buf_end - desc->buf_start <= desc->buf_size)
      {
        /* move data if we need at least 'leave_space' bytes of extra space BEFORE */
        memmove(desc->buf + leave_space, desc->buf + desc->buf_start, desc->buf_end - desc->buf_start);
        desc->buf_end -= desc->buf_start + leave_space;
        desc->buf_start = -leave_space;
        leave_space = 0;
        ASSERT(desc->buf_start < desc->buf_end);
        ASSERT(desc->buf_start < desc->buf_size);
        ASSERT(desc->buf_end <= desc->buf_size);
      }
      else
      {
        /* move data if we need at least 'leave_space' bytes of extra space BEFORE */
        ASSERT(leave_space + desc->buf_end - desc->buf_start > desc->buf_size);
        desc->buf = _realloc(desc->buf, (desc->buf_size = BUFIO_ROUND2ALLOC_SECTORSIZE(desc->buf_end - desc->buf_start - leave_space)));
        memmove(desc->buf + leave_space, desc->buf + desc->buf_start, desc->buf_end - desc->buf_start);
        desc->buf_end -= desc->buf_start + leave_space;
        desc->buf_start = -leave_space;
        leave_space = 0;
        ASSERT(desc->buf_start < desc->buf_end);
        ASSERT(desc->buf_start < desc->buf_size);
        ASSERT(desc->buf_end <= desc->buf_size);
      }
    }
  }
  /* make sure there's EXACTLY enough space BEFORE the content: 'shift_content' */
  if(shift_content + desc->buf_end - desc->buf_start > desc->buf_size)
  {
    ASSERT(desc->buf_start <= desc->buf_end);
    desc->buf = _realloc(desc->buf, (desc->buf_size = BUFIO_ROUND2ALLOC_SECTORSIZE(shift_content + desc->buf_end - desc->buf_start)));
    ASSERT(desc->buf_start <= desc->buf_end);
    ASSERT(desc->buf_start <= desc->buf_size);
    ASSERT(desc->buf_end <= desc->buf_size);
    if(desc->buf_start != shift_content && desc->buf_start != desc->buf_end)
    {
      ASSERT(desc->buf_start < desc->buf_end);
      memmove(desc->buf + shift_content, desc->buf + desc->buf_start, desc->buf_end - desc->buf_start);
      desc->buf_end -= desc->buf_start - shift_content;
      desc->buf_start = shift_content;
      ASSERT(desc->buf_start < desc->buf_end);
      ASSERT(desc->buf_start < desc->buf_size);
      ASSERT(desc->buf_end <= desc->buf_size);
    }
    else if(desc->buf_start == desc->buf_end)
    {
      desc->buf_end = desc->buf_start = shift_content;
      ASSERT(desc->buf_end <= desc->buf_size);
    }
  }
  if(desc->flags & BUFIO_F_DELAY_WRITE)
  {
    /* write mode: only allocate if we cannot get the amount by flushing the buffer */
    if(leave_space > desc->buf_size)
    {
      ASSERT(desc->buf_start == 0);
      desc->buf = _realloc(desc->buf, (desc->buf_size = BUFIO_ROUND2ALLOC_SECTORSIZE(leave_space)));
      ASSERT(desc->buf_start <= desc->buf_end);
      ASSERT(desc->buf_start <= desc->buf_size);
      ASSERT(desc->buf_end <= desc->buf_size);
    }
    /* all bytes previously written to disc? */
    ASSERT(desc->buf_end >= desc->buf_start);
    if(desc->buf_end == desc->buf_start)
    {
      desc->flags &= ~BUFIO_F_WRBUF_DIRTY;
    }
  }
  else
  {
    /* read mode: always grow the buffer to make everything fit. */
    ASSERT(!(desc->flags & ~BUFIO_F_WRBUF_DIRTY));
    if(desc->buf_end + leave_space > desc->buf_size)
    {
      ASSERT(desc->buf_start == 0);
      desc->buf = _realloc(desc->buf, (desc->buf_size = BUFIO_ROUND2ALLOC_SECTORSIZE(desc->buf_end + leave_space)));
      ASSERT(desc->buf_start <= desc->buf_end);
      ASSERT(desc->buf_start <= desc->buf_size);
      ASSERT(desc->buf_end <= desc->buf_size);
    }
  }
  retv = (desc->buf_end + leave_space > desc->buf_size);
  T();
  return retv;
}

static int _bufio_flock(bufio * desc, bool_t lock_mode)
{
  int retv;

  T();
  ASSERT(desc);
  ASSERT(bufio_getfd(desc) >= 0);
  retv = _flock(bufio_getfd(desc), desc->filename, desc->open_flags, lock_mode);
  T();
  return retv;
}

static int _bufio_funlock(bufio * desc)
{
  int retv;

  T();
  ASSERT(desc);
  ASSERT(bufio_getfd(desc) >= 0);
  _bufio_flush(desc);
  retv = _funlock(bufio_getfd(desc));
  T();
  return retv;
}

static void _bufio_mutex_lock(bufio * desc)
{
  ASSERT(desc);
  T();
#ifdef HAVE_MT
  if(desc->mutex)
  {
    pthread_mutex_lock(desc->mutex);
  }
#endif
}

static void _bufio_mutex_unlock(bufio * desc)
{
  ASSERT(desc);
  T();
#ifdef HAVE_MT
  if(desc->mutex)
  {
    pthread_mutex_unlock(desc->mutex);
  }
#endif
}

static int _bufio_set_minbuf(bufio * desc, int size)
{
  T();
  ASSERT(desc);
  size = ((desc->flags & BUFIO_F_IS_LOGFILE) ? TL_MAX(desc->buf_size, BUFIO_LOG_WRITEBUFSIZE) : TL_MAX(desc->buf_size, BUFIO_BUF_SIZE));

  if(size > desc->buf_size || !desc->buf)
  {
    ASSERT(size > 0);
    if(!desc->buf)
    {
      desc->buf = _malloc(BUFIO_ROUND2ALLOC_SECTORSIZE(size));
    }
    else
    {
      desc->buf = _realloc(desc->buf, BUFIO_ROUND2ALLOC_SECTORSIZE(size));
    }
    desc->buf_size = size;
  }
  T();
  return desc->buf_size;
}

static void _bufio_reset(bufio * desc)
{
  T();
  ASSERT(desc);
  desc->buf_start = 0;
  desc->buf_end = 0;
  desc->flags &= ~BUFIO_F_WRBUF_DIRTY;
}

static int _bufio_truncate(bufio * desc, off_t len)
{
  int retv;
  T();
  ASSERT(desc);
  ASSERT(bufio_is_open(desc));
  _bufio_flush(desc);
  retv = ftruncate(bufio_getfd(desc), len);
  _bufio_reset(desc);
  T();
  return retv;
}

static int _bufio_set_flags(bufio * desc, int flags, int mask)
{
  int retv;
  T();
  ASSERT(desc);
  T();
  desc->flags &= ~mask;
  desc->flags |= flags;
  retv = desc->flags;
  T();
  return retv;
}


/*
 FIXME: interface of this function is a mess. Really.
 */
static int _bufio_open2spec(bufio * desc, int open_flags, int open_mask)
{
  int retv;

  T();
  ASSERT(desc);
  DEBUG_BUFIO("open2spec(%d, %s)\n", desc->fd, mk_native(desc->filename));
  if(!bufio_is_open(desc))
  {
    ASSERT(desc->fd < 0);
    if(desc->flags & BUFIO_F_APPEND)
    {
      open_flags &= ~O_TRUNC;
      open_flags |= O_APPEND;
    }
    else
    {
      open_flags &= ~O_APPEND;
      open_flags |= O_TRUNC;
    }
    if(!open_mask)
    {
      desc->fd = tl_open(desc->filename, open_flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    }
    else
    {
      desc->fd = tl_open(desc->filename, open_flags, open_mask);
    }
    desc->open_flags = open_flags;
    if(desc->fd < 0)
    {
      DEBUG_BUFIO("open2spec(%d, %s) --> %d/%s\n", desc->fd, mk_native(desc->filename), errno, strerror(errno));
      desc->bufio_errno = errno;
    }
  }

  retv = !bufio_is_open(desc);
  if(!retv)
  {
    if(desc->open_flags & (O_WRONLY | O_RDWR))
    {
      _bufio_set_flags(desc, BUFIO_F_DELAY_WRITE, 0);
    }
    _bufio_set_minbuf(desc, desc->buf_size);

    /* retv->fd = fd; */
    /* retv->filename = tl_strdup(filename); */
    desc->flags &= ~BUFIO_F_APPEND;
    desc->flags |= ((open_flags & O_APPEND) ? BUFIO_F_APPEND : 0);
    if(open_flags & O_TRUNC)
    {
      _bufio_truncate(desc, 0);
    }
  }
  T();
  return retv;
}


static int _bufio_flush(bufio * desc)
{
  int l = 0;
  int retv = 0;

  T();
  if(desc->flags & BUFIO_F_WRBUF_DIRTY)
  {
    /* flush content, even under the worst conditions */
    bool_t st = cfg.stop;
    bool_t br = cfg.rbreak;
    cfg.stop = FALSE;
    cfg.rbreak = FALSE;
    ASSERT(desc->flags & BUFIO_F_DELAY_WRITE);
    l = desc->buf_end - desc->buf_start;
    ASSERT(l >= 0);
    while(l > 0 && !cfg.stop && !cfg.rbreak)
    {
      int r = 1;
      T();
#if defined(__CYGWIN__) || defined(WIN32) || defined(__WIN32)
      if(desc->is_sock)         /* [i_a] */
#endif
      {
        r = tl_selectw(desc->fd, (int) cfg.write_timeout);
      }
      if(r > 0)
      {
        r = _bufio_tl_write(desc, desc->buf + desc->buf_start, l);
        T();
        if(r > 0)
        {
          desc->buf_start += r;
          ASSERT(desc->buf_start <= desc->buf_end);
          ASSERT(desc->buf_start <= desc->buf_size);
          ASSERT(desc->buf_end <= desc->buf_size);
          l -= r;
        }
        else
        {
          /* failed to write any byte */
          desc->bufio_errno = errno;
          retv = -1;
          break;
        }
      }
      else
      {
        /* select() timeout or error */
        desc->bufio_errno = errno;
        if(!desc->bufio_errno)
        {
          errno = desc->bufio_errno = ETIMEDOUT;
        }
        retv = -1;
        break;
      }
    }
    cfg.stop = st;
    cfg.rbreak = br;
    ASSERT(desc->flags & BUFIO_F_WRBUF_DIRTY);
    if(_bufio_fixup_buf(desc, 0, 0))
    {
      _bufio_fixup_buf(desc, 0, 0);
      if(!retv)
      {
        ASSERT(0);
        errno = EIO;
        desc->bufio_errno = EIO;
        retv = -1;
      }
    }
    ASSERT(!retv ? !(desc->flags & BUFIO_F_WRBUF_DIRTY) : TRUE);
  }
  else
  {
    /* ASSERT(desc->buf_end == desc->buf_start); */
  }
  T();
  return retv;
}


static off_t _bufio_seek(bufio * desc, off_t pos, int mode)
{
  off_t retv;
  T();
  ASSERT(desc);
  ASSERT(desc->fd >= 0);
  _bufio_flush(desc);
#if defined(HAVE_LSEEK64)
  retv = lseek64(bufio_getfd(desc), pos, mode);
#elif defined(HAVE_LLSEEK)
  retv = llseek(bufio_getfd(desc), pos, mode);
#else
  retv = lseek(bufio_getfd(desc), pos, mode);
#endif
  T();
  return retv;
}

static int _bufio_vprintf(bufio * desc, const char *msg, va_list args)
{
  int n;
  char *dst;
  T();
  ASSERT(desc);
  ASSERT(desc->fd >= 0);
  ASSERT(desc->buf);
  if(desc->flags & BUFIO_F_DELAY_WRITE)
  {
    n = desc->buf_size - desc->buf_end;
    if(n < TL_MIN(desc->buf_size, BUFIO_ADVISED_WRITELN_BUFSIZE))
    {
      if(_bufio_fixup_buf(desc, 0, TL_MIN(desc->buf_size, BUFIO_ADVISED_WRITELN_BUFSIZE)))
      {
        if(_bufio_flush(desc))
        {
          ASSERT(desc->bufio_errno);
        }
      }
      n = desc->buf_size - desc->buf_end;
    }
    if(n > 1)
    {
      dst = desc->buf + desc->buf_end;
#ifdef HAVE_VSNPRINTF
      vsnprintf(dst, n, msg, args);
#elif defined(HAVE__VSNPRINTF)
      _vsnprintf(dst, n, msg, args);
#else
#error you must implement _snprintf() for your platform
#endif
      dst[n - 1] = 0;
      n = strlen(dst);
      desc->buf_end += n;
      desc->flags |= BUFIO_F_WRBUF_DIRTY;
    }
    else
    {
      n = 0;
    }
    ASSERT(desc->buf_start <= desc->buf_end);
    ASSERT(desc->buf_start <= desc->buf_size);
    ASSERT(desc->buf_end <= desc->buf_size);
  }
  else
  {
    n = 0;
    ASSERT(0);
  }
  T();
  return n;
}



/*
   ===========================================================================
 */

bufio *bufio_new(bool_t with_mutex, int requested_bufsize)
{
  bufio *retv;

  T();
  retv = _malloc(sizeof(*retv));
  retv->buf_size = (requested_bufsize <= 0 ? BUFIO_BUF_SIZE : requested_bufsize);
  retv->buf = _malloc(retv->buf_size);
  retv->fd = -1;
  retv->open_flags = 0;
  retv->buf_start = 0;
  retv->buf_end = 0;
  retv->bufio_errno = 0;
  retv->eof = 0;
  retv->is_sock = 0;
  retv->ssl_hook_data = NULL;
  retv->no_close = FALSE;
  retv->mutex = NULL;
  retv->filename = NULL;
  retv->flags = 0;
#ifdef HAVE_MT
  if(with_mutex)
  {
    retv->mutex = _malloc(sizeof(retv->mutex[0]));
    pthread_mutex_init(retv->mutex, NULL);
  }
#endif
  T();

  return retv;
}

int bufio_set_minbuf(bufio * desc, int size)
{
  int retv;

  T();
  ASSERT(desc);
  T();
  _bufio_mutex_lock(desc);
  T();
  retv = _bufio_set_minbuf(desc, size);
  T();
  _bufio_mutex_unlock(desc);
  T();
  return retv;
}

/*
   Return a fully initialized bufio structure for the given handle.

   Return NULL if the handle is invalid.
 */
bufio *bufio_sock_fdopen(int fd)
{
  bufio *retv;

  T();
  if (fd < 0)
	  return NULL;

  retv = bufio_new(FALSE, 0);
  retv->fd = fd;
  if(retv)
  {
    retv->is_sock = 1;
    ASSERT(!(retv->flags & BUFIO_F_DELAY_WRITE));
  }
  T();
  return retv;
}

bufio *bufio_fdopen(int fd)
{
  bufio *retv;

  if(fd < 0)
    return NULL;

  T();
  retv = bufio_new(FALSE, 0);
  retv->fd = fd;
  /* TBD: should set retv->open_flags too, but we don't know 'em: O_RDWR maybe? */
  T();

  return retv;
}

/*
 FIXME: interface of this function is a mess. Really.
 */
int bufio_open2spec(bufio * desc, int open_flags, int open_mask)
{
  int retv;

  T();
  ASSERT(desc);
  T();
  _bufio_mutex_lock(desc);
  T();
  retv = _bufio_open2spec(desc, open_flags, open_mask);
  T();
  _bufio_mutex_unlock(desc);
  T();
  return retv;
}


bufio *bufio_open(const char *filename, int flags)
{
  bufio *desc;

  T();
  DEBUG_BUFIO("open(%s)\n", filename);
  desc = bufio_new(FALSE, 0);
  desc->filename = tl_strdup(filename);
  desc->open_flags = flags;
  desc->flags |= ((flags | O_APPEND) ? BUFIO_F_APPEND : 0);
  if(_bufio_open2spec(desc, flags, 0))
  {
    bufio_close(desc);
    desc = NULL;
  }
  T();
  return desc;
}

bufio *bufio_copen(const char *filename, int flags, int mask)
{
  bufio *desc;

  T();
  desc = bufio_new(FALSE, 0);
  desc->filename = tl_strdup(filename);
  desc->open_flags = flags;
  desc->flags |= ((flags & O_APPEND) ? BUFIO_F_APPEND : 0);
  if(_bufio_open2spec(desc, flags, mask))
  {
    bufio_close(desc);
    desc = NULL;
  }
  T();
  return desc;
}

bufio *bufio_new_sslcon(bufio * sock, void *con)
{
  bufio *desc;

  T();
  if (!sock)
	  return NULL;
  desc = bufio_new(!!sock->mutex, sock->buf_size);
  T();
  _bufio_mutex_lock(desc);
  T();
  desc->ssl_hook_data = con;
  desc->is_sock = 1;
  ASSERT(!(desc->flags & BUFIO_F_DELAY_WRITE));
  desc->fd = bufio_getfd(sock);
  /*
     FIXME
     WARNING: this copying of file handles is DANGEROUS!
     I'm trying to fix it here, after looking at the usage of this function:
     NOTE that I 'dont_close' flag the ORIGINAL bufio, instead of the newly
     generated one as you'd normally expect.
   */
  sock->no_close = TRUE;
  T();
  _bufio_mutex_unlock(desc);
  T();

  return desc;
}

void bufio_free(bufio * desc)
{
  T();
  if (!desc)
	  return;
  ASSERT(!desc->bufio_errno ? !(desc->flags & BUFIO_F_WRBUF_DIRTY) : TRUE);
#ifdef HAVE_MT
  if(desc->mutex && pthread_mutex_destroy(desc->mutex))
  {
    xperror("mt_semaphore_destroy - bufio_free");
  }
  _free(desc->mutex);
#else
  ASSERT(!desc->mutex);
#endif
  _free(desc->filename);
  _free(desc->buf);
  _free(desc);
  T();
}

int bufio_close(bufio * desc)
{
  int retv = 0;

  T();
  if (!desc)
	  return 0;
  _bufio_mutex_lock(desc);
  T();
  if(desc->flags & BUFIO_F_WRBUF_DIRTY)
  {
    ASSERT((desc->flags & BUFIO_F_WRBUF_DIRTY));
    ASSERT(desc->flags & BUFIO_F_DELAY_WRITE);
    if(_bufio_flush(desc))
    {
      ASSERT(desc->bufio_errno);
    }
    else
    {
      ASSERT(!(desc->flags & BUFIO_F_WRBUF_DIRTY));
    }
  }
  ASSERT(!desc->bufio_errno ? !(desc->flags & BUFIO_F_WRBUF_DIRTY) : TRUE);

#ifdef USE_SSL
  if(bufio_get_ssl_hook_data(desc))
  {
    my_ssl_connection_destroy(bufio_get_ssl_hook_data(desc));
    desc->ssl_hook_data = NULL;
    desc->fd = -1;
  }
#endif

  if(desc->fd >= 0 && !desc->no_close)
  {
    if(desc->is_sock)
    {
      shutdown(desc->fd, SHUT_RDWR);
#ifdef __BEOS__
      retv = closesocket(desc->fd);
#elif defined(WIN32) || defined(__WIN32)
      retv = closesocket(desc->fd);
#else
      retv = close(desc->fd);
#endif
    }
    else
    {
      retv = close(desc->fd);
    }
  }

  desc->fd = -1;
  T();
  _bufio_mutex_unlock(desc);
  T();

  bufio_free(desc);
  T();
  return retv;
}

int bufio_flush(bufio * desc)
{
  int retv;

  T();
  _bufio_mutex_lock(desc);
  T();
  retv = _bufio_flush(desc);
  T();
  _bufio_mutex_unlock(desc);
  T();
  return retv;
}


int bufio_truncate(bufio * desc, off_t len)
{
  int retv;
  T();
  ASSERT(desc);
  ASSERT(bufio_is_open(desc));
  T();
  _bufio_mutex_lock(desc);
  T();
  retv = _bufio_truncate(desc, len);
  T();
  _bufio_mutex_unlock(desc);
  T();
  return retv;
}

int bufio_init_mutex(bufio * desc, bool_t has_a_mutex)
{
  int retv = 0;
  T();
  ASSERT(desc);
#ifdef HAVE_MT
  if(!has_a_mutex)
  {
    if(desc->mutex)
    {
      if((retv = pthread_mutex_destroy(desc->mutex)) != 0)
      {
        xperror("mt_semaphore_destroy - bufio_init_mutex");
      }
      _free(desc->mutex);
      ASSERT(!desc->mutex);
    }
  }
  else
  {
    if(!desc->mutex)
    {
      desc->mutex = _malloc(sizeof(desc->mutex[0]));
      retv = pthread_mutex_init(desc->mutex, NULL);
    }
  }
#endif
  T();
  return retv;
}

int bufio_set_flags(bufio * desc, int flags, int mask)
{
  int retv;
  T();
  ASSERT(desc);
  T();
  _bufio_mutex_lock(desc);
  T();
  retv = _bufio_set_flags(desc, flags, mask);
  T();
  _bufio_mutex_unlock(desc);
  T();
  return retv;
}

int bufio_flock(bufio * desc, bool_t lock_mode)
{
  int retv;

  T();
  ASSERT(desc);
  ASSERT(desc->fd >= 0);
  T();
  _bufio_mutex_lock(desc);
  T();
  retv = _bufio_flock(desc, lock_mode);
  T();
  _bufio_mutex_unlock(desc);
  T();
  return retv;
}

int bufio_funlock(bufio * desc)
{
  int retv;

  T();
  ASSERT(desc);
  ASSERT(desc->fd >= 0);
  T();
  _bufio_mutex_lock(desc);
  T();
  _bufio_flush(desc);
  retv = _bufio_funlock(desc);
  T();
  _bufio_mutex_unlock(desc);
  T();
  return retv;
}

/*
   lock using mutex;
   use _flock() ONLY once we have to write/flush the buffer to disc!

   return !0 if non_blocking detected a previous lock.

   When succesful, *nonblocking_dst_desc will have been set to a
   image copy of your IO descriptor 'desc', but with the mutex temporarily DISabled
   until you 'recover' by calling bufio_release(). See the notes below:

   Note to prevent deadlocks due to using calls like bufio_puts() et al
   within a bufio_lock() ... bufio_release() section, bufio_lock() provides you
   with a direct copy of the IO descriptor 'desc', but without any
   mutex in there, so bufio_puts() will not try to lock you out.

   Note however that you MUST use bufio_release() to 'recover' your original
   IO descriptor 'desc' to the proper state!
 */
int bufio_lock(bufio * desc, bool_t non_blocking, bufio ** nonlocking_dst_desc)
{
  int retv = 0;

  if(!nonlocking_dst_desc)
  {
    desc->bufio_errno = EINVAL;
    return -1;
  }
  *nonlocking_dst_desc = NULL;

  T();
  ASSERT(desc);
#if 0
  ASSERT(desc->fd >= 0);
#endif
#ifdef HAVE_MT
  if(desc->mutex)
  {
    if(non_blocking)
    {
      retv = pthread_mutex_trylock(desc->mutex);
    }
    else
    {
      retv = pthread_mutex_lock(desc->mutex);
    }
  }
#endif
  T();
  if(!retv)
  {
    nonlocking_dst_desc[0] = _malloc(sizeof(nonlocking_dst_desc[0][0]));
    memcpy(nonlocking_dst_desc[0], desc, sizeof(nonlocking_dst_desc[0][0]));    /* copy values across */
#ifdef HAVE_MT
    nonlocking_dst_desc[0]->mutex = NULL;       /* but ditch the mutex for now... */
#endif
  }
  return retv;
}

/*
   Note: a call to this function ASSUMES you had a previous SUCCESFUL call
   to bufio_lock().

   It also ASSUMES you have only really _used_ the 'nonlocking_desc'
   descriptor since then, as it will OVERWRITE any changes you made to the
   'desc' descriptor due to using it in other bufio_* IO operations.
 */
int bufio_release(bufio * desc, bufio ** nonlocking_desc)
{
  int retv = 0;

  T();
  ASSERT(desc);
  if(!nonlocking_desc)
  {
    desc->bufio_errno = EINVAL;
    return -1;
  }
  if(*nonlocking_desc)
  {
#ifdef HAVE_MT
    pthread_mutex_t *mutex = desc->mutex;       /* remember the mutex! */
#endif
    memcpy(desc, nonlocking_desc[0], sizeof(bufio));    /* copy values BACK! */
#ifdef HAVE_MT
    /* but restore the original mutex! */
    desc->mutex = mutex;
#endif
    _free(nonlocking_desc[0]);
    ASSERT(nonlocking_desc[0] == NULL);
  }
#if 0
  ASSERT(desc->fd >= 0);
#endif
#ifdef HAVE_MT
  if(desc->mutex)
  {
    retv = pthread_mutex_unlock(desc->mutex);
  }
#endif
  T();
  return retv;
}

off_t bufio_seek(bufio * desc, off_t pos, int mode)
{
  off_t retv;
  T();
  ASSERT(desc);
  ASSERT(desc->fd >= 0);
  T();
  _bufio_mutex_lock(desc);
  T();
  retv = _bufio_seek(desc, pos, mode);
  T();
  _bufio_mutex_unlock(desc);
  T();
  return retv;
}

int bufio_printf(bufio * desc, const char *msg, ...)
{
  int retv;
  va_list args;
  T();
  va_start(args, msg);
  ASSERT(desc);
  ASSERT(desc->fd >= 0);
  ASSERT(desc->buf);
  T();
  _bufio_mutex_lock(desc);
  T();
  retv = _bufio_vprintf(desc, msg, args);
  T();
  _bufio_mutex_unlock(desc);
  T();
  va_end(args);
  T();
  return retv;
}


int bufio_vprintf(bufio * desc, const char *msg, va_list args)
{
  int retv;
  T();
  ASSERT(desc);
  ASSERT(desc->fd >= 0);
  ASSERT(desc->buf);
  T();
  _bufio_mutex_lock(desc);
  T();
  retv = _bufio_vprintf(desc, msg, args);
  T();
  _bufio_mutex_unlock(desc);
  T();
  return retv;
}

int bufio_puts(bufio * desc, const char *str)
{
  T();
  return bufio_write(desc, str, strlen(str));
}

int bufio_puts_m(bufio * desc, const char *str, ...)
{
  int retv;

  va_list args;
  va_start(args, str);

  _bufio_mutex_lock(desc);

  retv = _bufio_write(desc, str, strlen(str));
  for(;;)
  {
    str = va_arg(args, const char *);
    if(str == NULL)
      break;
    retv += _bufio_write(desc, str, strlen(str));
  }

  _bufio_mutex_unlock(desc);

  va_end(args);

  return retv;
}






int bufio_asciidump(bufio * desc, const char *cbuf, int csize, const char *title)
{
  int n;
  char *dst;
  int dstsize = csize * 5 + 81;
  int fx;

  T();
  _bufio_mutex_lock(desc);
  if(title)
  {
    _bufio_write(desc, title, strlen(title));
  }
  if(desc->flags & BUFIO_F_DELAY_WRITE)
  {
    T();
    fx = _bufio_fixup_buf(desc, 0, dstsize);
    if(fx)
    {
      fx = 0;
      if(_bufio_flush(desc))
      {
        ASSERT(desc->bufio_errno);
        fx = 1;
      }
    }
    if(!fx)
    {
      n = desc->buf_size - desc->buf_end;
      if(n < dstsize)
      {
        _bufio_set_minbuf(desc, dstsize + desc->buf_end /* - desc->buf_start */ );
        n = desc->buf_size - desc->buf_end;
        ASSERT(n >= dstsize);
      }
      dst = desc->buf + desc->buf_end;
      tl_asciidump_content(dst, n, cbuf, csize);
      n = strlen(dst);
      desc->buf_end += n;
      desc->flags |= BUFIO_F_WRBUF_DIRTY;
    }
    else
    {
      n = 0;
    }
    ASSERT(desc->buf_start <= desc->buf_end);
    ASSERT(desc->buf_start <= desc->buf_size);
    ASSERT(desc->buf_end <= desc->buf_size);
  }
  else
  {
    n = 0;
    ASSERT(0);
  }
  T();
  _bufio_mutex_unlock(desc);
  T();
  return n;
}


int bufio_write(bufio * desc, const char *buf, size_t size)
{
  _bufio_mutex_lock(desc);

  size = _bufio_write(desc, buf, size);

  _bufio_mutex_unlock(desc);

  return size;
}

/*
   specify a series of byte blocks to write,
   terminate with a NULL pointer.
*/
int bufio_write_m(bufio * desc, const char *buf, size_t size, ...)
{
  va_list args;
  va_start(args, size);

  _bufio_mutex_lock(desc);

  size = _bufio_write(desc, buf, size);

  for(;;)
  {
    size_t s;

    buf = va_arg(args, const char *);
    if(buf == NULL)
      break;
    s = va_arg(args, size_t);
    size += _bufio_write(desc, buf, s);
  }

  _bufio_mutex_unlock(desc);

  va_end(args);
  return size;
}

static int _bufio_write(bufio * desc, const char *buf, size_t size)
{
  int n;
  char *dst;
  int fx;
  T();
  if(desc->flags & BUFIO_F_DELAY_WRITE)
  {
    fx = _bufio_fixup_buf(desc, 0, size);
    if(fx)
    {
      fx = 0;
      if(_bufio_flush(desc))
      {
        ASSERT(desc->bufio_errno);
        fx = 1;
      }
    }
    if(!fx)
    {
      n = desc->buf_size - desc->buf_end;
      if(n < size)
      {
        _bufio_set_minbuf(desc, size + desc->buf_end /* - desc->buf_start */ );
        n = desc->buf_size - desc->buf_end;
        ASSERT(n >= size);
      }
      dst = desc->buf + desc->buf_end;
      memmove(dst, buf, size);
      desc->buf_end += size;
      desc->flags |= BUFIO_F_WRBUF_DIRTY;
    }
    else
    {
      size = 0;
    }
    ASSERT(desc->buf_start <= desc->buf_end);
    ASSERT(desc->buf_start <= desc->buf_size);
    ASSERT(desc->buf_end <= desc->buf_size);
  }
  else
  {
    int l = _bufio_tl_write(desc, (char *) buf, size);
    size = l;
  }
  T();
  return size;
}

int bufio_read(bufio * desc, char *buf, size_t size)
{
  int read_sz = 0;
  int read_act;
  int miss_size = size;
  int acnt;

  T();
  if(desc->eof)
  {
    T();
    return 0;
  }

  T();
  if(desc->bufio_errno)
  {
    T();
    errno = desc->bufio_errno;
    return -1;
  }

  for(;;)
  {
    T();
    if(desc->buf_start < desc->buf_end)
    {
      read_act = miss_size < desc->buf_end - desc->buf_start ? miss_size : desc->buf_end - desc->buf_start;

      memcpy(buf + read_sz, desc->buf + desc->buf_start, read_act);

      desc->buf_start += read_act;
      ASSERT(desc->buf_start <= desc->buf_end);
      ASSERT(desc->buf_start <= desc->buf_size);
      ASSERT(desc->buf_end <= desc->buf_size);
      miss_size -= read_act;
      read_sz += read_act;

      if(read_sz == size)
      {
        T();
        return read_sz;
      }
    }
    else
    {
      desc->buf_start = 0;
      desc->buf_end = 0;
      acnt = _bufio_tl_read(desc, desc->buf, desc->buf_size);
      if(acnt <= 0)
      {
        if(acnt == 0)
          desc->eof = 1;
        if(read_sz)
        {
          desc->bufio_errno = errno;
          T();
          return read_sz;
        }
        else
        {
          T();
          return acnt;
        }
      }

      desc->buf_end = acnt;
      ASSERT(desc->buf_start <= desc->buf_end);
      ASSERT(desc->buf_start <= desc->buf_size);
      ASSERT(desc->buf_end <= desc->buf_size);
    }
  }
  T();
}

int bufio_nbfread(bufio * desc, char *buf, size_t size)
{
  int read_sz;

  T();
  if(desc->eof)
  {
    T();
    return 0;
  }
  if(desc->bufio_errno)
  {
    errno = desc->bufio_errno;
    T();
    return -1;
  }

  if(desc->buf_start < desc->buf_end)
  {
    read_sz = size < desc->buf_end - desc->buf_start ? size : desc->buf_end - desc->buf_start;

    memcpy(buf, desc->buf + desc->buf_start, read_sz);

    desc->buf_start += read_sz;
    ASSERT(desc->buf_start <= desc->buf_end);
    ASSERT(desc->buf_start <= desc->buf_size);
    ASSERT(desc->buf_end <= desc->buf_size);

    T();
    return read_sz;
  }
  else
  {
    desc->buf_start = 0;
    desc->buf_end = 0;
    read_sz = _bufio_tl_read(desc, buf, size);
    if(read_sz <= 0)
    {
      if(read_sz == 0)
        desc->eof = 1;
      if(read_sz < 0)
        desc->bufio_errno = errno;
    }
    T();
    return read_sz;
  }
}

int bufio_readln(bufio * desc, char *buf, size_t size)
{
  int read_sz = 0;
  int read_act;
  int miss_size = size - 1;
  int acnt;
  char *mc = NULL;

  T();
  if(desc->eof)
  {
    T();
    return 0;
  }

  if(desc->bufio_errno)
  {
    T();
    errno = desc->bufio_errno;
    return -1;
  }

  size--;

  for(;;)
  {
    T();
    if(desc->buf_start < desc->buf_end)
    {
      mc = memchr(desc->buf + desc->buf_start, '\n', desc->buf_end - desc->buf_start);

      if(mc && (mc - (desc->buf + desc->buf_start) < miss_size))
      {
        read_act = mc + 1 - (desc->buf + desc->buf_start);
      }
      else
      {
        read_act = miss_size < desc->buf_end - desc->buf_start ? miss_size : desc->buf_end - desc->buf_start;
      }
      memcpy(buf + read_sz, desc->buf + desc->buf_start, read_act);

      desc->buf_start += read_act;
      ASSERT(desc->buf_start <= desc->buf_end);
      ASSERT(desc->buf_start <= desc->buf_size);
      ASSERT(desc->buf_end <= desc->buf_size);
      miss_size -= read_act;
      read_sz += read_act;

      if(read_sz == size || mc)
      {
        *(buf + read_sz) = '\0';
        T();
        return read_sz;
      }
    }
    else
    {
      desc->buf_start = 0;
      desc->buf_end = 0;
      acnt = _bufio_tl_read(desc, desc->buf, desc->buf_size);

      if(acnt <= 0)
      {
        if(acnt == 0)
          desc->eof = 1;
        if(read_sz)
        {
          desc->bufio_errno = errno;
          *(buf + read_sz + 1) = '\0';
          T();
          return read_sz;
        }
        else
        {
          T();
          return acnt;
        }
      }

      desc->buf_end = acnt;
      ASSERT(desc->buf_start <= desc->buf_end);
      ASSERT(desc->buf_start <= desc->buf_size);
      ASSERT(desc->buf_end <= desc->buf_size);
    }
  }
}

void bufio_unread(bufio * desc, char *buf, size_t size)
{
  T();
  VERIFY(!_bufio_fixup_buf(desc, size, 0));
#if 0
  desc->buf_size = TL_MAX(desc->buf_size, (size + desc->buf_end - desc->buf_start));

  /* FIXME: fixup the bufio; no need to reallocate at all times; sometimes there's enough space to shift data forward
     only */

  desc->buf = _malloc(desc->buf_size);
#endif
  ASSERT(desc->buf);
  ASSERT(desc->buf_size >= size);
  ASSERT(desc->buf_start == size);
  ASSERT(desc->buf_start <= desc->buf_end);
  ASSERT(desc->buf_start <= desc->buf_size);
  ASSERT(desc->buf_end <= desc->buf_size);
  memmove(desc->buf, buf, size);
#if 0
  memmove(desc->buf + size, p + desc->buf_start, desc->buf_end - desc->buf_start);
#endif

  ASSERT(desc->buf_end == size + desc->buf_end - desc->buf_start);
  desc->buf_start = 0;
  ASSERT(desc->buf_start <= desc->buf_end);
  ASSERT(desc->buf_start <= desc->buf_size);
  ASSERT(desc->buf_end <= desc->buf_size);
  T();
}

void bufio_reset(bufio * desc)
{
  T();
  ASSERT(desc);
  T();
  _bufio_mutex_lock(desc);
  T();
  _bufio_reset(desc);
  T();
  _bufio_mutex_unlock(desc);
  T();
}
