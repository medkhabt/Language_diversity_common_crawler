/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#if !defined (__CYGWIN__) && (defined (WIN32) || defined (__WIN32))

int tl_rename(const char *p1, const char *p2)
{
  int rv;

  p1 = cvt_unix_to_win32_path(p1);
  p2 = cvt_unix_to_win32_path(p2);
  rv = rename(p1, p2);
  _free(p1);
  _free(p2);
  return rv;
}

int tl_stat(const char *p, struct stat *s)
{
  int rv;

  p = cvt_unix_to_win32_path(p);
  rv = stat(p, s);
  _free(p);
  return rv;
}

int tl_lstat(const char *p, struct stat *s)
{
  int rv;

  p = cvt_unix_to_win32_path(p);
  rv = lstat(p, s);
  _free(p);
  return rv;
}

int tl_link(const char *p1, const char *p2)
{
  int rv;

  p1 = cvt_unix_to_win32_path(p1);
  p2 = cvt_unix_to_win32_path(p2);
  rv = link(p1, p2);
  _free(p1);
  _free(p2);
  return rv;
}

int tl_symlink(const char *p1, const char *p2)
{
  int rv;

  p1 = cvt_unix_to_win32_path(p1);
  p2 = cvt_unix_to_win32_path(p2);
  rv = symlink(p1, p2);
  _free(p1);
  _free(p2);
  return rv;
}

int tl_unlink(const char *p)
{
  int rv;

  p = cvt_unix_to_win32_path(p);
  rv = unlink(p);
  _free(p);
  return rv;
}

int tl_open(const char *p, int flags, int mode)
{
  int rv;

  p = cvt_unix_to_win32_path(p);
  rv = open(p, flags, mode);
  _free(p);
  return rv;
}

int tl_access(const char *p, int flags)
{
  int rv;

  p = cvt_unix_to_win32_path(p);
  rv = access(p, flags);
  _free(p);
  return rv;
}

int tl_mkdir(const char *p, int rights)
{
  int rv;

  p = cvt_unix_to_win32_path(p);
  rv = mkdir(p, rights);
  _free(p);
  return rv;
}

int tl_rmdir(const char *p)
{
  int rv;

  p = cvt_unix_to_win32_path(p);
  rv = rmdir(p);
  _free(p);
  return rv;
}

DIR *tl_opendir(const char *p)
{
  DIR *rv;

  p = cvt_unix_to_win32_path(p);
  rv = opendir(p);
  _free(p);
  return rv;
}

FILE *tl_fopen(const char *p, const char *mode)
{
  FILE *rv;

  p = cvt_unix_to_win32_path(p);
  rv = fopen(p, mode);
  _free(p);
  return rv;
}



/*
 * Some very special code to display Win32 native format filepaths:
 *
 * As we do not wish to have the caller _free() the resulting string
 * (so they can use it as is as arguments in printf() like statements
 * for instance), we'll need something a bit thread-safe and
 * capable of managing its own memory allocation.
 *
 * This is done by initializing a mem pool with enough space for
 * N paths per thread, thus ASSUMING we'll NEVER require more than
 * N paths simultaneously per thread!
 */
static int path_pool_size = 0;
static char **path_pool = NULL;
static int path_pool_index = 0;
#ifdef HAVE_MT
static pthread_mutex_t *path_pool_mutex = NULL;
#endif

/* non-threadsafe init: */
static void _set_mk_native_pool(int size)
{
  int pos = path_pool_size;

  if (size <= path_pool_size)
    return;

  if (size < 1)
    size = 1;
  size *= 16;                   /* reserve space for 16 paths per thread */

  ASSERT(path_pool_index >= 0);
  ASSERT(path_pool_index <= size);

  if (!path_pool)
  {
    pos = 0;

    path_pool = _malloc(size * sizeof(path_pool[0]));
    path_pool_size = size;
  }
  else if (size > path_pool_size)
  {
    path_pool = _realloc(path_pool, size * sizeof(path_pool[0]));
    path_pool_size = size;
  }
  ASSERT(path_pool_index < size);

  /* zero the new entries in the enlarged pool */
  for ( ; pos < path_pool_size; pos++)
  {
    path_pool[pos] = NULL;
  }
}


