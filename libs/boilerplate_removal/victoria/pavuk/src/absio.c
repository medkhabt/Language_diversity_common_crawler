/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "absio.h"
#include "myssl.h"
#include "gui_api.h"
#include "net.h"                /* [i_a] */



int tl_selectr(int sock, int timeout)
{
  fd_set readfds, exceptfds;
  struct timeval tout, *toutptr = NULL;
  int rv;

  /* [i_a] */
  FD_ZERO(&readfds);
  FD_ZERO(&exceptfds);

  /*
     FD_SET will crash if you pass it sock = -1
   */
  ASSERT(sock_maybe_in_fdset(&readfds, sock));
  if(!sock_maybe_in_fdset(&readfds, sock))
  {
    char *msg = tl_str_printf(NULL, "(sock = %d)", sock);

    errno = EINVAL;
    xperror1("tl_selectr:FD_SETSIZE ", msg);
    _free(msg);
    return -1;
  }

  FD_SET(sock, &readfds);
  FD_SET(sock, &exceptfds);

  ASSERT(timeout >= 0);
  if(timeout > 0)
  {
    tout.tv_sec = timeout / 1000;
    tout.tv_usec = timeout % 1000;
    toutptr = &tout;
    rv = select(sock + 1, &readfds, NULL, &exceptfds, toutptr);
  }
  else
  {
    /* we could've used pselect() here, but not all systems support it [properly] */
    do
    {
      tout.tv_sec = 1;
      tout.tv_usec = 0;
      toutptr = &tout;
      rv = select(sock + 1, &readfds, NULL, &exceptfds, toutptr);
    }
    while((rv == 0) && !cfg.stop && !cfg.rbreak);
    if(rv == 0)
      rv = -1;
  }
#ifdef HAVE_WINSOCK2_H
  if(rv < 0)
    errno = WinSockErr2errno(WSAGetLastError());
#endif
  return rv;
}

int tl_selectw(int sock, int timeout)
{
  fd_set writefds, exceptfds;
  struct timeval tout, *toutptr = NULL;
  int rv;

  /* [i_a] */
  FD_ZERO(&writefds);
  FD_ZERO(&exceptfds);

  /*
     FD_SET will crash if you pass it sock = -1
   */
  if(!sock_maybe_in_fdset(&writefds, sock))
  {
    char *msg = tl_str_printf(NULL, "(sock = %d)", sock);

    errno = EINVAL;
    xperror1("tl_selectw:FD_SETSIZE ", msg);
    _free(msg);
    return -1;
  }

  FD_SET(sock, &writefds);
  FD_SET(sock, &exceptfds);

  ASSERT(timeout >= 0);
  if(timeout > 0)
  {
    tout.tv_sec = timeout / 1000;
    tout.tv_usec = timeout % 1000;
    toutptr = &tout;
    rv = select(sock + 1, NULL, &writefds, &exceptfds, toutptr);
  }
  else
  {
    /* we could've used pselect() here, but not all systems support it [properly] */
    do
    {
      tout.tv_sec = 1;
      tout.tv_usec = 0;
      toutptr = &tout;
      rv = select(sock + 1, NULL, &writefds, &exceptfds, toutptr);
    }
    while((rv == 0) && !cfg.stop && !cfg.rbreak);
    if(rv == 0)
      rv = -1;
  }
#ifdef HAVE_WINSOCK2_H
  if(rv < 0)
    errno = WinSockErr2errno(WSAGetLastError());
#endif
  return rv;
}

int tl_unix_read(int sock, char *buf, size_t len, void *ssl_con)
{
  int rv = 0;
  int timeout = (int) cfg.read_timeout;
  int rlen = 0;

  do
  {
#ifdef USE_SSL
    if(!ssl_con || (ssl_con && (my_ssl_data_pending(ssl_con) == 0)))
#endif
    {
#if defined I_FACE && !defined HAVE_MT
      if(cfg.xi_face && cfg.done)
      {
        rv = gui_wait_io(sock, TRUE);
      }
      else
#endif
      {
        while((rv = tl_selectr(sock, timeout)) == -1 && errno == EINTR)
        {
          if(cfg.rbreak)
          {
            DEBUG_DEVEL("cfg.stop = TRUE @ %s, line #%d\n", __FILE__, __LINE__);
            errno = EINTR;
            cfg.stop = TRUE;
            return -1;
          }
        }
        if(rv == 0 && timeout > 0)
        {
          errno = ETIMEDOUT;
          return -1;
        }
      }

      if(rv < 0)
        return -1;
    }

    if(cfg.rbreak)
    {
      errno = EINTR;
      DEBUG_DEVEL("cfg.stop = TRUE @ %s, line #%d\n", __FILE__, __LINE__);
      cfg.stop = TRUE;
      return -1;
    }

#ifdef USE_SSL
    if(ssl_con)
    {
      rv = my_ssl_read(ssl_con, buf, len);
    }
    else
#endif
    {
      errno = 0;                /* [i_a] reset errno or the code will sometimes fail */
#if defined __BEOS__
      /* [i_a] BeOS patch changed; VERY similar to Win32 needs BTW */
      rv = recv(sock, buf, (int) len, 0);
#elif defined(WIN32) || defined(__WIN32)
      WSASetLastError(0);
      rv = recv(sock, buf, (int) len, 0);
      errno = WinSockErr2errno(WSAGetLastError());
#else
      rv = read(sock, buf, len);
#endif
      if(!rv || errno)
      {
        if(errno == EAGAIN)
        {
          xprintf(1, "read() EAGAIN\n");
        }
        else if(errno == EWOULDBLOCK)
        {
          xprintf(1, "read() EWOULDBLOCK\n");
        }
        else if(errno != 0)
        {
#if defined(DEBUG) && 0
          DEBUG_DEVEL("I/O read ret = %d/%d, errno = %d:%s\n", rv, len, errno, strerror(errno));
#endif
          return -1;
        }
      }
    }
#if defined(DEBUG) && 0
    DEBUG_DEVEL("I/O read ret = %d/%d/%d, errno = %d:%s\n", rv, len, rlen, errno, strerror(errno));
#endif
    if(rv > 0)
    {
      rlen += rv;
      len -= rv;
      buf += rv;
    }
    /* it's 'good enough' when we've fetched a few bytes */
    if(rv == 0 && rlen > 0)
    {
      break;
    }
  }
  while(rv >= 0 && len >= 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) && !cfg.rbreak);

  return rlen;
}

