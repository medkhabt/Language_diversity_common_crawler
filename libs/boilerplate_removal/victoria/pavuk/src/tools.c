/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/


#include "config.h"

#ifdef GTK_FACE
#include <glib.h>
#endif


#include "gui_api.h"
#include "url.h"
#include "doc.h"
#include "ftp.h"
#include "dinfo.h"
#include "log.h"
#include "tools.h"


#define PRINTF_BUFFER_SIZE             16384


/*
 * Remove any trailing CR LF character sequence.
 *
 *
 * CAVEAT:
 *
 * Essentially, this routine CUTS the string at the
 * first CR or LF occurrence, so don't feed this baby any multiline
 * strings! ;-)
 */
void strip_nl(char *str)
{
  char *p;

  p = strchr(str, '\n');
  if (p)
    *p = 0;
  p = strchr(str, '\r');
  if (p)
    *p = 0;
}

/*
 * Remove any characters in the set 'chars' from string 'str'.
 */
void omit_chars(char *str, const char *chars)
{
  int src, dst;

  for (src = 0, dst = 0; str[src]; src++)
  {
    if (strchr(chars, str[src]))
      continue;
    str[dst] = str[src];
    dst++;
  }
  str[dst] = 0;
}

#ifndef HAVE_SETENV
int tl_setenv(const char *var, const char *val, int ovr)
{
  char *pom = _malloc(strlen(var) + strlen(val) + 2);
  int ret;

  sprintf(pom, "%s=%s", var, val);
  ret = putenv(pom);
  _free(pom);
  return ret;
}
#endif

#ifndef HAVE_INET6
static char *xstrherror(int enr)
{
  char *p;

  switch (enr)
  {
#ifdef NETDB_INTERNAL
  case NETDB_INTERNAL:
    p = strerror(errno);
    break;

#endif
#ifdef NETDB_SUCCESS
  case NETDB_SUCCESS:
    p = gettext("no error");
    break;

#endif
#ifdef HOST_NOT_FOUND
  case HOST_NOT_FOUND:
    p = gettext("host not found");
    break;

#endif
#ifdef TRY_AGAIN
  case TRY_AGAIN:
    p = gettext("temporary error (try again later)");
    break;

#endif
#ifdef NO_RECOVERY
  case NO_RECOVERY:
    p = gettext("non recoverable error");
    break;

#endif
#ifdef NO_ADDRESS
  case NO_ADDRESS:
    p = gettext("name is valid, but doesn't have an IP address");
    break;

#endif
  default:
    p = gettext("unknown hostname translation error");
  }
  return p;
}
#endif

static const char *getstrerror(int err)
{
  if (err == 0)
    return "---";

  return strerror(err);
}

#if defined (DEBUG)

void _xherror(const char *sourcefile, int lineno, const char *msg)
{
  if (!msg || ! * msg)
    msg = "---";
#ifdef HAVE_INET6
  xprintf(1, "[%s/%d] %s: %s\n", sourcefile, lineno, msg, gai_strerror(_h_errno_));
#else
  xprintf(1, "[%s/%d] %s: %s\n", sourcefile, lineno, msg, xstrherror(_h_errno_));
#endif
}

void xvherror(const char *sourcefile, int lineno, const char *filename, const char *msg, ...)
{
  va_list args;

  va_start(args, msg);
  if (!msg || ! * msg)
    msg = "---";
#ifdef HAVE_INET6
  xprintf(1, "[%s/%d] %s: %s : ", sourcefile, lineno, (filename ? filename : "???"),
          gai_strerror(_h_errno_));
#else
  xprintf(1, "[%s/%d] %s: %s : ", sourcefile, lineno, (filename ? filename : "???"),
          xstrherror(_h_errno_));
#endif
  xvaprintf(1, msg, &args);
  va_end(args);
}

void _xperror(const char *sourcefile, int lineno, const char *msg)
{
  if (!msg || ! * msg)
    msg = "---";
  xprintf(1, "[%s/%d] %s: %s\n", sourcefile, lineno, msg, getstrerror(errno));
}

void _xperror1(const char *sourcefile, int lineno, const char *msg, const char *arg1)
{
  if (!msg || ! * msg)
    msg = "---";
  if (!arg1)
    arg1 = "???";
  xprintf(1, "[%s/%d] %s: %s : %s\n", sourcefile, lineno, msg, getstrerror(errno), arg1);
}

void _xperror2(const char *sourcefile,
               int         lineno,
               const char *msg,
               const char *arg1,
               const char *arg2)
{
  if (!msg || ! * msg)
    msg = "---";
  if (!arg1)
    arg1 = "???";
  if (!arg2)
    arg2 = "???";
  xprintf(1, "[%s/%d] %s: %s : %s - %s\n", sourcefile, lineno, msg, getstrerror(errno), arg1, arg2);
}

void xvperror(const char *sourcefile, int lineno, const char *filename, const char *msg, ...)
{
  va_list args;

  va_start(args, msg);
  if (!msg || ! * msg)
    msg = "---";
  xprintf(1, "[%s/%d] %s: %s : ", sourcefile, lineno, (filename ? filename : "???"),
          getstrerror(errno));
  xvaprintf(1, msg, &args);
  va_end(args);
}

#else /* !DEBUG */

void xherror(const char *msg)
{
#ifdef HAVE_INET6
  xprintf(1, "%s: %s\n", msg, gai_strerror(_h_errno_));
#else
  xprintf(1, "%s: %s\n", msg, xstrherror(_h_errno_));
#endif
}

void xvherror(const char *filename, const char *msg, ...)
{
  va_list args;

  va_start(args, msg);
  if (!msg || ! * msg)
    msg = "---";
#ifdef HAVE_INET6
  xprintf(1, "%s: %s : ", (filename ? filename : "???"), gai_strerror(_h_errno_));
#else
  xprintf(1, "%s: %s : ", (filename ? filename : "???"), xstrherror(_h_errno_));
#endif
  xvaprintf(1, msg, &args);
  va_end(args);
}

void xperror(const char *msg)
{
  if (!msg || ! * msg)
    msg = "---";
  xprintf(1, "%s: %s\n", msg, getstrerror(errno));
}

void xperror1(const char *msg, const char *arg1)
{
  if (!msg || ! * msg)
    msg = "---";
  if (!arg1)
    arg1 = "???";
  xprintf(1, "%s: %s : %s\n", msg, getstrerror(errno), arg1);
}

void xperror2(const char *msg, const char *arg1, const char *arg2)
{
  if (!msg || ! * msg)
    msg = "---";
  if (!arg1)
    arg1 = "???";
  if (!arg2)
    arg2 = "???";
  xprintf(1, "%s: %s : %s - %s\n", msg, getstrerror(errno), arg1, arg2);
}

void xvperror(const char *filename, const char *msg, ...)
{
  va_list args;

  va_start(args, msg);
  if (!msg || ! * msg)
    msg = "---";
  xprintf(1, "%s: %s : ", (filename ? filename : "???"), getstrerror(errno));
  xvaprintf(1, msg, &args);
  va_end(args);
}

#endif
#ifdef HAVE_MT
static void st_xvaprintf(unsigned int log, const char *strs, va_list *args)
#else
void xvaprintf(unsigned int log, const char *strs, va_list *args)
#endif
{
#ifdef GTK_FACE
  char *buf = g_strdup_vprintf(strs, *args);
#else
  char buf[PRINTF_BUFFER_SIZE];
#ifdef HAVE_VSNPRINTF
  int l = vsnprintf(buf, PRINTF_BUFFER_SIZE, strs, *args);
  ASSERT(l <= PRINTF_BUFFER_SIZE);
  buf[PRINTF_BUFFER_SIZE - 1] = 0;
#elif defined (HAVE__VSNPRINTF)
  int l = _vsnprintf(buf, PRINTF_BUFFER_SIZE, strs, *args);
  ASSERT(l <= PRINTF_BUFFER_SIZE);
  buf[PRINTF_BUFFER_SIZE - 1] = 0;
#else
  vsprintf(buf, strs, *args);
  if (strlen(buf) >= PRINTF_BUFFER_SIZE)
  {
    xperror("fatal buffer overflow in xvaprintf");
    abort();
  }
#endif
  ASSERT(strlen(buf) < PRINTF_BUFFER_SIZE);
#endif

  if (log)
  {
    log_str(buf);
  }

#ifdef I_FACE
  if (cfg.xi_face && cfg.done && !cfg.quiet)
  {
    gui_xprint(buf);
  }
  else
#endif
  {
    if (!cfg.quiet && !cfg.bgmode)
    {
      bool_t wout = TRUE;
#ifdef HAVE_TERMIOS
      if (cfg.tccheck)
      {
        static int _istty = -1;
        static pid_t _pgrp;

        if (_istty == -1)
        {
          _istty = isatty(1);
#ifdef GETPGRP_NEED_PID
          _pgrp = getpgrp(getpid());
#else
          _pgrp = getpgrp();
#endif
        }

        if (_istty && tcgetpgrp(1) != _pgrp)
          wout = FALSE;
      }
#endif
      if (wout)
      {
        fwrite(buf, sizeof(char), strlen(buf), stdout);
        fflush(stdout);
      }
    }
  }
#ifdef GTK_FACE
  g_free(buf);
#endif
}