/*
 *      A threadsafe wrapper around cvt_unix_to_win32_path()
 *      which allows the use of that function without having
 *      to bother with _free()-ing the return value (string).
 *
 *      This is meant to be used in printf(...) style argument
 *      lists, where it would only complicate the code when
 *      you'd have to keep track of each string argument, just
 *      so it can be _free()d correctly.
 *
 *      Returns the same result string as cvt_unix_to_win32_path() would.
 *
 *      NOTE: This functions works by managing a cyclic buffer (pool) of
 *      cvt_unix_to_win32_path() results, which are _free()d
 *      when the pool cycles over this particular entry again.
 *      This means that the returned string value is safe while
 *      no more than path_pool_size additional calls to this function
 *      have been made.
 *
 *      path_pool_size is dimensioned at program startup to allow
 *      for 3 mk_native() entries per thread, i.e. it is dimensioned
 *      to allow for printf(...) statements which contain, either
 *      directly or indirectly, 3 calls to mk_native().
 *
 *      See also: set_mk_native_pool()
 */
const char *mk_native(const char *path)
{
  int pos;

  if (!path)
    return NULL;

#ifdef HAVE_MT
  if (!path_pool_mutex)
  {
    path_pool_mutex = _malloc(sizeof(path_pool_mutex[0]));
    pthread_mutex_init(path_pool_mutex, NULL);
  }
#endif
#ifdef HAVE_MT
  if (path_pool_mutex)
  {
    pthread_mutex_lock(path_pool_mutex);
  }
#endif
  if (!path_pool)
  {
    /* a fail-safe: you may later re-init (enlarge) the pool anyway */
    _set_mk_native_pool(1);
  }
  pos = path_pool_index++;
  ASSERT(path_pool_index <= path_pool_size);
  if (path_pool_index == path_pool_size)
  {
    path_pool_index = 0;
  }

  _free(path_pool[pos]);
  path_pool[pos] = cvt_unix_to_win32_path(path);

  path = path_pool[pos];


#ifdef HAVE_MT
  if (path_pool_mutex)
  {
    pthread_mutex_unlock(path_pool_mutex);
  }
#endif
  return path;
}


/*
 * threadsafe init:
 *
 * Note: the mutex/preproc code is done this way as - when the mutex doesn't exist yet -
 * it is automagically created INSIDE _set_mk_native_pool()!
 */
void set_mk_native_pool(int threadcount)
{
#ifdef HAVE_MT
  if (path_pool_mutex)
  {
    pthread_mutex_lock(path_pool_mutex);
    _set_mk_native_pool(threadcount * 3);
    pthread_mutex_unlock(path_pool_mutex);
  }
  else
#endif
  _set_mk_native_pool(threadcount * 3);
}

void cleanup_native_pool(void)
{
#ifdef HAVE_MT
  if (path_pool_mutex)
  {
    pthread_mutex_destroy(path_pool_mutex);
    _free(path_pool_mutex);
  }
#endif
  if (path_pool)
  {
    int pos = path_pool_size;

    for ( ; --pos >= 0;)
    {
      _free(path_pool[pos]);
    }
    _free(path_pool);
    path_pool_size = 0;
    path_pool_index = 0;
  }
}

#endif


#ifndef HAVE_SNPRINTF

int snprintf(char *dst, size_t dstlen, const char *msg, ...)
{
  int n;
  va_list ap;

  va_start(ap, msg);
#ifdef HAVE_VSNPRINTF
  n = vsnprintf(dst, dstlen, msg, ap);
#elif defined (HAVE__VSNPRINTF)
  n = _vsnprintf(dst, dstlen, msg, ap);
#else
#error you must implement snprintf() for your platform
#endif
  va_end(ap);
  return n;
}

#endif


#ifndef HAVE_STRCASECMP

int strcasecmp(const char *s1, const char *s2)
{
  return stricmp(s1, s2);
}

#endif


#ifndef HAVE_STRNCASECMP

