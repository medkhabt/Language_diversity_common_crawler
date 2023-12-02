/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "url.h"
#include "doc.h"
#include "tools.h"
#include "log.h"
#include "errcode.h"

#if 0
static void _log_str(const char *);
#endif

#ifndef HAVE_DPRINTF

#if 0
int dprintf(int fd, const char *format, ...)
{
  int n;
  va_list ap;
  static char buffer[1024];
  va_start(ap, format);
#ifdef HAVE_VSNPRINTF
  n = vsnprintf(buffer, sizeof(buffer), format, ap);
#elif defined(HAVE__VSNPRINTF)
  n = _vsnprintf(buffer, sizeof(buffer), format, ap);
#else
#error you must implement bufio_printf() for your platform
#endif
  buffer[sizeof(buffer) - 1] = 0;
  ASSERT(strlen(buffer) < NUM_ELEM(buffer));
  va_end(ap);
  return write(fd, buffer, n);
}
#endif

#endif

const char *errcodetype(int ecode)
{
  switch (ecode)
  {
  case ERR_NOERROR:
    return "OK";
  case ERR_STORE_DOC:
  case ERR_FILE_OPEN:
  case ERR_DIR_URL:
  case ERR_UNKNOWN:
  case ERR_PROXY_CONNECT:
  case ERR_FTP_UNKNOWN:
  case ERR_FTP_BUSER:
  case ERR_FTP_BPASS:
  case ERR_HTTP_UNKNOWN:
  case ERR_HTTP_AUTH:
  case ERR_HTTP_PAY:
  case ERR_HTTP_BADRQ:
  case ERR_HTTP_FORB:
  case ERR_HTTP_SERV:
  case ERR_GOPHER_UNKNOWN:
    return "FATAL";
  case ERR_LOCKED:
  case ERR_BIGGER:
  case ERR_NOMIMET:
  case ERR_BREAK:
  case ERR_OUTTIME:
  case ERR_SCRIPT_DISABLED:
  case ERR_SMALLER:
  case ERR_ZERO_SIZE:
  case ERR_PROCESSED:
  case ERR_UDISABLED:
  case ERR_RDISABLED:
  case ERR_FTP_NOREGET:
  case ERR_FTP_ACTUAL:
  case ERR_FTP_NOTRANSFER:
  case ERR_FTP_NOMDTM:
  case ERR_FTP_DIRNO:
  case ERR_HTTP_NOREGET:
  case ERR_HTTP_REDIR:
  case ERR_HTTP_ACTUAL:
    return "WARN";
  case ERR_READ:
  case ERR_FTP_BDIR:
  case ERR_FTP_CONNECT:
  case ERR_FTP_DATACON:
  case ERR_FTP_GET:
  case ERR_FTP_NODIR:
  case ERR_FTP_TRUNC:
  case ERR_HTTP_CONNECT:
  case ERR_HTTP_SNDREQ:
  case ERR_HTTP_TRUNC:
  case ERR_HTTP_CYCLIC:
  case ERR_HTTP_NFOUND:
  case ERR_GOPHER_CONNECT:
  case ERR_HTTPS_CONNECT:
    return "ERR";
  default:
    return "UNKNOWN";
  }
}

static bool_t shortlog_header_written = FALSE;

static void _short_log_header(bufio * fd, const char *msgstr)
{
  if(!bufio_is_open(fd))
    return;

  if(msgstr && *msgstr)
  {
    bufio_puts(fd, msgstr);
  }
  bufio_puts(fd, "PID\tCURRENT TIME\tDOC#/TOTAL#\tERR TYPE\tERR CODE\tURL\tPARENT URL\tFILENAME\tDOC SIZE\tDOC ETIME\tMIME\n");
  shortlog_header_written = TRUE;

  ASSERT(bufio_is_open(fd));
}

void short_log_header(const char *msgstr)
{
  if(!bufio_is_open(cfg.short_logfile))
    return;

#ifdef HAVE_MT
  ASSERT(cfg.short_logfile->mutex);
#endif

  _short_log_header(cfg.short_logfile, msgstr);

  ASSERT(bufio_is_open(cfg.short_logfile));
}