#ifdef HAVE_MT
void xvaprintf(unsigned int log, const char *strs, va_list *args)
{
  doc *cdoc;

  cdoc = (doc *)pthread_getspecific(cfg.currdoc_key);

  if (!cdoc || (cfg.nthr == 1) || cfg.immessages)
  {
    st_xvaprintf(log, strs, args);
  }
  else if (cdoc)
  {
    doc_msg *dm = _malloc(sizeof(doc_msg));
    char buf[PRINTF_BUFFER_SIZE];

    buf[0] = 0;
    vsnprintf(buf, PRINTF_BUFFER_SIZE, strs, *args);
    buf[PRINTF_BUFFER_SIZE - 1] = 0;

    dm->log = log;
    dm->msg = tl_strdup(buf);

    cdoc->msgbuf = dllist_append(cdoc->msgbuf, dm);
  }
  else
  {
    st_xvaprintf(log, strs, args);
  }
}
#endif

void xprintf(unsigned int log, const char *strs, ...)
{
  va_list args;

  va_start(args, strs);
  xvaprintf(log, strs, &args);
  va_end(args);
}

void xdebug(unsigned int level, const char *levelstr, const char *strs, ...)
{
#ifdef HAVE_DEBUG_FEATURES
#if defined (HAVE_MT)
  static pthread_mutex_t unique_lock = PTHREAD_MUTEX_INITIALIZER;
  static pthread_mutex_t recursive_call_detector = PTHREAD_MUTEX_INITIALIZER;
  static volatile int unique = 0;
  static pthread_t its_me =
#if defined (PTW32_VERSION)
  {
    0, 0
  }                             /* pthreads-Win32 uses a _struct_ for pthread_t! */
#else
    0
#endif
  ;
#endif

  if (cfg.debug && (level & cfg.debug_level))
  {
    va_list args;
    va_start(args, strs);
#ifdef HAVE_MT
    {
      int ret = 0;

      for ( ; ;)
      {
        if (!ret)
        {
          ret = pthread_mutex_trylock(&unique_lock);
        }
        else
        {
#if DEBUG_DEADLOCK              /* NOT #ifdef! */
          printf
          ("\n%4d - DEBUG LOCK - second round - now LOCKING!\n", unique++);
#endif
          pthread_mutex_lock(&unique_lock);
          ret = 0;
        }
        if (!ret)
        {
          char *fmt;
          int fmtlen;

          pthread_mutex_lock(&recursive_call_detector);
          its_me = pthread_self();
          pthread_mutex_unlock(&recursive_call_detector);

          fmtlen = strlen(strs) + 1 + /* 5 --> worst case: */ 14 + strlen(levelstr) + 1;
          fmt = _malloc(fmtlen);
          snprintf(fmt, fmtlen, "%4d %s %s", unique++, levelstr, strs);
          ASSERT(strlen(fmt) < fmtlen);
          fmt[fmtlen - 1] = 0;
          st_xvaprintf(1, fmt, &args);
          _free(fmt);

          pthread_mutex_lock(&recursive_call_detector);
          /* its_me = 0; */
          memset(&its_me, 0, sizeof(its_me));   /* [i_a] pthreads-Win32 portability: zero the 'who am i again?' pthread_t copy */
          pthread_mutex_unlock(&recursive_call_detector);

          pthread_mutex_unlock(&unique_lock);
          break;
        }
        else
        {
          bool_t locker_is_me;

          /*
           * st_xvaprintf() will recursively try to call this line if you are
           * going a bit wild with DEBUG_xxxx() calls (like I did ;-( )
           * The xxxx_trylock() code will take care of that situation by
           * preventing deadlock, when combined with this section:
           *
           * we keep track of which thread is actually locking us out, and
           * we only skip the log/print operation if the one locking is ourselves
           * (pthread_self()), because that singular situation implies this
           * routine has been called recursively from WITHIN.
           * Since we only need to detect if is ME or someone alse locking the
           * log/print section, I can test this easily; if it was ME anyway, the
           * locking routine wouldn't go anywhere as it must wait for this routine
           * to return to the caller (which will pop stack, etc. etc. until we
           * return in the xdebug() routine which'd otherwise have caused this thread
           * to deadlock itself (and all the others alongside).
           */
#if DEBUG_DEADLOCK              /* NOT #ifdef! */
          printf("\n%4d - DEADLOCK PREVENTION\n", unique++);
#endif
          pthread_mutex_lock(&recursive_call_detector);
          locker_is_me = pthread_equal(its_me, pthread_self());
          pthread_mutex_unlock(&recursive_call_detector);
          if (locker_is_me)
          {
#if DEBUG_DEADLOCK              /* NOT #ifdef! */
            printf
            ("\n%4d - DEADLOCK THROUGH RECURSIVE CALL TO xdebug() DETECTED & PREVENTED\n",
             unique++);
#endif
            break;
          }
          /*
           * else: try another round, but LOCK this time as we are now sure we have not
           * been called recursively from within, so no risk at deadlock.
           * (Unless we screw up somewhere else in the code, e.g. calling this xdebug()
           * routine inside code which locks the logfile(s) which are also used to log
           * these statements when the proper parameters have been set.
           *
           * Caveat emptor.
           */
        }
      }
    }
#else
    xvaprintf(1, strs, &args);
#endif
    va_end(args);
  }
#endif /* HAVE_DEBUG_FEATURES */
}

void xvadebug(unsigned int level, const char *strs, va_list *args)
{
#ifdef HAVE_DEBUG_FEATURES
  if (cfg.debug && (level & cfg.debug_level))
  {
    xvaprintf(1, strs, args);
  }
#endif
}

void print_progress_style(int where, const char *special_msg)
{
  static int wout = 0x03;       /* [i_a] sneaky way to say 'TRUE' in a tristate sort of fashion ;-) */

#ifdef HAVE_TERMIOS
  if (cfg.tccheck && (wout & 0x02))
  {
    static int _istty = -1;
    static pid_t _pgrp;

    if (_istty == -1)
    {
      _istty = isatty(1);
#ifdef GETPGRP_NEED_PID
      _pgrp = getpgrp(getpid());
#else
      _pgrp = getpgrp();
#endif
    }

    if (_istty)
    {
      /* wout &= 0x01; *//* do not check again! Implicit in the next line: */
      wout = (tcgetpgrp(1) == _pgrp);
    }
  }
#endif
  if (wout & 0x01)
  {
    const char *str = NULL;
    switch (cfg.progress_mode)
    {
    case 1:
      str = ".";
      break;

    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    default:
      str = special_msg;
      if (!str)
        str = ".";
      break;
    }

    if (str)
    {
      fwrite(str, sizeof(char), strlen(str), stdout);
      fflush(stdout);
    }
  }
}




unsigned int hash_func(const char *str, int num)
{
  const char *p = str;
  unsigned int rv = 0;

  while (*p)
  {
    rv = (rv + ((unsigned char *)p)[0]) % num;
    p++;
  }
  rv = rv % num;
  return rv;
}

int report_unsup_locking(void)
{
  static int visited = FALSE;

  if (!visited)
  {
    visited = TRUE;
    xprintf(1, "------------------------------------------------------------------------------\n");
    xprintf(1,
            gettext(
              "Warning: locking not supported ... don't run multiple processes or threads!\n"));
    xprintf(1, "------------------------------------------------------------------------------\n");
  }
  return 0;
}

/*
 * inputs:
 * fd: open filehandle
 * filename: filename for file create & diagnostics
 * opt: options for file create
 * b_lock: TRUE if BLOCKING lock is required; otherwise a non-blocking lock is requested.
 *
 * returns:
 * 0 on success.
 */