int strncasecmp(const char *s1, const char *s2, size_t len)
{
  return strnicmp(s1, s2, len);
}

#endif


#ifndef HAVE_LSTAT

int lstat(const char *fname, struct stat *s)
{
  return stat(fname, s);
}

#endif


#ifndef HAVE_GETPID

pid_t getpid(void)
{
#if defined (WIN32) || defined (__WIN32)
  return (pid_t)GetCurrentProcessId();

#else
  return 42;

#endif
}

#endif


#ifndef HAVE_GETTIMEOFDAY
#if defined (WIN32) || defined (_WIN32)

/* adapted from code ripped from Curl mailing list & PostgreSQL sources */

/* FILETIME of Jan 1 1970 00:00:00. */
#define EPOCH    116444736000000000LL

int gettimeofday(struct timeval *tv, struct timezone *tzp)
{
  union
  {
    LONGLONG ns100;             /*time since 1 Jan 1601 in 100ns units */
    FILETIME ft;
  } now;

  if (!tv)
  {
    errno = EINVAL;
    return -1;
  }

#if 0
  {
    SYSTEMTIME system_time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &now.ft);
  }
#else
  GetSystemTimeAsFileTime(&now.ft);
#endif
  tv->tv_usec = (long)((now.ns100 / 10LL) % 1000000LL);
  tv->tv_sec = (long)((now.ns100 - EPOCH) / 10000000LL);

#if 0
  // Get the timezone, if they want it
  if (tzp != NULL)
  {
    _tzset();
    tzp->tz_minuteswest = _timezone;
    tzp->tz_dsttime = _daylight;
  }
#else
  ASSERT(tzp == NULL);          /* shouldn't've been used in pavuk */
#endif
  return 0;
}

#endif
#endif


#if defined (WIN32) || defined (__WIN32)

void set_h_errno(int ecode)
{
  WSASetLastError(ecode);
}

#else

void set_h_errno(int ecode)
{
  _h_errno_ = ecode;
}

#endif

#ifndef HAVE_SYMLINK

int symlink(const char *to_path, const char *from_path)
{
  /*
   * better make sure you port this function if you like to handle FTP'd symlinks at all!
   *
   * Win32 (NTFS on NT2K/XP) supports symlinks (er, I mean: hardlinks), which may help.
   */
  errno = ENOSUP;
  return -1;
}

#endif


#ifndef HAVE_LINK

int link(const char *to_path, const char *from_path)
{
  errno = ENOSUP;
  return -1;
}

#endif


#ifndef HAVE_MKDIR
#ifdef HAVE__MKDIR

int mkdir(const char *path, int rights)
{
  return _mkdir(path);
}

#endif
#endif


#ifndef HAVE_RMDIR
#ifdef HAVE__RMDIR

int rmdir(const char *path)
{
  return _rmdir(path);
}

#endif
#endif


#ifndef HAVE_SLEEP
#if defined (WIN32) || defined (_WIN32)

void sleep(int seconds)
{
  DWORD count = seconds * 1000L;

  SleepEx(count, TRUE);
}

#endif
#endif

#ifndef HAVE_FTRUNCATE
#if defined (WIN32) || defined (_WIN32)

// derived from code at http://www.void.in/wiki/PortableFtruncate
int ftruncate(int filehandle, size_t size)
{
#if defined (HAVE_CHSIZE)
  return chsize(filehandle, size);

#elif defined (HAVE__CHSIZE)
  return _chsize(filehandle, size);

#else /* !HAVE__CHSIZE */
  HANDLE hFile;
  LARGE_INTEGER currentpos, pos, newpos;

  hFile = (HANDLE)_get_osfhandle(filehandle);
  pos.QuadPart = 0;
  if (!SetFilePointerEx(hFile, pos, &currentpos, FILE_CURRENT))
  {
    return -1;
  }
  pos.QuadPart = size;
  if (!SetFilePointerEx(hFile, pos, &newpos, FILE_BEGIN))
  {
    return -1;
  }
  if (!SetEndOfFile(hFile))
  {
    return -1;
  }
  if (!SetFilePointerEx(hFile, currentpos, &newpos, FILE_BEGIN))
  {
    return -1;
  }
  return 0;

#endif /* !HAVE__CHSIZE */
}