void short_log(doc * docp, url * urlp, const char *urlstr)
{
  char *p, *p1;
  time_t t = time(NULL);
  bufio *fd;

  if(!bufio_is_open(cfg.short_logfile))
    return;

#ifdef HAVE_MT
  ASSERT(cfg.short_logfile->mutex);
#endif
  VERIFY(!bufio_lock(cfg.short_logfile, FALSE, &fd));

  if(!shortlog_header_written)
  {
    _short_log_header(fd, NULL);
  }

  LOCK_TIME;
  p1 = ctime(&t);
  p = strchr(p1, '\n');
  if(p)
    *p = '\0';

  bufio_printf(fd, "%d\t%s\t%d/%d\t%s\t%d\t", (int) getpid(),
    p1, docp->doc_nr, cfg.total_cnt, errcodetype(docp->errcode), docp->errcode);
  UNLOCK_TIME;

  if(urlp)
  {
    p = url_to_urlstr(urlp, FALSE);
    bufio_puts(fd, p);
    _free(p);
  }
  else
  {
    bufio_puts(fd, (urlstr ? urlstr : "[none]"));
  }

  if(urlp && urlp->parent_url)
  {
    bufio_puts(fd, "\t");
    LOCK_URL(urlp);
    p = url_to_urlstr((url *) urlp->parent_url->data, FALSE);
    UNLOCK_URL(urlp);
    bufio_puts(fd, p);
    _free(p);
  }
  else
  {
    bufio_puts(fd, "\t[none]");
  }

  bufio_puts(fd, "\t");
  if(urlp)
  {
    p = url_to_filename(urlp, FALSE);
    bufio_puts(fd, p);
  }
  else
  {
    bufio_puts(fd, /* (urlstr ? urlstr : */ "[no file]");
  }

  bufio_printf(fd, "\t%d", docp->size);

  t = doc_etime(docp, FALSE);
  bufio_printf(fd, "\t%ld.%03ld", t / 1000, t % 1000);

  if(docp->mime)
  {
    int l = strcspn(docp->mime, "\r\n");
    bufio_puts(fd, "\t");
    bufio_write(fd, docp->mime, l);
  }

  bufio_puts(fd, "\n");

  VERIFY(!bufio_release(cfg.short_logfile, &fd));
  ASSERT(bufio_is_open(cfg.short_logfile));
}

static int time_relative_object(void)
{
  return cfg.time_relative && !strcmp(cfg.time_relative, "object");
}

static int time_relative_program(void)
{
  return cfg.time_relative && !strcmp(cfg.time_relative, "program");
}

/* print fixed point values: unit: 1/1000 */
static int log_num2(bufio * fd, const char *name, int width, long num)
{
  int rv;
  int w = width - 2;            /* take the '.' into account: we only have place for 'w' digits to display */
  int pwr = 10;
  long val = 1000000000L;
  int frag = 3;                 /* unit: 1/1000th */
  int neg = (num < 0);
  int i;
  long num_orig = num;
  if(neg)
  {
    num = -num;
    w--;
  }
  while(num < val)
  {
    val /= 10;
    pwr--;
  }
  /* adjust number */
  if(pwr > w)
  {
    /* show only most significant part of number */
    while(pwr > w)
    {
      pwr--;
      num /= 10;
      frag--;
    }
  }
  /* now we know value will fit in 'w' digits; determine location of decimal point */
  if(frag > 0)
  {
    long v1, v2;

    val = 1;
    for(i = 0; i < frag; i++)
      val *= 10;

    v1 = num / val;
    v2 = num % val;
    rv = bufio_printf(fd, " %s%*lu.%0*lu", (neg ? "-" : ""), w - frag, v1, frag, v2);
  }
  else
  {
    /* we cannot display the decimal part, but we FORCE the display of the complete integer value! */
    long v1;

    v1 = num_orig / 1000;
    rv = bufio_printf(fd, " %*ld", width - 1, v1);
  }
  return rv;
}