int tl_flock(int *fd, const char *filename, int opt, int b_lock)
{
  int i = 0;

  /* currently it seems to me that BeOS   */
  /* doesn't support file locking :-(     */
  /* so just without real locking, report */
  /* successfully acquired lock           */
#ifdef __BEOS__
  report_unsup_locking();
  return 0;

#endif
  DEBUG_LOCKS("Locking file - %s\n", filename);
  if (b_lock)
  {
    bool_t ready = FALSE;
    while (!ready)
    {
      /*
       * As the Linux flock(2) manpage says: flock() does not work across NFS
       * shares, so we _prefer_ to use fcntl() which _does_, as pavuk may
       * be storing spidered files on an NFS-shared filesystem!
       *
       * Hence we check for both, then prioritize in the source code here.
       * (See also 'configure.in')
       */
#if defined (HAVE_FCNTL_LOCK)
      struct flock fl;

      memset(&fl, 0, sizeof(fl));
      fl.l_type = F_WRLCK;
      if (fcntl(*fd, F_SETLK, &fl))
      {
        if (errno == EWOULDBLOCK)
        {
          xprintf(1, gettext("waiting to release lock on FD : %d\n"), *fd);
          memset(&fl, 0, sizeof(fl));
          fl.l_type = F_WRLCK;
          i = fcntl(*fd, F_SETLKW, &fl);
        }
        else if (errno == ENOSYS
#ifdef ENOTSUP
                 || errno == ENOTSUP
#endif
#ifdef EOPNOTSUPP
                 || errno == EOPNOTSUPP
#endif
        )
        {
          report_unsup_locking();
          break;
        }
        else
        {
          xperror1("Cannot [un]lock file", mk_native(filename));
        }
      }
      else
      {
        i = 0;
      }
#elif defined (HAVE_FLOCK) || (defined (WIN32) || defined (_WIN32))
      if (flock(*fd, LOCK_EX | LOCK_NB))
      {
        if (errno == EWOULDBLOCK)
        {
          xprintf(1, gettext("waiting to release lock on FD : %d\n"), *fd);
          i = flock(*fd, LOCK_EX);
        }
        else if (errno == ENOSYS
#ifdef ENOTSUP
                 || errno == ENOTSUP
#endif
#ifdef EOPNOTSUPP
                 || errno == EOPNOTSUPP
#endif
        )
        {
          report_unsup_locking();
          break;
        }
        else
        {
          xperror1("Cannot [un]lock file", mk_native(filename));
        }
      }
      else
      {
        i = 0;
      }
#else
      _set_errno(ENOSYS);
      report_unsup_locking();
      break;
#endif
      if (tl_access(filename, F_OK))
      {
        DEBUG_LOCKS("Lock file was removed - creating new one.\n");
        close(*fd);
        if (!makealldirs(filename))
        {
          *fd = tl_open(filename, opt, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
          if (*fd < 0)
          {
            i = -1;
            ready = TRUE;
          }
        }
        else
        {
          i = -1;
          ready = TRUE;
        }
      }
      else
      {
        ready = TRUE;
      }
    }
  }
  else
  {
#if defined (HAVE_FCNTL_LOCK)
    struct flock fl;

    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;
    i = fcntl(*fd, b_lock ? F_SETLKW : F_SETLK, &fl);
#elif defined (HAVE_FLOCK) || (defined (WIN32) || defined (_WIN32))
    i = flock(*fd, LOCK_EX | LOCK_NB);
#else
    _set_errno(ENOSYS);
    i = -1;
#endif
    if (i < 0 && (errno == ENOSYS
#ifdef ENOTSUP
                  || errno == ENOTSUP
#endif
#ifdef EOPNOTSUPP
                  || errno == EOPNOTSUPP
#endif
        ))
    {
      report_unsup_locking();
      i = 0;
    }
    else if (i < 0)
    {
      if (errno != EACCES && errno != EAGAIN)
      {
        xperror1("Cannot [un]lock file", mk_native(filename));
      }
    }
  }

  return i;
}

#ifdef HAVE_FCNTL_LOCK
int tl_funlock(int fd)
{
  struct flock fl;
  int i;

  memset(&fl, 0, sizeof(fl));
  fl.l_type = F_UNLCK;
  i = fcntl(fd, F_SETLK, &fl);
  if (i < 0)
  {
    if (errno != EACCES && errno != EAGAIN)
    {
      xperror("Cannot unlock file");
    }
  }

  return i;
}
#endif

int tl_mkstemp(char *pattern)
{
#ifdef HAVE_MKSTEMP
  return mkstemp(pattern);

#else
  tmpnam(pattern);

  return tl_open(pattern, O_BINARY | O_CREAT | O_RDWR | O_TRUNC | O_EXCL,
                 S_IRUSR | S_IWUSR);

#endif
}

long int _atoi(const char *str)
{
  char *__eptr__;
  long int rv;

  if (! * str)
  {
    errno = ERANGE;
    rv = 0;
  }
  else
  {
    errno = 0;
    rv = strtol(str, &__eptr__, 10);
    if (*__eptr__ != 0)
      errno = ERANGE;
    else
      errno = 0;
  }

  return rv;
}

/*
 * Parse decimal numbers, which are optionally followed
 * for one of these size scale characters:
 *
 *   K   (kilo)
 *   M   (mega)
 *   G   (giga)
 *
 * (lower case versions of those are treated the same).
 * The sizes are powers of 10 if decimal_or_binary_sizes!=FALSE,
 * otherwise these sizes are assumed to be the computer power-of-2
 * well known values: 1024,etc.
 */
long int _atoi_with_size(const char *str, bool_t decimal_or_binary_sizes)
{
  char *__eptr__;
  long int rv;

  if (! * str)
  {
    errno = ERANGE;
    rv = 0;
  }
  else
  {
    errno = 0;
    rv = strtol(str, &__eptr__, 10);
    switch (*__eptr__)
    {
    case 'g':
    case 'G':
      rv *= (decimal_or_binary_sizes ? 1000 : 1024);

    case 'm':
    case 'M':
      rv *= (decimal_or_binary_sizes ? 1000 : 1024);

    case 'k':
    case 'K':
      rv *= (decimal_or_binary_sizes ? 1000 : 1024);
      __eptr__++;
      break;
    }
    if (*__eptr__ != 0)
      errno = ERANGE;
    else
      errno = 0;
  }

  return rv;
}

double _atof(const char *str)
{
  char *__eptr__;
  double rv;

  if (! * str)
  {
    errno = ERANGE;
    rv = 0.0;
  }
  else
  {
    errno = 0;
    rv = strtod(str, &__eptr__);
    if (*__eptr__ != 0)
      errno = ERANGE;
    else
      errno = 0;
  }

  return rv;
}


char *_strtrchr(char *str, int cfrom, int cto)
{
  char *p = str;

  while ((p = strchr(p, cfrom)))
    *p = cto;

  return str;
}

char *get_1qstr(const char *str)
{
  static const char *p = 0;
  static char pom[4096];
  const char *pom1 = NULL, *pom2 = NULL;
  int found;
  bool_t wasesc = FALSE;

  if (str)
    p = str;

  for ( ; p && *p && (!pom1 || !pom2); p++)
  {
    found = FALSE;

    if (!wasesc && *p == '\"')
      found = TRUE;

    if (!wasesc && *p == '\\')
      wasesc = TRUE;
    else
      wasesc = FALSE;

    if (!pom1 && found)
      pom1 = p + 1;
    else if (!pom2 && found)
      pom2 = p;
  }
  if (pom1 && pom2)
  {
    char *p2;
    p2 = pom;
    while (pom1 < pom2)
    {
      if (*pom1 == '\\')
      {
        pom1++;
        *p2 = *pom1;
      }
      else
      {
        *p2 = *pom1;
      }
      p2++;
      pom1++;
    }
    *p2 = 0;
    return pom;
  }
  else
  {
    p = NULL;
    return NULL;
  }
}

/*
 * Escape all 'unsafe' characters in source string 'str'
 * and return the result string (which has been allocated
 * on the heap).
 *
 * Characters are escaped using a '\' backslash.
 */
char *escape_str(const char *str, const char *unsafe)
{
  char *sbuf;                   /* [i_a] */
  const char *p;
  char *r;

  ASSERT(str != NULL);
  sbuf = _malloc(strlen(str) * 2 + 1);
  for (p = str, r = sbuf; *p; p++)
  {
    if (strchr(unsafe, *p))
    {
      *r = '\\';
      r++;
      *r = *p;
      r++;
    }
    else if (*p == '\\') /* [i_a] always escape the escape character itself! */
    {
      *r = '\\';
      r++;
      *r = *p;
      r++;
    }
    else
    {
      *r = *p;
      r++;
    }
  }
  *r = 0;
  return sbuf;                  /* [i_a] */
}

/**************************************************************/
/* implementation of standard function strtok                 */
/* split string at occurence of <chr> by setting \0-character */
/* store next element (one behind \0) pointer for next call   */
/* calling with str equal to zero uses stored pointer         */
/**************************************************************/
char *strtokc_r(char *str, int chr, char **save)
{
  char *ret;

  if (str)
    *save = str;

  if ((ret = *save))
  {
    char *pom;

    if ((pom = strchr(*save, chr)))
    {
      *pom = 0;
      *save = pom + 1;
    }
    else
      *save = NULL;
  }

  return ret;
}

/*
 * Find the N-th occurrence of character chr in string str.
 *
 * Return NULL if the occurrence could not be found.
 *
 * NOTE: N is assumed to be greater or equal to 1.
 */
const char *strfindnchr(const char *str, int chr, int n)
{
  int cnt;
  const char *p;

  for (p = str, cnt = 0; *p && cnt < n; p++)
  {
    if (*p == chr)
      cnt++;
  }
  if (cnt != n)
    return NULL;
  else
    return p - 1;
}


/*
 * Find the N-th LAST occurrence of character chr in string str.
 *
 * Return NULL if the occurrence could not be found.
 *
 * NOTE: N is assumed to be greater or equal to 1.
 */
const char *strrfindnchr(const char *str, int chr, int n)
{
  int cnt;
  const char *p;

  for (p = str + strlen(str) - 1, cnt = 0; p >= str && cnt < n; p--)
  {
    if (*p == chr)
      cnt++;
  }
  if (cnt != n)
    return NULL;
  else
    return p + 1;
}

/********************************************/
/* search in list for string occurrence     */
/********************************************/
bool_t is_in_list(const char *str, const char **list)
{
  const char **p = list;

  while (*p)
  {
    if (!strcasecmp(*p, str))
      return TRUE;

    p++;
  }
  return FALSE;
}

bool_t is_in_dllist(const char *str, dllist *list)
{
  dllist *p;

  for (p = list; p; p = p->next)
  {
    if (!strcasecmp((const char *)p->data, str))
      return TRUE;
  }
  return FALSE;
}

/****************************************************/
/* match string again list of wildcard patterns     */
/****************************************************/
bool_t is_in_pattern_list(const char *str, const char **list)
{
  const char **p = list;

  while (*p)
  {
    if (!fnmatch(*p, str, 0))
      return TRUE;

    p++;
  }
  return FALSE;
}

bool_t is_in_pattern_dllist(const char *str, dllist *list)
{
  dllist *p;

  for (p = list; p; p = p->next)
  {
    if (!fnmatch((const char *)p->data, str, 0))
      return TRUE;
  }
  return FALSE;
}

/*******************************************************/
/* split string to NULL terminated array of strings    */
/* separated with some of characters in sep            */
/*******************************************************/
char **tl_str_split(const char *liststr, const char *sep)
{
  const char *p;
  int i = 0;
  char **ret_val = NULL;
  int ilen;

  if (!liststr || ! * liststr)
    return NULL;

  ret_val = _malloc(sizeof(char **));
  ret_val[0] = NULL;

  p = liststr;

  while (*p)
  {
    ilen = strcspn(p, sep);
    ret_val = _realloc(ret_val, sizeof(char **) * (i + 2));
    ret_val[i] = tl_strndup(p, ilen);
    ret_val[i + 1] = NULL;
    p += ilen;
    if (*p)
      p++;
    i++;
  }

  return ret_val;
}

dllist *tl_numlist_split(char *str, char *sep)
{
  dllist *rv = NULL;
  char **v;
  int i;

  v = tl_str_split(str, sep);

  for (i = 0; v && v[i]; i++)
  {
    long n = _atoi(v[i]);
    if (errno == ERANGE)
    {
      while (rv)
        rv = dllist_remove_entry(rv, rv);
      break;
    }

    rv = dllist_append(rv, (dllist_t)n);
  }

  tl_strv_free(v);

  return rv;
}

/* free null terminated string vector */
void tl_strv_free(char **v)
{
  int i;

  for (i = 0; v && v[i]; i++)
    _free(v[i]);

  _free(v);
}

/* count length of null terminated string vector */
int tl_strv_length(char **v)
{
  int i;

  for (i = 0; v && v[i]; i++) ;

  return i;
}

int tl_strv_find(char **v, char *s)
{
  int i;

  for (i = 0; v && v[i]; i++)
  {
    if (!strcmp(v[i], s))
      return i;
  }
  return -1;
}

static int sort_strcmp(const char **s1, const char **s2)
{
  return strcmp(*s1, *s2);
}

void tl_strv_sort(char **v)
{
  if (v)
  {
    qsort((void *)v, tl_strv_length(v), sizeof(char *),
          (int(*) (const void *, const void *))sort_strcmp);
  }
}

/*************************************************/
/* change all characters in string to upper case */
/*************************************************/
char *upperstr(char *s)
{
  char *p;

  for (p = s; *p != 0; p++)
    *p = tl_ascii_toupper(*p);

  return s;
}

/*************************************************/
/* change all characters in string to lower case */
/*************************************************/
char *lowerstr(char *s)
{
  char *p;

  for (p = s; *p != 0; p++)
    *p = tl_ascii_tolower(*p);

  return s;
}

/* [i_a] tl_strdup et al moved down; special memleak testing code added */

/***************************************************/
/* create all directories in path specification    */
/***************************************************/
int makealldirs(const char *path)
{
  char pom[PATH_MAX];
  const char *p;

  pom[0] = 0;

  if (path)
  {
    p = path;
#if defined (WIN32) || defined (__WIN32)
/*
 * we can't create new drive on WIN32, so the drive
 * specification part of path must be skipped
 */
#ifdef HAVE_NEWSTYLE_CYGWIN
    if (p[0] == '/' && p[1] == '/')
    {
      char *sp;

      if ((sp = strchr(p + 2, '/')) && (sp = strchr(sp + 1, '/')))
      {
        strncpy(pom, p, sp - p);
        pom[sp - p] = 0;
        p = sp;
      }
    }
    else if (!strncmp(p, "/cygdrive/", 10)
             && (strlen(p) > 10) && tl_ascii_isalpha(p[10]))
    {
      strncpy(pom, p, 11);
      pom[11] = 0;
      p += 11;
    }
#else
    if (strlen(p) > 2 && p[0] == '/' && p[1] == '/'
        && tl_ascii_isalpha(p[2]) && (p[3] == 0 || p[3] == '/'))
    {
      strncpy(pom, p, 3);
      pom[3] = 0;
      p += 3;
    }
#endif
    /* [i_a] Win32 native drive fixup: */
    p += strspn(p, "/");
#endif
    while (*p)
    {
      int ilen = strcspn(p, "/");

      strcat(pom, "/");
      strncat(pom, p, ilen);

      p += ilen;
      p += strspn(p, "/");

      if (*p && tl_access(pom, F_OK))
      {
        if (tl_mkdir(pom, S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH))
          return -1;
      }
    }
  }
  return 0;
}

/*
 * Transform the path into an absolute path, i.e.
 * remove any relative path elements such as '.'
 * and '..'.
 *
 * If the specified path <path> is not an absolute path,
 * it will be assumed to be relative to the current active
 * working directory UNLESS an alternative
 * 'document storage directory' has been specified by the user
 * using the pavuk '-cdir' commandline argument.
 *
 * NOTE: any leading whitespace in <path> will be stripped, i.e.
 *       relative paths should never start with a directory name
 *       which contains whitespace.
 *
 * When a NULL <path> has been specified, this function will
 * return NULL.
 *
 * Otherwise, a properly formatted and 'folded' i.e. './' and '../'
 * path elements removed) absolute path will be returned. Note that
 * this string is allocated on the head so you will need to _free()
 * it when done.
 */
#undef get_abs_file_path_oss
char *get_abs_file_path_oss(DBGdecl (const char *path))
{
  const char *pp;
  char *p;
  char pom[PATH_MAX];
  char *tmp;
  char result[PATH_MAX] = "/";
  int ilen;
  bool_t last = 1;

  if (path == NULL)
    return NULL;

  for (pp = path; tl_ascii_isspace(*pp) && *pp; pp++) ;

  if (*pp != '/')
  {
    getcwd(pom, PATH_MAX);
#if defined (WIN32) || defined (__WIN32)
    tmp = cvt_win32_to_unix_path(pom, FALSE);
    /* [i_a] fixup apr/2007 */
#else
    tmp = tl_strdup(pom);
#endif
    if (priv_cfg.cache_dir)
      snprintf(pom, sizeof(pom), "%s%s%s", priv_cfg.cache_dir,
               (tl_is_dirname(priv_cfg.cache_dir) ? "" : "/"), pp);
    else
      snprintf(pom, sizeof(pom), "%s%s%s", (tmp ? tmp : ""), (tl_is_dirname(tmp) ? "" : "/"), pp);
    _free(tmp);
  }
  else
  {
    snprintf(pom, sizeof(pom), "%s", pp);
  }
  pom[sizeof(pom) - 1] = 0;

  p = pom;

#if defined (WIN32) || defined (__WIN32)
#ifndef HAVE_NEWSTYLE_CYGWIN
  /* workaround to allow //[drive]/... paths on WIN32 */
  if (strlen(pom) > 2 && pom[0] == '/' && pom[1] == '/'
      && tl_ascii_isalpha(pom[2]) && (pom[3] == 0 || pom[3] == '/'))
  {
    strncpy(result, pom, 3);
    result[3] = 0;
    p = pom + 3;
  }
#else
  /* workaround to allow //host/share/... paths on WIN32 */
  /* AFAIK this type of paths work with cygwin-1.1 =<    */
  if (pom[0] == '/' && pom[1] == '/')
  {
    strcpy(result, "//");
    p++;
  }
#endif
#endif
  if (! * p)
    strcpy(result, "/");

  while (*p)
  {
    ilen = strcspn(p, "/");
    if (*(p + ilen))
      *(p + ilen) = 0;
    else
      last = 0;

    if (strcmp(p, "."))
    {
      if (strcmp(p, ".."))
      {
        if (!tl_is_dirname(result))
          strcat(result, "/");
        strcat(result, p);
      }
      else
      {
        tmp = strrchr(result, '/');
        *(tmp + 1 - (tmp != result)) = 0;
      }
    }
    p += ilen + last;
    p += strspn(p, "/");
  }

  ilen = strlen(path);
  pp = path + ilen - 1;
  if (ilen && *pp != '/' && tl_is_dirname(result))
  {
    result[strlen(result) - 1] = 0;
  }

  if ((tl_is_dirname(path) && !tl_is_dirname(result)) || (strlen(result) == 0))
  {
    result[strlen(result) + 1] = 0;
    result[strlen(result)] = '/';
  }

  return tld_strdup(DBGpass(result));
}


/*
 * Transform the path into an absolute path, i.e.
 * remove any relative path elements such as '.'
 * and '..'.
 *
 * Very similar to get_abs_file_path_oss() with
 * ONE EXCEPTION: this routine does NOT consider any
 * possible '-cdir' specified alternative document
 * storage directory as current working directory
 * override.
 *
 * All other notes and caveats apply as for get_abs_file_path_oss().
 */
#undef get_abs_file_path
char *get_abs_file_path(DBGdecl (const char *path))
{
  const char *pp;
  char *p;
  char pom[PATH_MAX];
  char *tmp;
  char result[PATH_MAX] = "/";
  int ilen;
  bool_t last = 1;

  if (path == NULL)
    return NULL;

  for (pp = path; tl_ascii_isspace(*pp) && *pp; pp++) ;

  if (*pp != '/')
  {
    getcwd(pom, PATH_MAX);
#if defined (WIN32) || defined (__WIN32)
    tmp = cvt_win32_to_unix_path(pom, FALSE);
    /* [i_a] fixup apr/2007 */
#else
    tmp = tl_strdup(pom);
#endif
    snprintf(pom, sizeof(pom), "%s%s%s", (tmp ? tmp : ""), (tl_is_dirname(tmp) ? "" : "/"), pp);
    _free(tmp);
  }
  else
  {
    snprintf(pom, sizeof(pom), "%s", pp);
  }
  pom[sizeof(pom) - 1] = 0;

  p = pom;

#if defined (WIN32) || defined (__WIN32)
#ifndef HAVE_NEWSTYLE_CYGWIN
  /* workaround to allow //[drive]/... paths on WIN32 */
  if (strlen(pom) > 2 && pom[0] == '/' && pom[1] == '/'
      && tl_ascii_isalpha(pom[2]) && (pom[3] == 0 || pom[3] == '/'))
  {
    strncpy(result, pom, 3);
    result[3] = 0;
    p = pom + 3;
  }
#else
  /* workaround to allow //host/share/... paths on WIN32 */
  /* AFAIK this type of paths work with cygwin-1.1 =<    */
  if (pom[0] == '/' && pom[1] == '/')
  {
    strcpy(result, "//");
    p++;
  }
#endif
#endif
  if (! * p)
    strcpy(result, "/");

  while (*p)
  {
    ilen = strcspn(p, "/");
    if (*(p + ilen))
      *(p + ilen) = 0;
    else
      last = 0;

    if (strcmp(p, "."))
    {
      if (strcmp(p, ".."))
      {
        if (!tl_is_dirname(result))
          strcat(result, "/");
        strcat(result, p);
      }
      else
      {
        tmp = strrchr(result, '/');
        *(tmp + 1 - (tmp != result)) = 0;
      }
    }
    p += ilen + last;
    p += strspn(p, "/");
  }

  ilen = strlen(path);
  pp = path + ilen - 1;
  if (ilen && *pp != '/' && tl_is_dirname(result))
  {
    result[strlen(result) - 1] = 0;
  }

  if ((tl_is_dirname(path) && !tl_is_dirname(result)) || (strlen(result) == 0))
  {
    result[strlen(result) + 1] = 0;
    result[strlen(result)] = '/';
  }

  return tld_strdup(DBGpass(result));
}


#if (defined (WIN32) || defined (__WIN32))

char *cvt_win32_to_unix_path(const char *path, bool_t keep_relative)
{
  char pom[PATH_MAX];
  char *p;
  char *p1;

  if (path == NULL)
    return NULL;

  p = tl_strdup(path);

  pom[0] = 0;
  p1 = p;

  if (strlen(p) >= 2 && tl_ascii_isalpha(p[0]) && p[1] == ':')
  {
#ifdef HAVE_NEWSTYLE_CYGWIN
    sprintf(pom, "/cygdrive/%c", p[0]);
    p += 2;
#else
    sprintf(pom, "//%c", p[0]);
    p += 2;
#endif
  }

  _strtrchr(p, '\\', '/');

  strncat(pom, p, sizeof(pom) - strlen(pom));
  pom[sizeof(pom) - 1] = 0;

  _free(p1);

  if (keep_relative)
  {
    return tl_strdup(pom);
  }
  else
  {
    return get_abs_file_path_oss(DBGvars(pom));
  }
}

#undef cvt_unix_to_win32_path
char *cvt_unix_to_win32_path(DBGdecl (const char *path))
{
  char pom[PATH_MAX];

#if defined (__CYGWIN__)
  cygwin32_conv_to_win32_path(path, pom);
#else
  char *p = tld_strdup(DBGpass(path));
  char *p1;

  pom[0] = 0;
  p1 = p;

  if (strlen(p) >= 3 && tl_ascii_isalpha(p[2]) && p[0] == '/' && p[1] == '/')
  {
    sprintf(pom, "%c:", p[2]);
    p += 3;
  }
  else if (strlen(p) >= 11 && !strncmp(p, "/cygdrive/", 10)
           && tl_ascii_isalpha(p[10]))
  {
    sprintf(pom, "%c:", p[10]);
    p += 11;
  }

  _strtrchr(p, '/', '\\');

  strncat(pom, p, sizeof(pom) - strlen(pom));
  pom[sizeof(pom) - 1] = 0;

  _free(p1);
#endif
  return tld_strdup(DBGpass(pom));
}

#endif

/*
 * Return the number of occurrences of character 'c' in string 's'.
 */
int str_cnt_chr(const char *s, int c)
{
  int ret = 0;

  if (!s)
    return 0;

  while (*s)
  {
    if (*s == c)
      ret++;
    s++;
  }

  return ret;
}

/*
 * Construct the (allocated) relative path for <toabs>, relative
 * to <fromabs>.
 *
 * NOTE: Neither <fromabs> nor <toabs> needs to be a fully qualified absolute
 * path itself. These paths will be converted into fully qualified and
 * properly 'folded' (i.e. './' and '../' path elements removed) paths
 * before comparing both by calling get_abs_file_path_oss() on each.
 *
 * For example,
 *
 *   get_relative_path("/ho/hum/index.htm", "ha/dot.htm")
 *
 * will produce the result
 *
 *   "../ha/dot.htm"
 *
 * when the current active working directory is "/ho" and no '-cdir'
 * commandline-specified alternative document storage directory has been
 * specified by the pavuk user (see the notes and caveats
 * at get_abs_file_path_oss().)
 *
 *
 * When both paths do not have any directories in common, this
 * function will return NULL.
 */
char *get_relative_path(char *fromabs, char *toabs)
{
  char *p1, *p2, *pom1, *pom2;
  int offset = 0, i, plom;
  char *rv = NULL;

  pom1 = p1 = get_abs_file_path_oss(DBGvars(fromabs));
  pom2 = p2 = get_abs_file_path_oss(DBGvars(toabs));

  while (*p1 && *p2 && *p1 == *p2)
  {
    p1++;
    p2++;
  }

#if 0
  /* this is not good behaviour, as lynx and netscape behaves */
  /* differently on empty HREFs                               */
  if (!strcmp(p1, p2))
  {
    _free(pom1);
    _free(pom2);
    return tl_strdup("");
  }
#endif
  if (*p1)
    p1--;

  while ((p1 >= pom1) && *p1 != '/')
    p1--;

  if (*p1 != '/')
  {
    _free(pom1);
    _free(pom2);

    return NULL;
  }
  offset = p1 - pom1;

  plom = str_cnt_chr(p1 + 1, '/');

  for (i = 0; i < plom; i++)
  {
    rv = tl_str_concat(DBGvars(rv), "../", NULL);
  }

  rv = tl_str_concat(DBGvars(rv), pom2 + offset + 1, NULL);

  _free(pom1);
  _free(pom2);

  return rv;
}


/*
 * Return the extension of the filename, excluding the '.' dot.
 *
 * Return an empty string if the filename does not have an extension.
 */
const char *tl_get_extension(const char *fname)
{
  const char *p1, *p2;

  p1 = strrchr(fname, '.');
  p2 = strrchr(fname, '/');

  if (p1 > p2)
  {
    return p1 + 1;
  }
  else
  {
    return "";
  }
}

char *tl_get_basename(char *fname)
{
  char *p;

  if ((p = strrchr(fname, '/')))
    p++;
  else
    p = fname;

  return p;
}

static const char *html_tag_tab[] =
{
  "<HTML",
  "<html",
  "<HEAD",
  "<head",
  "<META",
  "<meta",
  "<TITLE",
  "<title",
  "<BODY",
  "<body",
  "<script",
  "<SCRIPT",
  "<style",
  "<STYLE",
  "<!DOCTYPE HTML",
  "<!doctype html",
  "<!--",
};

bool_t ext_is_gzipped_file(const char *fn)
{
  const char *ext = tl_get_extension(fn);

  return str_is_in_list(0, ext, "gz", "tgz", "z", "Z", "GZ", "TGZ", NULL);
}

bool_t ext_is_html(const char *fn)
{
  const char *ext = tl_get_extension(fn);

  return str_is_in_list(0, ext, "html", "htm", "shtml", "phtml", "css", NULL);
}

static bool_t ext_is_nothtml(const char *fn)
{
  const char *ext = tl_get_extension(fn);

  return str_is_in_list(0, ext, "gif", "jpg", "jpeg", "png", "mpeg",
                        "mpg", "avi", "pdf", "gz", "tgz", "zip", "arj",
                        "hqx", "rar", "tar", "Z", "doc", "doc", "xls", "wav", "au", "mp3", NULL);
}

bool_t file_is_html(const char *fn)
{
  int i, j, len;
  char pom[512 /* no use to use: BUFIO_ADVISED_READLN_BUFSIZE */];
  bufio *fd;

  if (ext_is_html(fn))
    return TRUE;

  if (ext_is_nothtml(fn))
    return FALSE;

  if ((fd = bufio_open(fn, O_BINARY | O_RDONLY)))
  {
    for (j = 0; j < 10; j++)
    {
      if ((len = bufio_readln(fd, pom, sizeof(pom))) > 0)
      {
        for (i = 0; i < NUM_ELEM(html_tag_tab); i++)
        {
          if (strstr(pom, html_tag_tab[i]))
          {
            bufio_close(fd);
            fd = NULL;
            return TRUE;
          }
        }
      }
      else
      {
        if (len < 0)
          xperror1("file_is_html() - failure while reading data", fn);
        bufio_close(fd);
        fd = NULL;
        return FALSE;
      }
    }
    bufio_close(fd);
    fd = NULL;
  }
  return FALSE;
}

void tl_sleep(unsigned int s)
{
  /* if we measure timings, we don't sleep */
  if (bufio_is_open(cfg.time_logfile))
  {
    return;
  }

#if !defined (HAVE_MT) && defined (I_FACE)
  if (cfg.xi_face)
  {
    gui_msleep(s * 1000);
  }
  else
#endif
  {
#ifdef HAVE_MT
    struct timeval tout;

    tout.tv_sec = s;
    tout.tv_usec = 0;

    select(0, NULL, NULL, NULL, &tout);
#else
    sleep(s);
#endif
  }
}

void tl_msleep(unsigned int ms, bool_t forced)
{
  /* if we measure timings, we don't sleep */
  if (bufio_is_open(cfg.time_logfile))
  {
    return;
  }

#if !defined (HAVE_MT) && defined (I_FACE)
  if (cfg.xi_face)
  {
    gui_msleep(ms);
  }
  else
#endif
#if defined (HAVE_USLEEP) && !defined (HAVE_MT)
  {
    usleep(ms * 1000);
  }
#else
#if !defined (__CYGWIN__) && (defined (WIN32) || defined (__WIN32)) && !defined (HAVE_MT)
  {
    SleepEx(ms, TRUE);
  }
#else
  {
    struct timeval tout;

    tout.tv_sec = ms / 1000;
    tout.tv_usec = (ms % 1000) * 1000;

    select(0, NULL, NULL, NULL, &tout);
  }
#endif
#endif
}

int unlink_recursive(char *fn)
{
  struct stat estat;

  if (tl_lstat(fn, &estat))
  {
    xperror(mk_native(fn));
    return -1;
  }

  if (!S_ISDIR(estat.st_mode))
  {
    if (tl_unlink(fn))
    {
      xperror(mk_native(fn));
      return -1;
    }
  }
  else
  {
    DIR *dir;
    struct dirent *dent;
    char next_dir[PATH_MAX];

    if (!(dir = tl_opendir(fn)))
    {
      xperror(mk_native(fn));
      return -1;
    }

    while ((dent = readdir(dir)))
    {
      snprintf(next_dir, sizeof(next_dir), "%s%s%s", fn, (tl_is_dirname(fn) ? "" : "/"), dent->d_name);
      next_dir[sizeof(next_dir) - 1] = 0;
      if (!strcmp(dent->d_name, "."))
        continue;
      if (!strcmp(dent->d_name, ".."))
        continue;

      unlink_recursive(next_dir);
    }

    closedir(dir);

    if (tl_rmdir(fn))
    {
      xperror(mk_native(next_dir));
      return -1;
    }
  }

  return 0;
}

int str_is_in_list(int casesensitive, const char *str, ...)
{
  const char *which;
  va_list args;
  int found = FALSE;

  va_start(args, str);

  for (which = va_arg(args, const char *); which;
       which = va_arg(args, const char *))
  {
    if (casesensitive ? !strcmp(str, which) : !strcasecmp(str, which))
    {
      found = TRUE;
      break;
    }
  }

  va_end(args);

  return found;
}

int copy_fd_to_file(int fd, char *filename)
{
  char pom[32768];
  int len;
  int dfd;

  if ((dfd =
         tl_open(filename, O_BINARY | O_WRONLY | O_CREAT,
                 S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR)) < 0)
  {
    xperror(mk_native(filename));
    return -1;
  }

  lseek(fd, 0, SEEK_SET);
  while ((len = read(fd, pom, sizeof(pom))) > 0)
  {
    if (len != write(dfd, pom, len))
    {
      xperror(mk_native(filename));
      len = -1;
      break;
    }
  }

  close(dfd);

  lseek(fd, 0, SEEK_END);

  return len;
}

char *tl_adjust_filename(char *path)
{
  char *pom;
  char *p, *p2;
  int l, n;

  pom = _malloc(strlen(path) + 1);

  p = pom;
#if defined (__CYGWIN__) || defined (WIN32) || defined (__WIN32)
#ifndef HAVE_NEWSTYLE_CYGWIN
  l = strspn(path, "/");
  strncpy(p, path, l);
  p += l;
  *p = 0;
  path += l;
#endif
#endif
  while (*path)
  {
    n = strspn(path, "/");
    path += n;
    l = strcspn(path, "/");
    if (n)
    {
      *p = '/';
      p++;
    }

    if (! * (path + l) && ((NAME_MAX - 4) < l))
    {
      strncpy(p, path + l - (NAME_MAX - 4), NAME_MAX - 4);
      p += NAME_MAX - 4;
    }
    else if (l > NAME_MAX)
    {
      strncpy(p, path, NAME_MAX);
      p += NAME_MAX;
    }
    else
    {
      strncpy(p, path, l);
      p += l;
    }
    path += l;
  }
  *p = 0;

  n = strlen(pom);
  p = strrchr(pom, '/');

  while (p && (n > PATH_MAX))
  {
    *p = 0;
    p2 = strrchr(pom, '/');

    if (p2)
    {
      strcpy(p2 + 1, p + 1);
      n -= p - p2;
    }
    p = p2;
  }
  return pom;
}

int tl_filename_needs_adjust(const char *path)
{
  int l;

  if (strlen(path) > PATH_MAX)
    return TRUE;

  while (*path)
  {
    path += strspn(path, "/");
    l = strcspn(path, "/");

    if (!path[l] && ((NAME_MAX - 4) < l))
      return TRUE;
    else if (l > NAME_MAX)
      return TRUE;

    path += l;
  }

  return FALSE;
}

int tl_is_dirname(const char *path)
{
  return tl_strendchr(path, '/');
}

/*
 * Append str2 to str1.
 *
 * str1 MAY be NULL.
 *
 * Return the reallocated/resized concatenated result string.
 *
 * CAVEATS:
 *
 * - str1 must be allocated on the heap using malloc/strdup.
 *
 * - str1 WILL be pointing to invalid memory, due to the
 *   possibility of a realloc() occurring within this routine.
 */
#undef tl_str_append
char *tl_str_append(DBGdecl(char *str1), const char *str2)
{
  int l1, l2;
  char *rv;

  l1 = str1 ? strlen(str1) : 0;
  l2 = strlen(str2);
  rv = tld_realloc(DBGpass(str1), l1 + l2 + 1);
  strcpy(rv + l1, str2);

  return rv;
}

/*
 * Append at most n characters of str2 to str1.
 *
 * str1 MAY be NULL.
 *
 * Return the reallocated/resized concatenated result string.
 *
 * CAVEATS:
 *
 * - str1 must be allocated on the heap using malloc/strdup.
 *
 * - str1 WILL be pointing to invalid memory, due to the
 *   possibility of a realloc() occurring within this routine.
 */
char *tl_str_nappend(char *str1, const char *str2, int n)
{
  int l1;
  char *rv;

  l1 = str1 ? strlen(str1) : 0;
  rv = _realloc(str1, l1 + n + 1);
  if (n > 0)
    strncpy(rv + l1, str2, n);
  rv[l1 + n] = 0;

  return rv;
}

/*
 * Append a series of strings to str1. The series MUST be terminated
 * by a NULL pointer, e.g.
 *
 *    new_str1 = tl_str_concat(DBGvars(str1), str2, str3, NULL);
 *
 * str1 MAY be NULL.
 *
 * Return the reallocated/resized concatenated result string.
 *
 * CAVEATS:
 *
 * - str1 must be allocated on the heap using malloc/strdup.
 *
 * - str1 WILL be pointing to invalid memory, due to the
 *   possibility of a realloc() occurring within this routine.
 */
char *tl_str_concat(DBGdecl (char *str1), ...)
{
  char *p;
  va_list args;
  int len;
  char *rv = str1;

  len = str1 ? strlen(str1) : 0;

  va_start(args, str1);
  for (p = va_arg(args, char *); p; p = va_arg(args, char *))
  {
    int slen = strlen(p);

    rv = tld_realloc(DBGpass(rv), len + slen + 1);
    strcpy(rv + len, p);
    len += slen;
  }

  va_end(args);

  return rv;
}

/*
 * Append a series of strings to 'data'. The series MUST be terminated
 * by a NULL pointer, e.g.
 *
 *    new_data = tl_data_concat_str(DBGvars(lenptr), data, strA, strB, NULL);
 *
 * 'data' MAY be NULL.
 *
 * Return the reallocated/resized concatenated result string.
 *
 * The total length of the concatened strings will be added to the
 * value referenced by 'len'.
 *
 * CAVEATS:
 *
 * - 'data' must be allocated on the heap using malloc/strdup.
 *
 * - 'data' WILL be pointing to invalid memory, due to the
 *   possibility of a realloc() occurring within this routine.
 *
 * - '*len' is used as the initial allocated size of the 'data'
 *   block. Failure to comply to provide this number by the caller
 *       will result in incorrectly realloc()ed memory.
 *
 *       When 'data' is NULL, *len may be zero(0) too.
 */
char *tl_data_concat_str(DBGdecl (int *len), char *data, ...)
{
  char *p;
  va_list args;
  char *rv = data;

  va_start(args, data);

  for (p = va_arg(args, char *); p; p = va_arg(args, char *))
  {
    int slen = strlen(p);

    rv = tld_realloc(DBGpass(rv), *len + slen + 1);
    strcpy(rv + *len, p);
    *len += slen;
  }

  va_end(args);

  return rv;
}

/*
 * Append a byte section of length 'len' to the 'tdata' data block, which is
 * already of size '*tlen'.
 *
 * 'tdata' MAY be NULL (*tlen will be zero(0) then).
 *
 * Return the reallocated/resized concatenated data block.
 *
 * The length of the concatened block will be added to the
 * value referenced by 'len'.
 *
 * CAVEATS:
 *
 * - 'tdata' must be allocated on the heap using malloc/strdup.
 *
 * - 'tdata' WILL be pointing to invalid memory, due to the
 *   possibility of a realloc() occurring within this routine.
 *
 * - '*tlen' is used as the initial allocated size of the 'tdata'
 *   block. Failure to comply to provide this number by the caller
 *       will result in incorrectly realloc()ed memory.
 *
 *       When 'tdata' is NULL, *tlen may be zero(0) too.
 */
char *tl_data_concat_data(int *tlen, char *tdata, int len, const char *data)
{
  tdata = _realloc(tdata, *tlen + len);
  memcpy(tdata + *tlen, data, len);
  *tlen += len;

  return tdata;
}

/*
 * Append the printf(fmt, ...) output into string 'str1'.
 *
 * This routine can handle printf() output of arbitrary size.
 *
 * str1 MAY be NULL.
 *
 * Return the reallocated/resized concatenated result string.
 *
 * CAVEATS:
 *
 * - str1 must be allocated on the heap using malloc/strdup.
 *
 * - str1 WILL be pointing to invalid memory, due to the
 *   possibility of a realloc() occurring within this routine.
 */
char *tl_str_printf(char *str1, const char *fmt, ...)
{
  va_list args;
  int len;
  int plen;
  int extra_len = 4096;
  char *rv = str1;

  ASSERT(fmt != NULL);
  len = (str1 ? strlen(str1) : 0);
  rv = _realloc(rv, len + extra_len);
  for (;;)
  {
    va_start(args, fmt);
    plen = vsnprintf(rv + len, extra_len - 1, fmt, args);
    va_end(args);
    /* did string fit in allocated space? If yes, it's ok: exit loop */
    if (plen >= 0 && plen < extra_len - 2)
      break;
    extra_len *= 2;
    if (extra_len >= INT_MAX / 2)
      break;
  }
  rv[len + extra_len - 1] = 0;

  return rv;
}

char *tl_load_text_file(const char *filename)
{
  char pom[1024];
  int tlen, len, fd;
  char *rv = NULL;

  if ((fd =
         tl_open(filename, O_BINARY | O_RDONLY,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) < 0)
  {
    xperror(mk_native(filename));
    return NULL;
  }

  tlen = 0;

  while ((len = read(fd, pom, sizeof(pom))) > 0)
  {
    rv = tl_data_concat_data(&tlen, rv, len, pom);
  }

  close(fd);

  if (rv)
  {
    rv = _realloc(rv, tlen + 1);
    rv[tlen] = 0;
  }

  return rv;
}

int tl_save_text_file(const char *filename, const char *content, int length)
{
  int fd;
  int rv = 0;

  if (length < 0)
    length = strlen(content);

  if ((fd =
         tl_open(filename, O_BINARY | O_WRONLY | O_CREAT | O_TRUNC,
                 S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR)) < 0)
  {
    xperror(mk_native(filename));
    return -1;
  }

  if (write(fd, content, length) != length)
  {
    xperror(mk_native(filename));
    rv = -1;
  }

  close(fd);

  return rv;
}


/*
 * return 'dst'; use max 'dstsize' characters in the destination buffer
 */
char *tl_hexdump_content(char *dst, size_t dstsize, const void *content, size_t content_length)
{
  ASSERT(dst != NULL);
  if (content && content_length)
  {
    const unsigned char *s = (const unsigned char *)content;
    char *d = dst;
    int si = content_length;
    int dni = dstsize - 1;
    for ( ; si > 0 && dni > 0;)
    {
      int li = 0;
      char buf[80];
      sprintf(buf, "%08X ", (int)(s - (const unsigned char *)content));
      for ( ; li < 16 && si > 0 && dni > 0; li++, s++, si--)
      {
        sprintf(buf + li * 3 + 9, "%02X ", *s);
        if (*s && tl_ascii_isprint(*s))
        {
          buf[li + 3 * 16 + 2 + 9] = *s;
        }
        else
        {
          buf[li + 3 * 16 + 2 + 9] = '.';
        }
      }
      memset(buf + li * 3 + 9, ' ', 3 * 16 + 2 - li * 3);       /* fix NUL byte here from last sprintf */
      buf[3 * 16 + 9] = '|';
      strcpy(buf + li + 3 * 16 + 2 + 9, (si > 0 && dni > strlen(buf) ? "\n" : ""));
      strncpy(d, buf, dni + 1);
      d[dni] = 0;
      dni -= strlen(d);
      d += strlen(d);
    }
  }
  else if (!content)
  {
    snprintf(dst, dstsize, "(NULL buffer / length: %d)",
             (int)content_length);
  }
  else
  {
    snprintf(dst, dstsize, "(buffer @ %p / length: %d)", content,
             (int)content_length);
  }
  return dst;
}

/*
 * return 'dst'; use max 'dstsize' characters in the destination buffer
 */
char *tl_asciidump_content(char *dst, size_t dstsize, const char *content, size_t content_length)
{
  ASSERT(dst != NULL);
  if (content && content_length)
  {
    const char *s = content;
    char *d = dst;
    int si = content_length;
    int dni = dstsize - 1;
    for ( ; si > 0 && dni > 0; s++, si--, dni--)
    {
      if (*s && tl_ascii_isprint(*s))
      {
        *d++ = *s;
      }
      else
      {
        switch (*s)
        {
        case '\t':
        case '\r':
        case '\n':
          *d++ = *s;
          break;

        default:
          *d++ = '.';
          break;
        }
      }
    }
    *d = 0;
  }
  else if (!content)
  {
    snprintf(dst, dstsize, "(NULL buffer / length: %d)",
             (int)content_length);
  }
  else
  {
    snprintf(dst, dstsize, "(buffer @ %p / length: %d)", (void *)content,
             (int)content_length);
  }
  return dst;
}



#ifdef HAVE_MT
RETSIGTYPE pavuk_sigintthr(int nr)
{
#ifdef I_FACE
  if (!cfg.processing)
  {
    exit(0);
  }
#endif
  errno = EINTR;
  cfg.stop = TRUE;
  cfg.rbreak = TRUE;
  DEBUG_DEVEL("cfg.stop = TRUE @ %s, line #%d\n", __FILE__, __LINE__);
  xprintf(0, "INT signal received. Aborting application.\n");
#if !defined (RETSIGTYPE_IS_VOID)
  return 1;

#endif
}

#ifdef SIGQUIT
RETSIGTYPE pavuk_sigquitthr(int nr)
{
  errno = EINTR;
  cfg.stop = TRUE;
  cfg.rbreak = TRUE;
  DEBUG_DEVEL("cfg.stop = TRUE @ %s, line #%d\n", __FILE__, __LINE__);
  xprintf(0, "QUIT signal received. Aborting application.\n");
#if 0
  pthread_exit(NULL);
#endif
#if !defined (RETSIGTYPE_IS_VOID)
  return 1;

#endif
}
#endif

#ifdef SIGXFSZ
RETSIGTYPE pavuk_sigfilesizethr(int nr)
{
#if 0
  errno = EINTR;
  cfg.stop = TRUE;
  cfg.rbreak = TRUE;
#endif
#if 0
  errno = EFBIG;
#endif
  xprintf(0,
          gettext(
            "SIGXFSZ signal received: file size exceeded. YMMV from here, but we'll go on if we can!\n"));
  DEBUG_TRACE("SIGXFSZ @ %s, line #%d\n", __FILE__, __LINE__);
#if 0
  pthread_exit(NULL);
#endif
#if !defined (RETSIGTYPE_IS_VOID)
  return 1;

#endif
}
#endif
#endif
RETSIGTYPE pavuk_intsig(int signum)
{
#if defined (I_FACE)
  cfg.xi_face = FALSE;
#endif
  cfg.rbreak = TRUE;
  cfg.stop = TRUE;
  errno = EINTR;
  xprintf(0, gettext("INT signal received. Aborting application.\n"));
  DEBUG_DEVEL("cfg.stop = TRUE @ %s, line #%d\n", __FILE__, __LINE__);
#if !defined (RETSIGTYPE_IS_VOID)
  return 1;

#endif
}

#ifdef SIGQUIT
RETSIGTYPE pavuk_quitsig(int signum)
{
#if defined (I_FACE)
  cfg.xi_face = FALSE;
#endif
  cfg.rbreak = TRUE;
  cfg.stop = TRUE;
  errno = EINTR;
  xprintf(0, gettext("QUIT signal received. Aborting application.\n"));
  DEBUG_DEVEL("cfg.stop = TRUE @ %s, line #%d\n", __FILE__, __LINE__);
#if !defined (RETSIGTYPE_IS_VOID)
  return 1;

#endif
}
#endif

#ifdef SIGXFSZ
RETSIGTYPE pavuk_filesizesig(int signum)
{
#if 0
#if defined (I_FACE)
  cfg.xi_face = FALSE;
#endif
  cfg.rbreak = TRUE;
  cfg.stop = TRUE;
#endif
#if 0
  errno = EFBIG;
#endif
  xprintf(0,
          gettext
          (
            "SIGXFSZ signal received: file size exceeded. YMMV from here, but we'll go on if we can!\n"));
  DEBUG_TRACE("SIGXFSZ @ %s, line #%d\n", __FILE__, __LINE__);
#if !defined (RETSIGTYPE_IS_VOID)
  return 1;

#endif
}
#endif


void tl_signal(int signum, sighandler_t handler)
{
#if defined (HAVE_SIGACTION)
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handler;
  sigaction(signum, &sa, NULL);
#else
  signal(signum, handler);
#endif
}


char *tl_mktempfname(const char *fn, const char *prefix)
{
  char *pom;
  int len;
  char *p;
  int count = 1024;       /* max number of attempts... */

  if (!fn)
  {
    xperror("Invalid argument for mktempfname()");
    return NULL;
  }

  len = strlen(fn) + strlen((prefix ? prefix : "")) + strlen(PAVUK_TEMPFNAME_PREFIX) + 25;
  pom = _malloc(len);
  if (!pom)
  {
    xperror("Out of memory in mktempfname()");
  }

  strcpy(pom, fn);
  p = strrchr(pom, '/');
  for ( ; count > 0; count--)
  {
    struct stat st;
    int stat_ret;

    snprintf((p ? p + 1 : pom), (p ? len - (p - pom) : len), PAVUK_TEMPFNAME_PREFIX "%s%d",
             (prefix ? prefix : ""), (int)(getpid() + rand()));
    pom[len - 1] = 0;

    stat_ret = tl_lstat(pom, &st);
    if (stat_ret != 0 && errno == ENOENT)
    {
      // found a nonexisting file!
      return pom;
    }
  }
  return NULL;
}



#ifndef HAVE__SET_ERRNO
void _set_errno(int errcode)
{
  errno = errcode;
}
#endif




#undef _malloc
#undef _realloc
#undef _free

#undef tl_strdup
#undef tl_strndup

#if defined (DEBUG)
void *tld_malloc(const char *sourcefile, int sourceline, size_t len)
#else
void *_malloc(size_t len)
#endif
{
#if (defined (WIN32) || defined (__WIN32)) && defined (_CRTDBG_MAP_ALLOC)
  void *ret = _malloc_dbg(len, _NORMAL_BLOCK, sourcefile, sourceline);
#else
  void *ret = malloc(len);
#endif
  if (!ret)
  {
    xperror("malloc");
    ASSERT(0);
    abort();
  }
#if defined (DEBUG)
  memset(ret, 0xA5, len);       /* [i_a] sanity check */
#endif
  return ret;
}

#if defined (DEBUG)
void *tld_realloc(const char *sourcefile, int sourceline, void *ptr,
                  size_t len)
#else
void *_realloc(void *ptr, size_t len)
#endif
{
#if (defined (WIN32) || defined (__WIN32)) && defined (_CRTDBG_MAP_ALLOC)
  void *ret = (ptr
               ? _realloc_dbg(ptr, len, _NORMAL_BLOCK, sourcefile, sourceline)
               : _malloc_dbg(len, _NORMAL_BLOCK, sourcefile, sourceline));
#else
  void *ret = ptr ? realloc(ptr, len) : malloc(len);
#endif
  if (!ret)
  {
    xperror("realloc");
    ASSERT(0);
    abort();
  }
  return ret;
}

#if defined (DEBUG)
void tld_free(const char *sourcefile, int sourceline, void *ptr)
{
#if (defined (WIN32) || defined (__WIN32)) && defined (_CRTDBG_MAP_ALLOC)
  _free_dbg(ptr, _NORMAL_BLOCK);
#else
  free(ptr);
#endif
}
#endif
/**********************/
/* duplicate string   */
/**********************/
#if defined (DEBUG)
char *tld_strdup(const char *sourcecode_file, int sourcecode_line,
                 const char *s)
#else
char *tld_strdup(const char *s)
#endif
{
  char *p;

  if (s)
  {
#if (defined (WIN32) || defined (__WIN32)) && defined (_CRTDBG_MAP_ALLOC)
    p =
      (char *)_malloc_dbg(strlen(s) + 1, _NORMAL_BLOCK, sourcecode_file,
                          sourcecode_line);
#else
    p = (char *)tld_malloc(DBGpass(strlen(s) + 1));
#endif
    strcpy(p, s);
  }
  else
  {
    p = NULL;
  }
  return p;
}

/****************************************/
/* duplicate n characters from string   */
/****************************************/
#if defined (DEBUG)
char *tld_strndup(const char *sourcecode_file, int sourcecode_line,
                  const char *s, int l)
#else
char *tl_strndup(const char *s, int l)
#endif
{
  char *p;

  if (!s)
    return NULL;

#if (defined (WIN32) || defined (__WIN32)) && defined (_CRTDBG_MAP_ALLOC)
  p =
    (char *)_malloc_dbg(l + 1, _NORMAL_BLOCK, sourcecode_file,
                        sourcecode_line);
#else
  p = (char *)tld_malloc(DBGpass(l + 1));
#endif
  if (l)
  {
    strncpy(p, s, l);
  }
  *(p + l) = 0;
  return p;
}