/* assumes a non-blocking socket is used! */
int tl_unix_nbread(int sock, char *buf, size_t len, void *ssl_con)
{
  int rv = 0;
  int rlen = 0;

#ifdef USE_SSL
  if(ssl_con && (my_ssl_data_pending(ssl_con) == 0))
  {
    return 0;
  }
#endif

  if(cfg.rbreak)
  {
    errno = EINTR;
    DEBUG_DEVEL("cfg.stop = TRUE @ %s, line #%d\n", __FILE__, __LINE__);
    cfg.stop = TRUE;
    return -1;
  }

#ifdef USE_SSL
  if(ssl_con)
  {
    rv = my_ssl_read(ssl_con, buf, len);
  }
  else
#endif
  {
    errno = 0;                  /* [i_a] reset errno or the code will sometimes fail */
#if defined __BEOS__
    /* [i_a] BeOS patch changed; VERY similar to Win32 needs BTW */
    rv = recv(sock, buf, (int) len, 0);
#elif defined(WIN32) || defined(__WIN32)
    WSASetLastError(0);
    rv = recv(sock, buf, (int) len, 0);
    errno = WinSockErr2errno(WSAGetLastError());
#else
    rv = read(sock, buf, len);
#endif
    if(!rv || errno)
    {
      if(errno == EAGAIN)
      {
        DEBUG_NET("read() EAGAIN\n");
        return 0;
      }
      else if(errno == EWOULDBLOCK)
      {
        DEBUG_NET("read() EWOULDBLOCK\n");
        return 0;
      }
      else if(errno != 0)
      {
#if defined(DEBUG) && 0
        DEBUG_DEVEL("I/O nb read ret = %d/%d, errno = %d:%s\n", rv, len, errno, strerror(errno));
#endif
        return -1;
      }
    }
  }
#if defined(DEBUG) && 0
  DEBUG_DEVEL("I/O nb read ret = %d/%d/%d, errno = %d:%s\n", rv, len, rlen, errno, strerror(errno));
#endif
  if(rv > 0)
  {
    rlen += rv;
    len -= rv;
    buf += rv;
  }

  return rlen;
}

int tl_unix_write(int sock, char *buf, size_t len, void *ssl_con)
{
  int rv = 0;
  int wlen = 0;
  int timeout = (int) cfg.write_timeout;

  do
  {
#if defined I_FACE && !defined HAVE_MT
    if(cfg.xi_face && cfg.done)
    {
      rv = gui_wait_io(sock, FALSE);
    }
    else
#endif
    {
      while((rv = tl_selectw(sock, timeout)) == -1 && errno == EINTR)
      {
        if(cfg.rbreak)
        {
          DEBUG_DEVEL("cfg.stop = TRUE @ %s, line #%d\n", __FILE__, __LINE__);
          errno = EINTR;
          cfg.stop = TRUE;
          return -1;
        }
      }
      if(rv == 0 && timeout > 0)
      {
        errno = ETIMEDOUT;
        return -1;
      }
    }

    if(rv < 0)
      return -1;

    if(cfg.rbreak)
    {
      errno = EINTR;
      DEBUG_DEVEL("cfg.stop = TRUE @ %s, line #%d\n", __FILE__, __LINE__);
      cfg.stop = TRUE;
      return -1;
    }

#ifdef USE_SSL
    if(ssl_con)
    {
      rv = my_ssl_write(ssl_con, buf, len);
    }
    else
#endif
    {
      errno = 0;                /* [i_a] reset errno or the code will sometimes fail */
#if defined __BEOS__
      /* [i_a] BeOS patch changed; VERY similar to Win32 needs BTW */
      rv = send(sock, buf, (int) len, 0);
#elif defined(WIN32) || defined(__WIN32)
      WSASetLastError(0);
      rv = send(sock, buf, (int) len, 0);
      errno = WinSockErr2errno(WSAGetLastError());
#else
      rv = write(sock, buf, len);
#endif
      if(errno != 0)
      {
#if defined(DEBUG)
        DEBUG_NET("I/O write: socket: %d, buf: %p, len: %d, errno: %d:%s\n", sock, buf, len, errno, strerror(errno));
        DEBUG_NET("I/O write: -> return: %d / %d, errno: %d:%s\n", rv, len, errno, strerror(errno));
#endif
        return -1;
      }
    }
    if(rv < 0)
    {
        DEBUG_NET("I/O write: -> return rv = %d, len = %d, errno: %d:%s\n", rv, len, errno, strerror(errno));
    }
    if(rv > 0)
    {
      wlen += rv;
      buf += rv;
      len -= rv;
    }
  }
  while(rv >= 0 && len > 0 && !cfg.rbreak);

  return wlen;
}







/* [i_a] BeOS patch changed; VERY similar to Win32 needs BTW */