static int log_num(bufio * fd, const char *name, int width, long num)
{
  int rv = -1;
  /* we print space before the number, but decrease
   * width to ensure there is always at least
   * one space before the number
   */
  rv = bufio_printf(fd, " %*ld", width - 1, num);
  return rv;
}

static int log_time(bufio * fd, const char *name, int width, doc * docp, const struct timeval *end, const struct timeval *begin)
{
  long time_diff = -1;
  const struct timeval *relative = begin;

  if(time_relative_object())
  {
    relative = &docp->hr_start_time;
  }
  else if(time_relative_program())
  {
    relative = &cfg.hr_start_time;
  }

  if(timerisset(end) && timerisset(begin) && timerisset(relative))
  {
    time_diff = (long) ((end->tv_sec - relative->tv_sec) * 1000 + (end->tv_usec - relative->tv_usec) / 1000.0);
    if(!cfg.sdemo_mode && (time_diff < 100) && (time_diff > -100))
    {
      time_diff = (end->tv_sec - relative->tv_sec) * 1000000L + (end->tv_usec - relative->tv_usec);
      return log_num2(fd, name, width, time_diff);
    }
    else
    {
      return log_num(fd, name, width, time_diff);
    }
  }
  else
  {
    if(cfg.sdemo_mode)
    {
      return log_num(fd, name, width, time_diff);
    }
    else
    {
      return bufio_printf(fd, " %*s", width - 1, "*");
    }
  }
}


static int timelog_header_printed = 0;
static int time_width = 6;
static int size_width = 8;
/*
 * The following are not enough in all cases, but the space
 * is just too precious in common case to care
 */
static int result_width = sizeof(" HTTP/1.1 302 Moved Temporarily                   ");


static int determine_time_width(void)
{
  time_width = 8;
  if(time_relative_object())
  {
    time_width = 6;
  }
  else if(time_relative_program())
  {
    time_width = 9;
  }
  return time_width;
}


static void write_timelog_fileheader(bufio * fd, int result_width)
{
  time_width = determine_time_width();
  bufio_printf(fd, "%*s", 5, "TEST");
  bufio_printf(fd, "%*s", 6, "THR");
  bufio_printf(fd, "%*s", 7, "RUN");
  bufio_printf(fd, "%*s", 7, "ITEM");
  bufio_printf(fd, "%*s", 5, "MODE");
  bufio_printf(fd, "%*s", time_width, "START");
  bufio_printf(fd, "%*s", time_width, "END");
  bufio_printf(fd, "%*s", time_width, "DNS");
  bufio_printf(fd, "%*s", time_width, "CONN");
  bufio_printf(fd, "%*s", time_width, "FB");
  bufio_printf(fd, "%*s", time_width, "LB");
  bufio_printf(fd, "%*s", time_width, "TOTAL");
  bufio_printf(fd, "%*s", size_width, "SIZE");
  bufio_printf(fd, " %-*s", result_width - 1, "RESULT");
  bufio_printf(fd, " %-*s", result_width - 1, "CLIENT");

  bufio_printf(fd, " %s\n", "URL");

  timelog_header_printed = 1;
}