#endif
#endif


#ifndef HAVE_GETCWD
#if defined (WIN32) || defined (_WIN32)

char *getcwd(char *buf, int max_len)
{
  DWORD len;

  if (!buf)
  {
    buf = _malloc(max_len + 1);
  }
  len = GetCurrentDirectoryA(max_len, (LPSTR)buf);
  if (len >= (DWORD)max_len || len == 0)
  {
    /* quick hack: no neat error conversion, etc. */
    errno = EINVAL;
    _free(buf);
    ASSERT(buf == NULL);
  }
  _strtrchr(buf, '\\', '/');
  return buf;
}

#endif
#endif


#ifndef HAVE_FLOCK
#if defined (WIN32) || defined (_WIN32)

/*
 * Program:   Unix compatibility routines
 *
 * Author:  Mark Crispin
 *      Networks and Distributed Computing
 *      Computing & Communications
 *      University of Washington
 *      Administration Building, AG-44
 *      Seattle, WA  98195
 *      Internet: MRC@CAC.Washington.EDU
 *
 * Date:    14 September 1996
 * Last Edited: 14 August 1997
 *
 * Copyright 1997 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made available
 * "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */



/*              DEDICATION
 *
 *  This file is dedicated to my dog, Unix, also known as Yun-chan and
 * Unix J. Terwilliker Jehosophat Aloysius Monstrosity Animal Beast.  Unix
 * passed away at the age of 11 1/2 on September 14, 1996, 12:18 PM PDT, after
 * a two-month bout with cirrhosis of the liver.
 *
 *  He was a dear friend, and I miss him terribly.
 *
 *  Lift a leg, Yunie.  Luv ya forever!!!!
 */

/* Note: this code has been augmented by [i_a] Ger Hobbelt, inspired by the PHP5 code. */

int flock(int fd, int op)
{
  HANDLE hdl;
  DWORD low = 1;
  DWORD high = 0;
  OVERLAPPED offset = { 0, 0, 0, 0, NULL };
  DWORD err;

  if (fd < 3)
  {
    errno = EINVAL;
    return -1;
  }
  hdl = (HANDLE)_get_osfhandle(fd);
  if (hdl < 0)
  {
    errno = EINVAL;
    return -1;                                      /* error in file descriptor */
  }


  /* bug for bug compatible with Unix */
  (void)UnlockFileEx(hdl, 0, low, high, &offset);
  /* translate to LockFileEx() op */
  switch (op & ~LOCK_NB)
  {
  case LOCK_EX:                         /* exclusive */
    if (LockFileEx(hdl, LOCKFILE_EXCLUSIVE_LOCK +
                   ((op & LOCK_NB) ? LOCKFILE_FAIL_IMMEDIATELY : 0),
                   0, low, high, &offset))
    {
      return 0;
    }
    break;

  case LOCK_SH:                         /* shared */
    if (LockFileEx(hdl, ((op & LOCK_NB) ? LOCKFILE_FAIL_IMMEDIATELY : 0),
                   0, low, high, &offset))
    {
      return 0;
    }
    break;

  case LOCK_UN:                             /* unlock */
    return 0;                               /* always succeeds */

  default:                                                      /* default */
    errno = EINVAL;                                             /* bad call */
    return -1;
  }

  /*
   * Under Win32 MT library, errno is not a variable but a function call,
   * which cannot be assigned to.
   */
  err = GetLastError();
  switch (err)
  {
  case ERROR_IO_PENDING:
    _set_errno(EAGAIN);
    break;

  case ERROR_FILE_NOT_FOUND:
  case ERROR_PATH_NOT_FOUND:
    _set_errno(ENOENT);
    break;

  case ERROR_ACCESS_DENIED:
    _set_errno(EACCES);
    break;

  default:
    _set_errno(EINVAL);
    break;
  }
  if ((op & LOCK_NB) && (errno == EACCES || errno == EAGAIN))
  {
    _set_errno(EWOULDBLOCK);
  }
  return -1;
}

#endif
#endif