/*** Logs timings information. Modelled after short_log() above ***/
void time_log(doc * docp, int mode, const char *urlstr)
{
  char *p;
  char pom[1024] = "\0";
  bufio *fd;

  if(!bufio_is_open(cfg.time_logfile))
    return;

#ifdef HAVE_MT
  ASSERT(cfg.time_logfile->mutex);
#endif
  VERIFY(!bufio_lock(cfg.time_logfile, FALSE, &fd));

  /* Log file is locked. No race here */
  if(!timelog_header_printed)
  {
    write_timelog_fileheader(fd, result_width);
    ASSERT(timelog_header_printed);
  }

  ASSERT(docp != NULL);
  bufio_printf(fd, "%*.*s", 5, 5, cfg.test_id); /* [i_a] test STRING; used to identify the test */
#ifdef HAVE_MT
  bufio_printf(fd, "%*d", 6, docp->threadnr);   /* [i_a] */
#endif
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  bufio_printf(fd, "%*d", 7, docp->cyclenr);    /* [i_a]  */
#endif
  bufio_printf(fd, "%*d", 7, docp->doc_nr);     /* [i_a]  */
  bufio_printf(fd, "%*d", 5, mode);     /* [i_a] collect/record mode */
  log_time(fd, "START", time_width, docp, &docp->hr_start_time, &cfg.hr_start_time);
  log_time(fd, "END", time_width, docp, &docp->end_time, &cfg.hr_start_time);
  log_time(fd, "DNS", time_width, docp, &docp->dns_time, &docp->hr_start_time);
  log_time(fd, "CONN", time_width, docp, &docp->connect_time, &docp->dns_time);
  log_time(fd, "FB", time_width, docp, &docp->first_byte_time, &docp->connect_time);
  log_time(fd, "LB", time_width, docp, &docp->end_time, &docp->first_byte_time);
  log_time(fd, "TOTAL", time_width, docp, &docp->end_time, &docp->hr_start_time);

  log_num(fd, "SIZE", size_width, docp->size);

  strcpy(pom, "---");
  ASSERT(strlen(pom) < sizeof(pom));
  if(docp->mime)
  {
    int len = strcspn(docp->mime, "\r\n");
    if(len > result_width - 1)
      len = result_width - 1;
    ASSERT(len < sizeof(pom));
    if(len)
    {
      char *d;
      strncpy(pom, docp->mime, len);
      *(pom + len) = '\0';
      /*
         [i_a] this is also used in the hammer section where 'mime' may point at BINARY data
         due to misbehaving MIME detectors: use ASCIIDUMP translation style to keep the output
         from getting clobbered with weird characters.
       */
      for(d = pom; *d; d++)
      {
        if(tl_ascii_isprint(*d))
          continue;
        *d = '.';
      }
    }
  }
  bufio_printf(fd, " %-*s", result_width - 1, pom);

  {
    char *mstr = doc_strerror(docp);
    snprintf(pom, sizeof(pom), "%s:%s", errcodetype(docp->errcode), mstr);
    pom[sizeof(pom) - 1] = 0;
    _free(mstr);
  }
  bufio_printf(fd, " %-*s", result_width - 1, pom);

  if(docp->doc_url)
  {
    p = url_to_urlstr(docp->doc_url, FALSE);
    ASSERT(p);
    bufio_printf(fd, " %s\n", p);
    _free(p);
  }
  else
  {
    bufio_printf(fd, " %s\n", (urlstr ? urlstr : ""));
  }

  VERIFY(!bufio_release(cfg.time_logfile, &fd));
  ASSERT(bufio_is_open(cfg.time_logfile));
}


void time_log_header(int mode, const char *str)
{
  bufio *fd;

  if(!bufio_is_open(cfg.time_logfile))
  {
    return;
  }
#ifdef HAVE_MT
  ASSERT(cfg.time_logfile->mutex);
#endif
  VERIFY(!bufio_lock(cfg.time_logfile, FALSE, &fd));

  /* Log file is locked. No race here */
  if(!timelog_header_printed)
  {
    write_timelog_fileheader(fd, result_width);
    ASSERT(bufio_is_open(fd));
    ASSERT(timelog_header_printed);
  }

  bufio_printf(fd, "%*d", 5, mode);     /* [i_a] collect/record mode */
  bufio_printf(fd, "%*s", time_width, "*");
  bufio_printf(fd, "%*s", time_width, "*");
  bufio_printf(fd, "%*s", time_width, "*");
  bufio_printf(fd, "%*s", time_width, "*");
  bufio_printf(fd, "%*s", time_width, "*");
  bufio_printf(fd, "%*s", time_width, "*");
  bufio_printf(fd, "%*s", time_width, "*");
  bufio_printf(fd, "%*s", size_width, "*");
  bufio_printf(fd, " %-*s", result_width - 1, "*****");
  bufio_printf(fd, " %-*s", result_width - 1, "*****");

  bufio_printf(fd, " %s\n", str);

  VERIFY(!bufio_release(cfg.time_logfile, &fd));
  ASSERT(bufio_is_open(cfg.time_logfile));
}


static void write_timelog4sum_fileheader(bufio * fd, int result_width)
{
  time_width = determine_time_width();
  bufio_printf(fd, "%*s", 5, "TEST");
  bufio_printf(fd, "%*s", 5, "THR");
  bufio_printf(fd, "%*s", 5, "RUN");
  bufio_printf(fd, "%*s", 5, "MODE");
  bufio_printf(fd, "%*s", time_width, "START");
  bufio_printf(fd, "%*s", time_width, "END");
  bufio_printf(fd, "%*s", time_width, "DNS");
  bufio_printf(fd, "%*s", time_width, "CONN");
  bufio_printf(fd, "%*s", time_width, "FB");
  bufio_printf(fd, "%*s", time_width, "LB");
  bufio_printf(fd, "%*s", time_width, "TOTAL");
  bufio_printf(fd, "%*s", size_width, "SIZE");
  bufio_printf(fd, " %-*s", result_width - 1, "RESULT");
  bufio_printf(fd, " %-*s", result_width - 1, "CLIENT");

  bufio_printf(fd, " %s\n", "URL");

  timelog_header_printed = 1;
}



void time_log4sum(doc * docp, int mode, const char *urlstr)
{
  char *p;
  char pom[1024] = "\0";
  bufio *fd;

  if(!bufio_is_open(cfg.time_logfile4sum))
    return;

#ifdef HAVE_MT
  ASSERT(cfg.time_logfile4sum->mutex);
#endif
  VERIFY(!bufio_lock(cfg.time_logfile4sum, FALSE, &fd));

  /* Log file is locked. No race here */
  if(!timelog_header_printed)
  {
    write_timelog4sum_fileheader(fd, result_width);
    ASSERT(timelog_header_printed);
  }

  ASSERT(docp != NULL);
  bufio_printf(fd, "%*.*s", 5, 5, cfg.test_id); /* [i_a] test STRING; used to identify the test */
#ifdef HAVE_MT
  bufio_printf(fd, "%*d", 5, docp->threadnr);   /* [i_a] */
#endif
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  bufio_printf(fd, "%*d", 5, docp->cyclenr);    /* [i_a]  */
#endif
  bufio_printf(fd, "%*d", 5, mode);     /* [i_a] collect/record mode */
  log_time(fd, "START", time_width, docp, &docp->hr_start_time, &cfg.hr_start_time);
  log_time(fd, "END", time_width, docp, &docp->end_time, &cfg.hr_start_time);
  log_time(fd, "DNS", time_width, docp, &docp->dns_time, &docp->hr_start_time);
  log_time(fd, "CONN", time_width, docp, &docp->connect_time, &docp->dns_time);
  log_time(fd, "FB", time_width, docp, &docp->first_byte_time, &docp->connect_time);
  log_time(fd, "LB", time_width, docp, &docp->end_time, &docp->first_byte_time);
  log_time(fd, "TOTAL", time_width, docp, &docp->end_time, &docp->hr_start_time);

  log_num(fd, "SIZE", size_width, docp->size);

  strcpy(pom, "---");
  ASSERT(strlen(pom) < sizeof(pom));
  if(docp->mime)
  {
    int len = strcspn(docp->mime, "\r\n");
    if(len > result_width - 1)
      len = result_width - 1;
    ASSERT(len < sizeof(pom));
    if(len)
    {
      char *d;
      strncpy(pom, docp->mime, len);
      *(pom + len) = '\0';
      /*
         [i_a] this is also used in the hammer section where 'mime' may point at BINARY data
         due to misbehaving MIME detectors: use ASCIIDUMP translation style to keep the output
         from getting clobbered with weird characters.
       */
      for(d = pom; *d; d++)
      {
        if(tl_ascii_isprint(*d))
          continue;
        *d = '.';
      }
    }
  }
  bufio_printf(fd, " %-*s", result_width - 1, pom);

  {
    char *mstr = doc_strerror(docp);
    snprintf(pom, sizeof(pom), "%s:%s", errcodetype(docp->errcode), mstr);
    pom[sizeof(pom) - 1] = 0;
    _free(mstr);
  }
  bufio_printf(fd, " %-*s", result_width - 1, pom);

  if(docp->doc_url)
  {
    p = url_to_urlstr(docp->doc_url, FALSE);
    ASSERT(p);
    bufio_printf(fd, " %s\n", p);
    _free(p);
  }
  else
  {
    bufio_printf(fd, " %s\n", (urlstr ? urlstr : ""));
  }

  VERIFY(!bufio_release(cfg.time_logfile4sum, &fd));
  ASSERT(bufio_is_open(cfg.time_logfile4sum));
}


void time_log4sum_header(int mode, const char *str)
{
  bufio *fd;

  if(!bufio_is_open(cfg.time_logfile4sum))
  {
    return;
  }
#ifdef HAVE_MT
  ASSERT(cfg.time_logfile4sum->mutex);
#endif
  VERIFY(!bufio_lock(cfg.time_logfile4sum, FALSE, &fd));

  /* Log file is locked. No race here */
  if(!timelog_header_printed)
  {
    write_timelog_fileheader(fd, result_width);
    ASSERT(bufio_is_open(fd));
    ASSERT(timelog_header_printed);
  }

  bufio_printf(fd, "%*d", 5, mode);     /* [i_a] collect/record mode */
  bufio_printf(fd, "%*s", time_width, "*");
  bufio_printf(fd, "%*s", time_width, "*");
  bufio_printf(fd, "%*s", time_width, "*");
  bufio_printf(fd, "%*s", time_width, "*");
  bufio_printf(fd, "%*s", time_width, "*");
  bufio_printf(fd, "%*s", time_width, "*");
  bufio_printf(fd, "%*s", time_width, "*");
  bufio_printf(fd, "%*s", size_width, "*");
  bufio_printf(fd, " %-*s", result_width - 1, "*****");
  bufio_printf(fd, " %-*s", result_width - 1, "*****");

  bufio_printf(fd, " %s\n", str);

  VERIFY(!bufio_release(cfg.time_logfile4sum, &fd));
  ASSERT(bufio_is_open(cfg.time_logfile4sum));
}



static void log_mark(const char *str1, const char *str2)
{
  if(bufio_is_open(cfg.logfile))
  {
    char pom[1024];
    time_t t = time(NULL);
    LOCK_TIME;
    strftime(pom, sizeof(pom), gettext("%%s%%s: %H:%M:%S %d.%m.%Y\n"), localtime(&t));
    UNLOCK_TIME;
    bufio_printf(cfg.logfile, pom, str1, str2);
  }
}


void log_start(const char *str)
{
  log_mark(gettext("Starting log "), str);
}


void log_end(void)
{
  log_mark(gettext("Ending log "), "");
}


void log_str(const char *str)
{
/* WARNING/FIXME: cannot detect deadlock by recursive calling; watch your six! */
  if(bufio_is_open(cfg.logfile))
  {
    bufio_puts(cfg.logfile, str);
  }
}

#if 0
//void log_str(char *str)
//{
//  int ret = TRYLOCK_LOG;
//  if(!ret)
//  {
//    _log_str(str);
//    UNLOCK_LOG;
//  }
//#if DEBUG_DEADLOCK              /* NOT #ifdef! */
//  else
//  {
//    printf("\nlog_str() - DEADLOCK PREVENTED: msg = '%s'\n", str);
//  }
//#endif
//}
#endif



void init_page_stats(page_statistics_t * d)
{
  memset(d, 0, sizeof(*d));
  ASSERT(!timerisset(&d->start_time));
  ASSERT(!timerisset(&d->end_time));

#if 0
  timerclear(&d->total_wait_time);
  timerclear(&d->total_timeout_time);
#endif
}

void init_summary_stats(summary_statistics_t * d)
{
  memset(d, 0, sizeof(*d));
  ASSERT(!timerisset(&d->start_time));
  ASSERT(!timerisset(&d->end_time));

#if 0
  timerclear(&d->total_wait_time);
  timerclear(&d->total_timeout_time);
#endif
}
