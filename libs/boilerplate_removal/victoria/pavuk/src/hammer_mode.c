/*
   additions for replay web attack mode (hammer_mode 1)
 */
/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#ifdef INCLUDE_CHUNKY_DoS_FEATURES

#include "dllist.h"
#include "errcode.h"
#include "absio.h"
#include "gui_api.h"
#include "log.h"
#include "hammer_mode.h"

/*
   record activity during initial run; then replay those requests brute force
   in an attempt to get the server on its knees.

   Assumptions:

   This assumes 'sessions' and other time/connection-limited cookies and parameters
   remain valid for slightly longer than the initial web request; it also assumes
   session/auth codes (cookies, etc.) are NOT changed during a session; and last but
   not least this code assumes we can ride on in through slightly aged sessions, which,
   through our recurring requests, are kept 'alive' for a longer time still.
 */


/* -hammer_flags bits: */
#define HAMMER_FLAG_PERSISTENT_CONNECTION                                       0x0001
#define HAMMER_FLAG_BOGUS_REQ_FOLLOWS_ON_PERSISTENT_CONNECTION                  0x0002
#define HAMMER_FLAG_HALF_CLOSE                                                  0x0004
#define HAMMER_FLAG_AUTO_EASE_OFF                                               0x0008
#define HAMMER_FLAG_BOGUS_AFTER_SILENT_TIME                                     0x0010
#define HAMMER_FLAG_NO_TIME_NICETIES                                            0x0020
#define HAMMER_FLAG_PERSISTENT_DOS                                              0x0040
#define HAMMER_FLAG_NO_READ_AFTER_BOGUS                                         0x0080
#define HAMMER_FLAG_PUSH_BOGUS                                                  0x0100




#if 0
#define HAMMER_BOGUS_REQUEST                            \
        "GET /xyz HTTP/1.0\r\n"                         \
        "\r\n"                                          \
        "\r\n"                                          \
        "\r\n"
#else
#define HAMMER_BOGUS_REQUEST                            \
        "BOGUS / HTTP/1.0\r\n"                          \
        "\r\n"                                          \
        "\r\n"                                          \
        "\r\n"
#endif




#define HAMMERDUMP_SECTION_SEPSTR                               \
        "\r\n================== HAMMER REPLAY DUMP =====================\r\n"

#define HAMMERDUMPSTART_SECTION_SEPSTR                          \
        "\r\n================== [START] HAMMER REPLAY DUMP [START] =====================\r\n"





typedef struct
{
  struct timeval start;
  struct timeval end;
} timelapse_t;


void timer_init(timelapse_t * d)
{
  timerclear(&d->start);
  timerclear(&d->end);
}

void timer_start(timelapse_t * d)
{
  timer_init(d);
  gettimeofday(&d->start, NULL);
}

int timer_start_cond(timelapse_t * d)
{
  int retv = 0;
  if(!timerisset(&d->start))
  {
    gettimeofday(&d->start, NULL);
    retv = 1;
  }
  return retv;
}

void timer_end(timelapse_t * d)
{
  gettimeofday(&d->end, NULL);
}

int timer_end_cond(timelapse_t * d)
{
  int retv = 0;
  if(!timerisset(&d->end))
  {
    gettimeofday(&d->end, NULL);
    retv = 1;
  }
  return retv;
}



void timer_diff_add_run(page_statistics_t * page, summary_statistics_t * sum, timelapse_t * dt)
{
  if(timerisset(&dt->start) && timerisset(&dt->end))
  {
    struct timeval delta;
    timersub(&dt->start, &dt->end, &delta);
    if(!timerisset(&sum->start_time))
    {
      sum->start_time = dt->start;
    }
    sum->end_time = dt->end;

    sum->run_count++;
    timeradd2(&sum->total_run_time, &delta);

    sum->transmitted_request_count += !!page->transmitted_request_count;
    sum->failed_request_count += !!page->failed_request_count;
    sum->failed_socket_count += !!page->failed_socket_count;
    sum->failed_connect_count += !!page->failed_connect_count;
    sum->failed_tx_count += !!page->failed_tx_count;
    sum->failed_response_count += !!page->failed_response_count;
    sum->unexpected_response_count += !!page->unexpected_response_count;
    sum->bad_content_count += !!page->bad_content_count;
  }
}

void timer_diff_add_page(page_statistics_t * page, summary_statistics_t * sum, timelapse_t * dt)
{
  if(timerisset(&dt->start) && timerisset(&dt->end))
  {
    struct timeval delta;
    timersub(&dt->start, &dt->end, &delta);
    ASSERT(!timerisset(&page->start_time));
    page->start_time = dt->start;
    page->end_time = dt->end;

    sum->page_count++;
    timeradd2(&sum->total_page_time, &delta);
    timeradd2(&sum->total_wait_time, &page->total_wait_time);
    timeradd2(&sum->total_timeout_time, &page->total_timeout_time);
    timeradd2(&sum->total_connect_timeout_time, &page->total_connect_timeout_time);
    timeradd2(&sum->total_write_timeout_time, &page->total_write_timeout_time);
    timeradd2(&sum->total_read_timeout_time, &page->total_read_timeout_time);
  }
}

void timer_diff_add_connect(page_statistics_t * page, summary_statistics_t * sum, timelapse_t * dt)
{
  if(timerisset(&dt->start) && timerisset(&dt->end))
  {
    struct timeval delta;
    timersub(&dt->start, &dt->end, &delta);

    timeradd2(&page->total_connect_timeout_time, &delta);
  }
}

void timer_diff_add_read(page_statistics_t * page, summary_statistics_t * sum, timelapse_t * dt)
{
  if(timerisset(&dt->start) && timerisset(&dt->end))
  {
    struct timeval delta;
    timersub(&dt->start, &dt->end, &delta);
  }
}

void timer_diff_add_ease_off(page_statistics_t * page, summary_statistics_t * sum, timelapse_t * dt)
{
  if(timerisset(&dt->start) && timerisset(&dt->end))
  {
    struct timeval delta;
    timersub(&dt->start, &dt->end, &delta);
  }
}

void time_log_page(const summary_statistics_t * sum, const page_statistics_t * page)
{
}

void time_log_run(const summary_statistics_t * sum, const page_statistics_t * page)
{
}








static void yakdbg(const char *msg)
{
  if(cfg.progress)
  {
    switch (cfg.progress_mode)
    {
    case 0:
    case 1:
    case 2:
      break;

    case 3:
    case 4:
      print_progress_style(13, msg);
      break;

    case 5:
    case 6:
    default:
      /* nada */
      break;
    }
  }
}

typedef struct
{
  unsigned long id;
  char *option;
  char *label;
} hammerflag_level_type;


hammerflag_level_type cfg_hammerflag_levels[] = {
  {HAMMER_FLAG_PERSISTENT_CONNECTION, "persist",
    gettext_nop("support persistent connections")}
  ,
  {HAMMER_FLAG_BOGUS_REQ_FOLLOWS_ON_PERSISTENT_CONNECTION,
      "bogus_ends_persist",
    gettext_nop("bogus request terminates persistent connection")}
  ,
  {HAMMER_FLAG_HALF_CLOSE, "halfclose",
    gettext_nop("half-close client connection after request")}
  ,
  {HAMMER_FLAG_AUTO_EASE_OFF, "auto_ease",
    gettext_nop("automatic ease off tuning")}
  ,
  {HAMMER_FLAG_BOGUS_AFTER_SILENT_TIME, "bogus_after_silent",
    gettext_nop("bogus request is transmitted as soon as the connection is silent for a given period of time")}
  ,
  {HAMMER_FLAG_NO_TIME_NICETIES, "no_niceties", gettext_nop("don't play nice")}
  ,
  {HAMMER_FLAG_PERSISTENT_DOS, "dos_persist",
    gettext_nop("DoS the server IP stack through persistent connections")}
  ,
  {HAMMER_FLAG_NO_READ_AFTER_BOGUS, "no_read_after_bogus",
    gettext_nop("no more reading data after 'bogus' sentinel has been transmitted")}
  ,
  {HAMMER_FLAG_PUSH_BOGUS, "push_bogus",
    gettext_nop("write 'bogus' sentinel ASAP after initial response bytes arrive")}
  ,
  {0, NULL, NULL}
};

int hammerflag_level_parse(const char *str, unsigned long *dst)
{
  unsigned int dl = 0;
  const char *p = str;
  int l, i;
  bool_t found;

  ASSERT(dst != NULL);
  *dst = 0;
  while(*p)
  {
    p += strspn(p, ",");
    l = strcspn(p, ",");
    found = FALSE;
    for(i = 0; cfg_hammerflag_levels[i].id; i++)
    {
      if(!strncasecmp(p, cfg_hammerflag_levels[i].option, l) &&
        (l == strlen(cfg_hammerflag_levels[i].option)))
      {
        found = TRUE;
        dl |= cfg_hammerflag_levels[i].id;
        break;
      }
      else if(*p == '!'
        && (!strncasecmp(p + 1, cfg_hammerflag_levels[i].option, l - 1) &&
          (l - 1 == strlen(cfg_hammerflag_levels[i].option))))
      {
        found = TRUE;
        dl &= ~cfg_hammerflag_levels[i].id;
        break;
      }
    }
    if(!found)
    {
      char *ep;

      i = strtol(p, &ep, 0);

      if((ep - p) != l)
      {
        xprintf(0, gettext("Bad hammer_flag level selection: %s. Cannot decode '%s' remainder\n"), str, p);
        return -1;
      }
      else
      {
        dl |= i;
      }
    }
    p += l;
  }

  *dst = dl;
  return 0;
}

char *hammerflag_level_construct(unsigned long level)
{
  bool_t frst = TRUE;
  int i;
  char *strbuf = NULL;

  if(!level)
  {
    return tl_strdup("0");
  }

  for(i = 0; cfg_hammerflag_levels[i].id; i++)
  {
    if((cfg_hammerflag_levels[i].id & level) != cfg_hammerflag_levels[i].id)
      continue;

    if(!frst)
      strbuf = tl_str_concat(DBGvars(strbuf), ",", NULL);
    strbuf = tl_str_concat(DBGvars(strbuf), cfg_hammerflag_levels[i].option, NULL);
    frst = FALSE;
    level &= ~cfg_hammerflag_levels[i].id;
  }
  if(level)
  {
    char buf[20];
    sprintf(buf, "%lu", level);
    if(!frst)
      strbuf = tl_str_concat(DBGvars(strbuf), ",", NULL);
    strbuf = tl_str_concat(DBGvars(strbuf), buf, NULL);
  }
  return strbuf;
}




void dlfree_func_act_rec_queue(void *key_data)
{
  activity_record *rec = (activity_record *) key_data;
  if(rec)
  {
    free_deep_activityrecord(rec);
    _free(rec);
  }
}



activity_record *new_activity_record(void)
{
  activity_record *rec = _malloc(sizeof(*rec));
  memset(rec, 0, sizeof(*rec));
  rec->type = URLT_UNKNOWN;
  DEBUG_DEVEL("create a new activity record [%d/%s]\n", __LINE__, __FILE__);
  return rec;
}

void free_deep_activityrecord(activity_record * p)
{
  if(p)
  {
    _free(p->local_addr);
    _free(p->dest_addr);
    _free(p->host_name);
    _free(p->sent_to_server);
    _free(p->req_mime_header);
    _free(p->req_data);
    _free(p->url4display);

    p->local_addr_len = 0;
    p->dest_addr_len = 0;
    p->sent_size = 0;
    p->header_sent_size = 0;
    p->req_mime_header_len = 0;
    p->req_data_len = 0;
  }
}


static char *show_mem_as_hex(void *src, int len)
{
  static const char *hexcvt = "0123456789ABCDEF";
  int i;
  int dlen = len * 2 + 1 + 8;
  char *dst = _malloc(dlen);
  char *p = dst;
  if(src)
  {
    sprintf(p, "%7d:", len);
    p += strlen(p);
    for(i = 0; i < len; i++)
    {
      *p++ = hexcvt[((unsigned char *) src)[i] >> 4];
      *p++ = hexcvt[((unsigned char *) src)[i] & 0xF];
      ASSERT(p - dst < dlen);
    }
    *p = 0;
    ASSERT(p - dst < dlen);
  }
  else
  {
    strcpy(p, "(null)");
  }
  return dst;
}


activity_record *get_registered_activityrecord(activity_record * parent, url * urlp)
{
  activity_record *rec = new_activity_record();

  DEBUG_DEVEL("get new activity record registered [%d/%s]\n", __LINE__, __FILE__);

  /* add record to queue */
  LOCK_RECACT;
  cfg.rec_act_queue = dllist_append(cfg.rec_act_queue, rec);
  UNLOCK_RECACT;

  if(urlp)
  {
    char *p = url_to_urlstr(urlp, TRUE);
    rec->url4display = p;
  }

  if(parent)
  {
    if(parent->host_name)
    {
      rec->host_name = tl_strdup(parent->host_name);
    }
    rec->host_port = parent->host_port;
    if(parent->local_addr_len)
    {
      rec->local_addr = _malloc(parent->local_addr_len);
      memcpy(rec->local_addr, parent->local_addr, parent->local_addr_len);
      rec->local_addr_len = parent->local_addr_len;
    }
    if(parent->dest_addr_len)
    {
      rec->dest_addr = _malloc(parent->dest_addr_len);
      memcpy(rec->dest_addr, parent->dest_addr, parent->dest_addr_len);
      rec->dest_addr_len = parent->dest_addr_len;
    }
    rec->socket_family = parent->socket_family;
    rec->socket_proto = parent->socket_proto;
    rec->ref_count = parent->ref_count + 1;
  }
  return rec;
}

void fixup_record(activity_record * rec)
{
  int l;
  char *p = NULL;

  DEBUG_DEVEL("fixup activity record [%d/%s]\n", __LINE__, __FILE__);

  ASSERT(!rec->req_mime_header ? !rec->req_mime_header_len : TRUE);
  ASSERT(!rec->req_data ? !rec->req_data_len : TRUE);
  l = rec->req_mime_header_len + rec->req_data_len;
  if(l > 0)
  {
    p = _malloc(l);
    rec->sent_to_server = p;
    rec->sent_size = l;
    rec->header_sent_size = rec->req_mime_header_len;
    if(rec->req_mime_header)
    {
      memcpy(p, rec->req_mime_header, rec->req_mime_header_len);
      p += rec->req_mime_header_len;
    }
    if(rec->req_data)
    {
      memcpy(p, rec->req_data, rec->req_data_len);
      p += rec->req_data_len;
    }
    p = rec->sent_to_server;
  }
  else
  {
    rec->sent_to_server = NULL;
    rec->sent_size = 0;
  }

  switch (rec->type)
  {
  default:
  case URLT_UNKNOWN:
  case URLT_FTP:
  case URLT_FTPS:
  case URLT_FILE:
  case URLT_GOPHER:
  case URLT_FROMPARENT:
    rec->is_page_starter = TRUE;
    break;

  case URLT_HTTP:
  case URLT_HTTPS:
    if(cfg.page_suffix && cfg.page_suffix[0] && cfg.allow_page_suffix)
    {
      const char **pp = cfg.page_suffix;
      const char *p = rec->url4display;

      rec->is_page_starter = FALSE;
      while(*pp)
      {
        int xlen;
        const char *url_end;

        xlen = strlen(*pp);
        url_end = p + strcspn(p, "?#");
        if(xlen > 0 && url_end - xlen >= p)
        {
          if(!strncmp(*pp, url_end - xlen, xlen))
          {
            rec->is_page_starter = TRUE;
            break;
          }
        }
        pp++;
      }
    }
    else
    {
      static const char *def[] = {
        ".html", ".htm", ".asp", ".aspx", ".php", ".php3", ".php4", ".pl",
        ".jsp",
        ".shtml", ".phtml", NULL
      };
      const char **pp = def;
      const char *p = rec->url4display;

      rec->is_page_starter = FALSE;
      while(*pp)
      {
        int xlen;
        const char *url_end;

        xlen = strlen(*pp);
        url_end = p + strcspn(p, "?#");
        if(xlen > 0 && url_end - xlen >= p)
        {
          if(!strncmp(*pp, url_end - xlen, xlen))
          {
            rec->is_page_starter = TRUE;
            break;
          }
        }
        pp++;
      }
    }
    break;
  }

  if(cfg.log_hammer_action && bufio_is_open(cfg.rec_act_dump_fd))
  {
    char *asciibuf;
    char buf[2048];
    int hexlen = l * 5 + 81;
    char *s1;
    char *s2;
    int totlen;

    snprintf(buf, sizeof(buf), "\f\n\n"
      "!-- [FIXUP] Recording request: %8u --\n"
      "!   URL: %s\n"
      "!   actual host: %s\n"
      "!   actual port: %u\n"
      "!   local addr: %s\n"
      "!   destination addr: %s\n"
      "!   socket family: %d\n"
      "!   socket proto: %d\n"
      "!   ref count: %d\n"
      "!   expected response code: %d\n"
      "!   expect persistent: %d\n"
      "!   is a page starter: %d %s\n"
      "!   -- request data (mime: %d / extra: %d / len: %d / header: %d): --\n",
      0,
      rec->url4display,
      rec->host_name,
      rec->host_port,
      (s1 = show_mem_as_hex(rec->local_addr, rec->local_addr_len)),
      (s2 = show_mem_as_hex(rec->dest_addr, rec->dest_addr_len)),
      rec->socket_family,
      rec->socket_proto,
      rec->ref_count,
      rec->expected_response_code,
      rec->expect_persistent,
      rec->is_page_starter, (rec->is_page_starter ? " [STARTER]" : ""),
      (int) rec->req_mime_header_len, (int) rec->req_data_len,
      (int) rec->sent_size, (int) rec->header_sent_size);
    buf[sizeof(buf) - 1] = 0;
    asciibuf = _malloc(hexlen);
    tl_asciidump_content(asciibuf, hexlen, p, l);
    /* record 'fixup' */
    totlen = strlen(buf) + strlen(asciibuf);
    snprintf(buf, sizeof(buf), "\f\n\n"
      "!-- [FIXUP] Recording request: %8u --", totlen);
    buf[strlen(buf)] = '\n';
    /* end of fixup */
    bufio_puts_m(cfg.rec_act_dump_fd, buf, asciibuf, NULL);
    _free(asciibuf);
    _free(s1);
    _free(s2);
  }

  return;
}




void clean_actrec_list(void)
{
  dllist *dptr;

  LOCK_RECACT;
  for(dptr = cfg.rec_act_queue; dptr;)
  {
    bool_t empty;
    activity_record *rec = (activity_record *) dptr->data;

    fixup_record(rec);

    empty = (!rec->host_name && !rec->host_port
      && !rec->local_addr && !rec->local_addr_len
      && !rec->dest_addr && !rec->dest_addr_len
      && !rec->socket_family && !rec->socket_proto
      /* && !rec->ref_count */  && !rec->sent_size && !rec->header_sent_size);
/* gotta have something to send! */
    empty |= !rec->sent_size;

    if(empty)
    {
      if(dptr != cfg.rec_act_queue)
      {
        dptr = dllist_remove_entry_custom(dptr, dptr, dlfree_func_act_rec_queue);
      }
      else
      {
        cfg.rec_act_queue = dptr = dllist_remove_entry_custom(dptr, dptr, dlfree_func_act_rec_queue);
      }
      continue;
    }
    dptr = dptr->next;
  }
  /* always mark initial record as a 'starter' anyway */
  dptr = cfg.rec_act_queue;
  if(dptr)
  {
    activity_record *rec = (activity_record *) dptr->data;

    if(dptr == cfg.rec_act_queue)
    {
      rec->is_page_starter = TRUE;
    }
  }
  UNLOCK_RECACT;
}


void dump_actrec_list(void)
{
  if(cfg.log_hammer_action && bufio_is_open(cfg.rec_act_dump_fd))
  {
    char *asciibuf;
    char buf[2048];
    int hexlen;
    char *s1;
    char *s2;
    int totlen;
    dllist *dptr;
    bufio *fd;

    VERIFY(!bufio_lock(cfg.rec_act_dump_fd, FALSE, &fd));
    bufio_puts(fd, "\n\n\n================== ACTION RECORDING DUMP =====================\n\n\n");

    LOCK_RECACT;
    for(dptr = cfg.rec_act_queue; dptr; dptr = dptr->next)
    {
      activity_record *rec = (activity_record *) dptr->data;

      ASSERT(rec);
      snprintf(buf, sizeof(buf), "\f\n\n"
        "-- Recording request: %8u --\n"
        "   URL: %s\n"
        "   actual host: %s\n"
        "   actual port: %u\n"
        "   local addr: %s\n"
        "   destination addr: %s\n"
        "   socket family: %d\n"
        "   socket proto: %d\n"
        "   ref count: %d\n"
        "   expected response code: %d\n"
        "   expect persistent: %d\n"
        "   is a page starter: %d\n"
        "   -- request data (mime: %d / extra: %d / len: %d / header: %d): --\n",
        0,
        rec->url4display,
        rec->host_name,
        rec->host_port,
        (s1 = show_mem_as_hex(rec->local_addr, rec->local_addr_len)),
        (s2 = show_mem_as_hex(rec->dest_addr, rec->dest_addr_len)),
        rec->socket_family,
        rec->socket_proto,
        rec->ref_count,
        rec->expected_response_code,
        rec->expect_persistent,
        rec->is_page_starter,
        (int) rec->req_mime_header_len, (int) rec->req_data_len,
        (int) rec->sent_size, (int) rec->header_sent_size);
      buf[sizeof(buf) - 1] = 0;
      hexlen = TL_MAX(rec->sent_size, rec->req_mime_header_len + rec->req_data_len) * 5 + 81;
      asciibuf = _malloc(hexlen);
      if(rec->sent_to_server)
      {
        tl_asciidump_content(asciibuf, hexlen, rec->sent_to_server, rec->sent_size);
      }
      else
      {
        if(rec->req_mime_header)
        {
          tl_asciidump_content(asciibuf, hexlen, rec->req_mime_header, rec->req_mime_header_len);
        }
        else
        {
          asciibuf[0] = 0;
        }
        if(rec->req_data)
        {
          tl_asciidump_content(asciibuf + strlen(asciibuf), hexlen - strlen(asciibuf), rec->req_data, rec->req_data_len);
        }
      }
      /* record 'fixup' */
      totlen = strlen(buf) + strlen(asciibuf);
      snprintf(buf, sizeof(buf), "\f\n\n" "-- Recording request: %8u --\n", totlen);
      buf[strlen(buf)] = ' ';
      /* end of fixup */
      bufio_puts(fd, buf);
      bufio_puts(fd, asciibuf);
      _free(asciibuf);
      _free(s1);
      _free(s2);
    }
    UNLOCK_RECACT;
    VERIFY(!bufio_release(cfg.rec_act_dump_fd, &fd));
  }
}


#ifndef INVALID_SOCKET
#define INVALID_SOCKET  (-1)
#endif

static int replay_in_nb_thread(int threadnr)
{
  int conn_timeout = (int) cfg.conn_timeout;
  doc fake_doc = { 0 };
  int fake_mimebuf_size = 100000;
  int fake_mimebuf_fill = 0;
  char *fake_mimebuf = _malloc(fake_mimebuf_size);
  char slurp_buf[4098];
  bool_t persistent_conn = FALSE;
  dllist *persistent_conn_dptr = NULL;
  int sock = INVALID_SOCKET;
  bool_t logtime = bufio_is_open(cfg.time_logfile);
  bool_t logtime4sum = bufio_is_open(cfg.time_logfile4sum);
  bool_t shortlog = bufio_is_open(cfg.short_logfile);
  summary_statistics_t sum_stats;
  page_statistics_t page_stats;

  init_page_stats(&page_stats);
  init_summary_stats(&sum_stats);
  {
    int i;
    dllist *dptr;
    for(i = 0, dptr = cfg.rec_act_queue; dptr; dptr = dptr->next)
      i++;
    sum_stats.recording_size = i;
  }

#ifdef HAVE_MT
  fake_doc.threadnr = threadnr;
#endif
  fake_doc.cyclenr = 0;
#ifdef HAVE_MT
  pthread_setspecific(cfg.currdoc_key, (void *) (&fake_doc));
  pthread_setspecific(cfg.herrno_key, (void *) (&(fake_doc.__herrno)));
#endif

  if(cfg.log_hammer_action && bufio_is_open(cfg.dump_fd))
  {
    ASSERT(cfg.dump_fd);
    ASSERT(bufio_is_open(cfg.dump_fd));
    bufio_puts(cfg.dump_fd, HAMMERDUMPSTART_SECTION_SEPSTR);
  }

  for(; !cfg.rbreak && !cfg.stop;)
  {
    bool_t another_round;
    int recnr;
    int cyclenr;
    dllist *dptr;
    bool_t retry_item = FALSE;
    timelapse_t dt_run;
    timelapse_t dt_page;

    timer_init(&dt_run);
    timer_init(&dt_page);

    LOCK_TCNT;
    another_round = ((cyclenr = ++cfg.hammer_cycle) <= cfg.hammer_endcount);
    UNLOCK_TCNT;

    fake_doc.cyclenr = cyclenr;

    DEBUG_TRACE("run another round? %d\n", another_round);
    if(!another_round)
      break;

    if(cfg.progress)
    {
      switch (cfg.progress_mode)
      {
      case 0:
#ifdef HAVE_MT
        xprintf(0, gettext("======= RUN[%2d]: %5d ======\n"), fake_doc.threadnr + 1, cyclenr);
#else
        xprintf(0, gettext("======= RUN: %5d ======\n"), cyclenr);
#endif
        break;

      case 6:
        /* nada */
        break;

      case 2:
      case 3:
      case 4:
      case 5:
      default:
        print_progress_style(12, ">");
        break;

      case 1:
        print_progress_style(12, "=RUN=\n");
        break;
      }
    }

    /*
       NOTE: we only access rec_act_queue in READONLY mode, so we don't need mutexes
       around this code. There is NO concurrent process WRITING into rec_act_queue.
     */
    DEBUG_TRACE("execute the queue? %p\n", cfg.rec_act_queue);
    timer_start(&dt_run);
    timer_start(&dt_page);
    recnr = 1;
    ASSERT(cfg.rec_act_queue ? !cfg.rec_act_queue->prev : TRUE);
    for(dptr = cfg.rec_act_queue; dptr && !cfg.rbreak && !cfg.stop;)
    {
      int rv = 0;
      activity_record *rec = (activity_record *) dptr->data;

      DEBUG_TRACE("execute activity record [%d/%d/%d]: %p\n", threadnr, cyclenr, recnr, rec);

      fake_doc.doc_nr = recnr;
      fake_doc.mime = NULL;
      fake_doc.cyclenr = cyclenr;
      fake_doc.size = 0;
      fake_doc.errcode = ERR_NOERROR;

      page_stats.page_size++;
      if(rec->is_page_starter)
      {
        if(timerisset(&dt_page.start))
        {
          timer_end(&dt_page);
          timer_diff_add_page(&page_stats, &sum_stats, &dt_page);
        }
        time_log_page(&sum_stats, &page_stats);
        init_page_stats(&page_stats);
        timer_start(&dt_page);
      }

      if(cfg.progress)
      {
        switch (cfg.progress_mode)
        {
        case 0:
#ifdef HAVE_MT
          xprintf(0, gettext("URL[%2d]: %5d(%d) of %5d  %s\n"),
            fake_doc.threadnr + 1, fake_doc.doc_nr, cfg.fail_cnt, cfg.total_cnt, rec->url4display);
#else
          xprintf(0, gettext("URL: %5d(%d) of %5d  %s\n"), fake_doc.doc_nr, cfg.fail_cnt, cfg.total_cnt, rec->url4display);
#endif
          break;

        case 2:
        case 3:
        case 4:
        case 5:
          {
            char msg[10];
            if(persistent_conn)
            {
              strcpy(msg, "!");
            }
            else
            {
              strcpy(msg, "@");
            }
            print_progress_style(11, msg);
          }
          break;

        case 6:
          /* nada */
          break;

        default:
        case 1:
          {
            char msg[10];
#ifdef HAVE_MT
            if(cfg.hammer_threadcount <= 26)
            {
              if(fake_doc.threadnr >= 0 && fake_doc.threadnr < 26)
              {
                msg[0] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[fake_doc.threadnr];
              }
              else
              {
                msg[0] = '@';
              }
              if(persistent_conn)
              {
                msg[1] = '!';
                msg[2] = 0;
              }
              else
              {
                msg[1] = 0;
              }
            }
            else
            {
              snprintf(msg, sizeof(msg), "%4d%s", fake_doc.threadnr, (persistent_conn ? "!" : ""));
              msg[sizeof(msg) - 1] = 0;
            }
#else
            if(persistent_conn)
            {
              strcpy(msg, "!");
            }
            else
            {
              strcpy(msg, "@");
            }
#endif
            print_progress_style(11, msg);
          }
          break;
        }
      }

      if(!persistent_conn || sock < 0)
      {
        persistent_conn = !!(cfg.hammer_flags & HAMMER_FLAG_PERSISTENT_CONNECTION);
        persistent_conn_dptr = dptr;
      }

      if(logtime || logtime4sum)
      {
        gettimeofday(&fake_doc.hr_start_time, NULL);
        timerclear(&fake_doc.redirect_time);
        timerclear(&fake_doc.dns_time);
        timerclear(&fake_doc.connect_time);
        timerclear(&fake_doc.first_byte_time);
        timerclear(&fake_doc.end_time);
      }

      set_h_errno(0);

      if(sock < 0)
      {
        DEBUG_TRACE("get a socket: %d/%d/%d\n", rec->socket_family, rec->socket_proto, sock);
        if((sock = socket(rec->socket_family, SOCK_STREAM, rec->socket_proto)) == INVALID_SOCKET)
        {
          page_stats.failed_socket_count++;
          DEBUG_HAMMER("socket() -> %d/%d/%s\n", sock, errno, strerror(errno));
          xperror("socket");
          retry_item = TRUE;
        }

        if(sock >= 0)
        {
#ifdef HAVE_WINSOCK2_H
          u_long val = 1;

          DEBUG_TRACE("set it to NON-BLOCKING\n");
          if(SOCKET_ERROR == ioctlsocket(sock, FIONBIO, &val))
          {
            errno = WinSockErr2errno(WSAGetLastError());
            xperror("fcntl() - F_SETFL");
            closesocket(sock);
            sock = INVALID_SOCKET;
            retry_item = TRUE;
          }
#else
          DEBUG_TRACE("set it to NON-BLOCKING\n");
          if(fcntl(sock, F_SETFL, O_NONBLOCK))
          {
            page_stats.failed_socket_count++;
            xperror("fcntl() - F_SETFL");
            close(sock);
            sock = INVALID_SOCKET;
            retry_item = TRUE;
          }
#endif
        }

        if(sock >= 0)
        {
          DEBUG_TRACE("optional: bind to local port\n");
          if(rec->local_addr)
          {
            ASSERT(rec->local_addr_len);

            if(bind(sock, rec->local_addr, rec->local_addr_len) != 0)
            {
              page_stats.failed_socket_count++;
              xperror("local bind");
              close(sock);
              sock = INVALID_SOCKET;
              retry_item = TRUE;
            }
          }
        }

        if(logtime || logtime4sum)
        {
          gettimeofday(&fake_doc.dns_time, NULL);
        }

        if(sock >= 0)
        {
          DEBUG_TRACE("socket connect: %d:%p/%d\n", sock, rec->dest_addr, rec->dest_addr_len);
          rv = connect(sock, rec->dest_addr, rec->dest_addr_len);
          if(rv && (errno != EINPROGRESS) && (errno != EISCONN) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
          {
            page_stats.failed_connect_count++;
            DEBUG_HAMMER("socket connect -> %d:%d/%d\n", sock, rv, errno);
            xperror("socket connect");
            close(sock);
            fake_doc.errcode = ERR_HTTP_CONNECT;
            sock = INVALID_SOCKET;
            retry_item = TRUE;
          }
        }
        DEBUG_TRACE("socket connect -> %d:%d/%d\n", sock, rv, errno);

        if(sock >= 0)
        {
#if defined(I_FACE) && !defined(HAVE_MT)
          if(cfg.xi_face)
          {
            if(rv && (errno == EINPROGRESS))
            {
              rv = gui_wait_io(sock, FALSE);
              if(!rv)
              {
                DEBUG_NET("Async connect - connected\n");
              }

              if(cfg.rbreak || cfg.stop || rv)
              {
                close(sock);
                fake_doc.errcode = ERR_BREAK;
                sock = INVALID_SOCKET;
              }
            }
          }
          else
#endif /* !HAVE_MT */
          {
            if(rv && (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK))
            {
              timelapse_t dt;

              yakdbg("c");

              timer_start(&dt);
              while((rv = tl_selectw(sock, conn_timeout)) == -1 && errno == EINTR)
              {
                if(cfg.rbreak)
                {
                  DEBUG_DEVEL("cfg.stop = TRUE @ %s, line #%d\n", __FILE__, __LINE__);
                  errno = EINTR;
                  cfg.stop = TRUE;
                  fake_doc.errcode = ERR_BREAK;
                  break;
                }
              }
              if(rv == 0 /* && conn_timeout > 0 */ )
              {
                page_stats.failed_connect_count++;
                timer_end(&dt);
                timer_diff_add_connect(&page_stats, &sum_stats, &dt);
                yakdbg("[XC]");

                DEBUG_HAMMER("connect timeout: %d\n", conn_timeout);
                errno = ETIMEDOUT;
                fake_doc.errcode = ERR_HTTP_CONNECT;
                rv = -1;
              }
              if(cfg.rbreak || cfg.stop || (rv < 0))
              {
                if(rv < 0)
                {
                  page_stats.failed_connect_count++;
                }
                DEBUG_HAMMER("break/stop/connect failure: %d/%d/%d:%d\n", cfg.rbreak, cfg.stop, conn_timeout, rv);
                close(sock);
                sock = INVALID_SOCKET;
              }
            }
          }
        }

        DEBUG_TRACE("socket after connect + delay: %d\n", sock);
        if(sock >= 0)
        {
#if defined __QNX__ || defined __BEOS__
          if(connect(sock, rec->dest_addr, rec->dest_addr_len) && (errno != EISCONN))
          {
            close(sock);
            sock = INVALID_SOCKET;
          }
#else
#ifdef HAVE_WINSOCK2_H
          int socklen = sizeof(rv);
          if(getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *) &rv, &socklen) || rv)
          {
            page_stats.failed_socket_count++;
            closesocket(sock);
            errno = WinSockErr2errno(rv);
            sock = INVALID_SOCKET;
          }
#else
          socklen_t socklen = sizeof(rv);
          if(getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *) &rv, &socklen) || rv)
          {
            page_stats.failed_socket_count++;
            close(sock);
            errno = rv;
            sock = INVALID_SOCKET;
          }
#endif
#endif
        }
        if(sock >= 0)
        {
          DEBUG_NET("[%d] Succesfully connected to %s,%d\n", threadnr, rec->host_name, rec->host_port);
        }
      }
      else
      {
        if(logtime || logtime4sum)
        {
          gettimeofday(&fake_doc.dns_time, NULL);
        }
      }


      /*
         NOTE: we do not (yet) support SSL connections!
       */
      DEBUG_TRACE("socket after geterr: %d\n", sock);

      if(logtime || logtime4sum)
      {
        gettimeofday(&fake_doc.connect_time, NULL);
      }

      if(sock >= 0)
      {
        /* now that we are connected, push those request bytes out the door pronto! */
        if(rec->sent_to_server)
        {
          long receive_count;
          int wlen;

          yakdbg("W");
          if(cfg.log_hammer_action && bufio_is_open(cfg.dump_fd))
          {
            DEBUG_TRACE("sock_write: dump content\n");
            ASSERT(cfg.dump_fd);
            ASSERT(bufio_is_open(cfg.dump_fd));
            bufio_asciidump(cfg.dump_fd, rec->sent_to_server, rec->sent_size, HAMMERDUMP_SECTION_SEPSTR);
            DEBUG_TRACE("sock_write: content dumped\n");

            if(bufio_is_open(cfg.dump_fd) && cfg.dump_fd->bufio_errno == EFBIG)
            {
              xprintf(1,
                gettext
                ("Cannot store any more data in the dump file! Will stop dump log. err:%d = %s\n"),
                cfg.dump_fd->bufio_errno, strerror(cfg.dump_fd->bufio_errno));
              /* HACK! */
              close(bufio_getfd(cfg.dump_fd));
              cfg.dump_fd->fd = -1;
            }
          }


          page_stats.transmitted_request_count++;
          if((wlen = tl_sock_write(sock, rec->sent_to_server, rec->sent_size, NULL)) != rec->sent_size)
          {
            int errcode = errno;
            DEBUG_TRACE("socket write %d -> %d\n", rec->sent_size, wlen);
            if(errcode == EPIPE)
            {
              page_stats.failed_tx_count++;
              yakdbg("[XW]");
              if(cfg.hammer_flags & HAMMER_FLAG_AUTO_EASE_OFF)
              {
                cfg.hammer_ease_time += 10;
                DEBUG_HAMMER("auto ease-off UP: %lf\n", cfg.hammer_ease_time);
              }
            }
            xperror("tl_sock_write");
            close(sock);
            fake_doc.errcode = ERR_HTTP_SNDREQ;
            page_stats.failed_request_count++;
            sock = INVALID_SOCKET;
          }
          yakdbg("w");
          DEBUG_TRACE("request written: %d; waiting for reponse\n", wlen);

          receive_count = 0;
          if(sock >= 0)
          {
            bool_t bogus_written = FALSE;
            int loop_cnt;
            int sel_rv;

#if 0
            gui_set_status(gettext("Waiting for response ..."));
#endif
            DEBUG_TRACE("sock_read coming up! %d:%p/%d\n", sock, slurp_buf, sizeof(slurp_buf));
            if(cfg.hammer_flags & HAMMER_FLAG_HALF_CLOSE)
            {
              /* half-close so we don't have to detect/handle persistent connections: */
              DEBUG_TRACE("half-close!\n");
              rv = shutdown(sock, SHUT_WR);
              if(rv)
              {
                page_stats.failed_socket_count++;
                page_stats.failed_request_count++;
                DEBUG_DEVEL("ERR_HTTP_RCVRESP [%d/%s]\n", __LINE__, __FILE__);
                fake_doc.errcode = ERR_HTTP_RCVRESP;
                DEBUG_HAMMER("half-close: %d -> %d\n", sock, rv);
              }
              persistent_conn = FALSE;
            }
            /*
               WARNING: if you don't use 'persistent' connections AND you have not specified
               the halfclose option, you'll wait for a VERY long time in the next read() call
               as it'll have to timeout to terminate the connection.
             */
            yakdbg("R");
            for(sel_rv = -1, loop_cnt = 0;
              !cfg.stop && !cfg.rbreak
              && ((wlen = ((bogus_written
                      && (cfg.
                        hammer_flags & HAMMER_FLAG_NO_READ_AFTER_BOGUS))
                    ? (sel_rv = 0) : tl_sock_nbread(sock, slurp_buf, sizeof(slurp_buf), NULL))) >= 0); loop_cnt++)
            {
              yakdbg("r");
              DEBUG_TRACE("sock_read %d:%p/%d -> %d/%ld\n", sock, slurp_buf, sizeof(slurp_buf), wlen, receive_count);
              if(sel_rv == 1 && wlen == 0)
              {
                /*
                   non-blocking read() + previous select() signal we
                   should close: CLOSE_WAIT state detected
                 */
                yakdbg("[CR]");
                DEBUG_TRACE("sock_read: CLOSE_WAIT detected; closing connection!\n");
                persistent_conn = FALSE;
                break;
              }
              if(wlen > 0)
              {
                if(logtime || logtime4sum)
                {
                  if(!timerisset(&fake_doc.first_byte_time))
                  {
                    gettimeofday(&fake_doc.first_byte_time, NULL);
                  }
                }
                if(!fake_doc.mime)
                {
                  /* assume we received a MIME header! */
                  fake_doc.mime = fake_mimebuf; /* no time spent on malloc here! */
                  fake_mimebuf_fill = 0;
                  fake_mimebuf[fake_mimebuf_fill] = 0;
                }
                if(fake_mimebuf_fill < fake_mimebuf_size)
                {
                  int copylen = TL_MIN(wlen, fake_mimebuf_size - 1 - fake_mimebuf_fill);
                  memcpy(fake_mimebuf + fake_mimebuf_fill, slurp_buf, copylen);
                  fake_mimebuf_fill += copylen;
                  fake_mimebuf[fake_mimebuf_fill] = 0;
                  ASSERT(fake_mimebuf_fill < fake_mimebuf_size);
                }
                receive_count += wlen;
                if(cfg.log_hammer_action && bufio_is_open(cfg.dump_fd))
                {
                  DEBUG_TRACE("sock_read: dump content\n");
                  ASSERT(cfg.dump_fd);
                  ASSERT(bufio_is_open(cfg.dump_fd));
                  bufio_asciidump(cfg.dump_fd, slurp_buf, wlen, HAMMERDUMP_SECTION_SEPSTR);
                  DEBUG_TRACE("sock_read: content dumped\n");
                  if(bufio_is_open(cfg.dump_fd) && cfg.dump_fd->bufio_errno == EFBIG)
                  {
                    xprintf(1,
                      gettext
                      ("Cannot store any more data in the dump file! Will stop dump log. err:%d = %s\n"),
                      cfg.dump_fd->bufio_errno, strerror(cfg.dump_fd->bufio_errno));
                    /* HACK! */
                    close(bufio_getfd(cfg.dump_fd));
                    cfg.dump_fd->fd = -1;
                  }
                }
              }

              {
                /*
                   check if there are any more bytes received; grab those,
                   otherwise hit the server with another request over the persistent
                   connection?
                 */
                int timeout = (int) cfg.hammer_read_timeout;
                bool_t send_bogus;
                timelapse_t dt;

                timer_init(&dt);

                send_bogus = (persistent_conn && (cfg.hammer_flags & HAMMER_FLAG_BOGUS_REQ_FOLLOWS_ON_PERSISTENT_CONNECTION));
                if(!send_bogus) /* OR */
                  send_bogus = ((loop_cnt > 0) && (cfg.hammer_flags & HAMMER_FLAG_PUSH_BOGUS));
                if(!send_bogus) /* OR */
                  send_bogus = ((loop_cnt > 0) && (wlen == 0) && (cfg.hammer_flags & HAMMER_FLAG_PERSISTENT_DOS));
                if(!bogus_written && send_bogus)
                {
                  int blen;

                  yakdbg("b1");
#if 0
                  ASSERT(persistent_conn);
#endif
                  yakdbg("WZ");
                  if(cfg.log_hammer_action && bufio_is_open(cfg.dump_fd))
                  {
                    DEBUG_TRACE("sock_write: bogus dump content\n");
                    ASSERT(cfg.dump_fd);
                    ASSERT(bufio_is_open(cfg.dump_fd));
                    bufio_asciidump(cfg.dump_fd, HAMMER_BOGUS_REQUEST, strlen(HAMMER_BOGUS_REQUEST), HAMMERDUMP_SECTION_SEPSTR);
                    DEBUG_TRACE("sock_write: bogus content dumped\n");
                    if(bufio_is_open(cfg.dump_fd) && cfg.dump_fd->bufio_errno == EFBIG)
                    {
                      xprintf(1,
                        gettext
                        ("Cannot store any more data in the dump file! Will stop dump log. err:%d = %s\n"),
                        cfg.dump_fd->bufio_errno, strerror(cfg.dump_fd->bufio_errno));
                      /* HACK! */
                      close(bufio_getfd(cfg.dump_fd));
                      cfg.dump_fd->fd = -1;
                    }
                  }

                  if((blen = tl_sock_write(sock, HAMMER_BOGUS_REQUEST,
                        strlen(HAMMER_BOGUS_REQUEST), NULL)) != strlen(HAMMER_BOGUS_REQUEST))
                  {
                    /* page_stats.failed_tx_count++; */
                    yakdbg("[XB1]");
                    DEBUG_TRACE("socket bogus write %d -> %d\n", strlen(HAMMER_BOGUS_REQUEST), blen);
                    xperror("tl_sock_write(bogus)");
                    close(sock);
                    fake_doc.errcode = ERR_HTTP_SNDREQ;
                    persistent_conn = FALSE;
                    sock = INVALID_SOCKET;
                    break;
                  }
                  yakdbg("wz");
                  bogus_written = TRUE;
                  DEBUG_NET("bogus request written (%d) on persistent connection; waiting for reponse?\n", blen);
                }

                if(!(cfg.hammer_flags & HAMMER_FLAG_NO_TIME_NICETIES))
                {
                  if(timeout < 10)
                    timeout = 10;
                }
                DEBUG_TRACE("hammer read timeout: %d\n", timeout);

                yakdbg("S");
                timer_start(&dt);
                if((sel_rv = tl_selectr(sock, timeout)) == -1 && errno == EINTR)
                {
                  yakdbg(",");
                  if(cfg.rbreak)
                  {
                    DEBUG_DEVEL("cfg.stop = TRUE @ %s, line #%d\n", __FILE__, __LINE__);
                    errno = EINTR;
                    cfg.stop = TRUE;
                    fake_doc.errcode = ERR_BREAK;
                    persistent_conn = FALSE;
                    break;
                  }
                }
                if(sel_rv < 0)
                {
                  page_stats.failed_request_count++;
                  page_stats.failed_response_count++;
                }
                if(sel_rv == 0)
                {
                  timer_end(&dt);
                  timer_diff_add_read(&page_stats, &sum_stats, &dt);
                }
                yakdbg("s");
                DEBUG_TRACE("read select -> %d / %ld / %d / %d / 0x%08X\n", rv, receive_count, wlen, loop_cnt, cfg.hammer_flags);
                if((sel_rv == 0) && ((receive_count > 0) || (loop_cnt > 0))
                  && !(cfg.hammer_flags & (HAMMER_FLAG_BOGUS_AFTER_SILENT_TIME | HAMMER_FLAG_PERSISTENT_DOS)))
                {
                  yakdbg("[WR]");
                  DEBUG_NET("read wait timeout: %d; try the next persistent request\n", timeout);
                  break;
                }
                if(sel_rv < 0)
                {
                  yakdbg("[XR]");
                  DEBUG_NET("read wait (%d) error: %d; try the next request on a new connection\n", timeout, errno);
                  xperror("write wait");
                  persistent_conn = FALSE;
                  break;
                }
                send_bogus = ((loop_cnt > 0) && (sel_rv == 0) && (cfg.hammer_flags & HAMMER_FLAG_BOGUS_AFTER_SILENT_TIME));
                if(!send_bogus)
                  send_bogus = ((sel_rv == 1) && (cfg.hammer_flags & HAMMER_FLAG_PUSH_BOGUS));
                if(!bogus_written && send_bogus)
                {
                  int blen;

                  yakdbg("b2");
#if 0
                  ASSERT(persistent_conn);
#endif
                  yakdbg("WY");
                  if((blen = tl_sock_write(sock, HAMMER_BOGUS_REQUEST,
                        strlen(HAMMER_BOGUS_REQUEST), NULL)) != strlen(HAMMER_BOGUS_REQUEST))
                  {
                    /* page_stats.failed_tx_count++; */
                    yakdbg("[XB2]");
                    DEBUG_TRACE("socket bogus write %d -> %d\n", strlen(HAMMER_BOGUS_REQUEST), blen);
                    xperror("tl_sock_write(bogus)");
                    close(sock);
                    fake_doc.errcode = ERR_HTTP_SNDREQ;
                    persistent_conn = FALSE;
                    sock = INVALID_SOCKET;
                    break;
                  }
                  yakdbg("wy");
                  bogus_written = TRUE;
                  DEBUG_NET("bogus request written (%d) on persistent connection; waiting for reponse?\n", blen);
                }
              }
            }
            yakdbg("t");
            DEBUG_TRACE("end of read loop: all data read: %d/%ld/%d\n", wlen, receive_count, wlen);
            if(wlen < 0)
            {
              persistent_conn = FALSE;
            }
          }
          if(receive_count <= 0)
          {
            page_stats.failed_request_count++;
            page_stats.failed_response_count++;
            DEBUG_DEVEL("ERR_HTTP_RCVRESP [%d/%s]\n", __LINE__, __FILE__);
            fake_doc.errcode = ERR_HTTP_RCVRESP;
            persistent_conn = FALSE;
          }
          else
          {
            http_response resp;
            fake_doc.size = receive_count;
            if(!http_get_response_info2(&resp, fake_mimebuf))
            {
              DEBUG_HAMMER
                ("page '%s': web server MIME response: version %d.%d, HTTP response code %d: '%s'\n",
                rec->url4display, resp.ver_maj, resp.ver_min, resp.ret_code, resp.text);
              if(resp.ret_code != rec->expected_response_code)
              {
                page_stats.unexpected_response_count++;
                fake_doc.errcode = ERR_UNEXPECTED_RESPONSE;
                xprintf(1,
                  "page '%s' returned UNEXPECTED response code: %d (act) vs. %d (exp)\n",
                  rec->url4display, resp.ret_code, rec->expected_response_code);
              }
            }
            else
            {
              DEBUG_HAMMER("web server MIME response could not be decoded\n");
            }
			_free(resp.text);
			ASSERT(!resp.text);

            /*
               check content for 'bad_content' strings. Assume BINARY content!

               FIXME: due to the speed at which this hammer 'hack' was developed, here's one
               to fix: the regular run checks only CONTENT against bad_content, while we include
               possible MIME headers here too.

               Should we assume correct MIME headers anyway and skip until after the first
               occurrence of a double CRLF pair? I guess so.
             */
            if(fake_doc.mime)
            {
              /* find double CRLF pair. */
              int ml;
              char *s = fake_doc.mime;

              for(ml = strcspn(s, "\r\n"); fake_doc.mime[ml]; ml = strcspn(s, "\r\n"))
              {
                s += ml;
                if(!strncmp(s, "\r\n\r\n", 4))
                {
                  s[2] = 0;     /* mark this as end of mime block; include first CRLF pair of them both */
                  s += 4;
                  break;
                }
                s += strspn(s, "\r\n"); /* skip CR's and LF's */
              }
              fake_doc.contents = s;
              ml = fake_mimebuf_fill - (fake_doc.contents - fake_doc.mime);
              ASSERT(fake_doc.contents);
              ASSERT(ml >= 0);
              if(cfg.bad_content && (fake_doc.errcode == ERR_NOERROR) && ml > 0)
              {
                const char **cpp;

                ASSERT(fake_doc.mime == fake_mimebuf);

                for(cpp = cfg.bad_content; *cpp && (fake_doc.errcode == ERR_NOERROR); cpp++)
                {
                  const char *cp = *cpp;
                  int n = ml;   /* cannot use 'fakedoc.size'! */
                  char *dp = fake_doc.contents;
                  int cn = strlen(cp);
                  char *ep = dp + n - cn;
                  char *idx;
                  if(ep >= dp)
                  {
                    for(idx = (char *) memchr(dp, *cp, ep - dp);
                      idx; idx = (char *) ((ep - ++idx > 0) ? memchr(idx, *cp, ep - idx) : NULL))
                    {
                      if(!memcmp(idx, cp, cn))
                      {
                        /* found a match! */
                        char pom[1024];
                        page_stats.bad_content_count++;
                        snprintf(pom, sizeof(pom),
                          gettext
                          ("check document '%s' for bad content: match '%s' at index %d in mime+content"),
                          rec->url4display, cp, (int) (idx - dp));
                        pom[sizeof(pom) - 1] = 0;
                        /* KILL_PERSISTANT_CONNECTION; */
                        fake_doc.errcode = ERR_BAD_CONTENT;
                        report_error(&fake_doc, pom);
                        break;
                      }
                    }
                  }
                }
              }
            }
          }
          DEBUG_TRACE("reponse length: %ld, error: %d\n", receive_count, fake_doc.errcode);
        }
        else
        {
          xprintf(1, gettext("There's nothing to send to the host\n"));
          fake_doc.errcode = ERR_NOTHING_TO_SEND;
        }
      }
      DEBUG_TRACE
        ("end of recorded action [%d/%d/%d]; jot down the timing figures: %d/%d/%d:%d:%d\n",
        threadnr, cyclenr, recnr, cfg.rbreak, cfg.stop, conn_timeout, sock, fake_doc.errcode);

      if(logtime || logtime4sum)
      {
        gettimeofday(&fake_doc.end_time, NULL);
      }
      if(logtime)
      {
        /*
           time log MODE=1: first URL in a persistent connection.
           MODE=2: subsequent URL in a persistent connection.
         */
        time_log(&fake_doc, (sock >= 0 ? sock : -2 - (persistent_conn_dptr == dptr)), rec->url4display);
      }
      if(shortlog)
      {
        short_log(&fake_doc, NULL, rec->url4display);
      }

      /*
         NOTE: we do not support persistent connections (HTTP Keep-Alive)
       */
      if(!persistent_conn && (sock >= 0))
      {
        struct linger li = { 0 };

        yakdbg("L");
        DEBUG_TRACE("closing socket: end action\n");
        li.l_onoff = TRUE;
        rv = setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *) &li, sizeof(li));
        if(rv < 0)
        {
          xperror("set zero linger");
        }
        rv = close(sock);
        if(rv < 0)
        {
          xperror("close");
        }
        yakdbg("l");
        sock = INVALID_SOCKET;
      }

#ifdef HAVE_MT
      doc_finish_processing(&fake_doc);
#endif

      if(!(cfg.hammer_flags & HAMMER_FLAG_NO_TIME_NICETIES))
      {
        if(cfg.hammer_ease_time > 0.0)
        {
          timelapse_t dt;
          timer_start(&dt);
          DEBUG_DEVEL("tick %lf\n", cfg.hammer_ease_time);
          tl_msleep((int) cfg.hammer_ease_time, TRUE);
          DEBUG_DEVEL("TACK\n");
          timer_end(&dt);
          timer_diff_add_ease_off(&page_stats, &sum_stats, &dt);
        }
      }

      if(sock < 0)
      {
        persistent_conn = FALSE;
      }
      if(!retry_item)
      {
        recnr++;
        DEBUG_TRACE("Get next action record [%d/%d/%d]: %p/%p\n", threadnr, cyclenr, recnr, dptr, persistent_conn_dptr);
        if(!persistent_conn)
        {
          /*
             ignore all records replayed in persistent mode: they might not have been
             seen at all by the server.
           */
          ASSERT(sock < 0);
          if(persistent_conn_dptr)
          {
            DEBUG_TRACE("persistent connection closed: resume from 2nd entry\n");
            dptr = persistent_conn_dptr->next;
            continue;
          }
        }
        dptr = dptr->next;
      }
      /* else: retry this item: do NOT change 'dptr'! */
    }
    DEBUG_HAMMER("One round of replay hammering done [%d:%d]\n", threadnr, cyclenr);

    /* last page timelapse has NOT been terminated / processed yet! */
    timer_end(&dt_run);
    if(timerisset(&dt_page.start))
    {
      timer_end(&dt_page);
      timer_diff_add_page(&page_stats, &sum_stats, &dt_page);
    }
    time_log_page(&sum_stats, &page_stats);
    timer_diff_add_run(&page_stats, &sum_stats, &dt_run);
    time_log_run(&sum_stats, &page_stats);
    init_page_stats(&page_stats);
  }

  return (!cfg.rbreak && !cfg.stop);
}




#ifdef HAVE_MT
static void replay_the_recording_thread(int thrnr)
{
#ifdef I_FACE
  _config_struct_priv_t privcfg;
#endif

  {
#if defined(HAVE_SIGEMPTYSET) && defined(HAVE_SIGADDSET) && defined(HAVE_PTHREAD_SIGMASK)
    sigset_t smask;

    sigemptyset(&smask);
    sigaddset(&smask, SIGINT);
#ifdef SIGQUIT
    sigaddset(&smask, SIGQUIT);
#endif
#ifdef SIGXFSZ
    sigaddset(&smask, SIGXFSZ);
#endif
    pthread_sigmask(SIG_UNBLOCK, &smask, NULL);
#endif

    tl_signal(SIGINT, pavuk_sigintthr);
#ifdef SIGQUIT
    tl_signal(SIGQUIT, pavuk_sigquitthr);
#endif
#ifdef SIGXFSZ
    tl_signal(SIGXFSZ, pavuk_sigfilesizethr);
#endif
  }

  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  pthread_setspecific(cfg.currdoc_key, (void *) NULL);
  pthread_setspecific(cfg.thrnr_key, (void *) thrnr);
  DEBUG_MTTHR("starting hammer thread(%ld) %d\n", pthread_t2long(pthread_self()), thrnr);

#ifdef I_FACE
  privcfg_make_copy(&privcfg);
  pthread_setspecific(cfg.privcfg_key, (void *) (&privcfg));
  pthread_cleanup_push((void *) privcfg_free, (void *) (&privcfg));
#endif

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

/*    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); */
  DEBUG_MTTHR("hammer thread %d awaking\n", thrnr);

  replay_in_nb_thread(thrnr);

  gui_clear_status();
  DEBUG_MTTHR("Hammer thread %d sleeping\n", thrnr);
  gui_set_status(gettext("Hammer sleeping ..."));
/*    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); */
  DEBUG_TRACE("mt_semaphore_up(cfg.nrunning_sem)\n");
  mt_semaphore_up(&cfg.nrunning_sem);

#ifdef I_FACE
  pthread_cleanup_pop(TRUE);
#endif
  DEBUG_MTTHR("Hammer thread %d exiting\n", thrnr);
  gui_set_status(gettext("Hammer terminating ..."));
  pthread_exit(NULL);
}
#endif



void replay_the_recording(void)
{
  if(cfg.rec_act_queue && !cfg.rbreak)
  {
/*
   Let the action run in the number of threads we requested.

   If, however, we do not run in threaded mode (hammer_threadcount == 0)
   we'll employ the use of non-blocking socket I/O to make sure we keep
   the pipeline filled to the brim.
 */
#ifdef HAVE_MT
    DEBUG_HAMMER("hammer_threadcount = %d\n", cfg.hammer_threadcount);
#endif
    DEBUG_HAMMER("hammer_endcount = %d\n", cfg.hammer_endcount);
    cfg.stop = FALSE;

    xprintf(1, "== Hammer Mode %d - hammering starts! ==\n", cfg.hammer_mode);
    time_log_header(1, "=== HAMMERING ===");
    {
      doc fake_doc = { 0 };
      short_log(&fake_doc, NULL, HAMMERDUMPSTART_SECTION_SEPSTR);
    }

#ifdef HAVE_MT
    if(cfg.hammer_threadcount <= 0)
#endif
    {
      replay_in_nb_thread(-1);
    }
#ifdef HAVE_MT
    else
    {
      pthread_attr_t thrdattr;
      int i;
      int num = cfg.hammer_threadcount;

#if 0
      mt_semaphore_destroy(&cfg.nrunning_sem);
      _free(cfg.allthreads);
      cfg.allthreadsnr = 0;
#endif

      {
#if defined(HAVE_SIGEMPTYSET) && defined(HAVE_SIGADDSET) && defined(HAVE_PTHREAD_SIGMASK)
        sigset_t smask;

        sigemptyset(&smask);
        sigaddset(&smask, SIGINT);
#ifdef SIGQUIT
        sigaddset(&smask, SIGQUIT);
#endif
#ifdef SIGXFSZ
        sigaddset(&smask, SIGXFSZ);
#endif
        pthread_sigmask(SIG_UNBLOCK, &smask, NULL);
#endif

        tl_signal(SIGINT, pavuk_sigintthr);
#ifdef SIGQUIT
        tl_signal(SIGQUIT, pavuk_sigquitthr);
#endif
#ifdef SIGXFSZ
        tl_signal(SIGXFSZ, pavuk_sigfilesizethr);
#endif
      }

      pthread_attr_init(&thrdattr);
      pthread_attr_setscope(&thrdattr, PTHREAD_SCOPE_SYSTEM);
      pthread_attr_setstacksize(&thrdattr, MT_STACK_SIZE);

      DEBUG_TRACE("mt_semaphore_init(nrunning/urlstack)\n");
      mt_semaphore_initv(&cfg.nrunning_sem, 1);

      if(num <= 0)
        num = 1;

      cfg.allthreadsnr = 0;
      cfg.allthreads = _malloc(num * sizeof(cfg.allthreads[0]));

      LOCK_TCNT;
      cfg.hammer_cycle = 0;
      UNLOCK_TCNT;
      DEBUG_HAMMER("create %d hammer threads\n", num);
      for(i = 0; i < num; i++)
      {
        if(!pthread_create(&(cfg.allthreads[cfg.allthreadsnr]),
            &thrdattr, (void *(*)(void *)) replay_the_recording_thread, (void *) cfg.allthreadsnr))
        {
          cfg.allthreadsnr++;
          gui_mt_thread_start(cfg.allthreadsnr);
          DEBUG_TRACE("mt_semaphore_dec(cfg.nrunning_sem)\n");
          mt_semaphore_decrement(&cfg.nrunning_sem);
        }
        else
        {
          xprintf(0, "Create hammer thread %d", i);
        }
        if(cfg.rbreak || cfg.stop)
          break;
      }

      DEBUG_TRACE("mt_semaphore_timed_wait(cfg.nrunning_sem)\n");
      while(!cfg.stop && !cfg.rbreak && mt_semaphore_timed_wait(&cfg.nrunning_sem, 500) < 0)
      {
#if 0
        DEBUG_TRACE("wait for cfg.nrunning_sem: v = %ld\n", cfg.nrunning_sem.v);
#endif
      }

      DEBUG_DEVEL("cfg.stop = TRUE @ %s, line #%d\n", __FILE__, __LINE__);
      /*
         signal all processing threads to stop as each one of them is only waiting
         for another URL to arrive in the URL stack/queue right now - which will never
         happen as all URLs have been processed.
       */
      cfg.stop = TRUE;

      tl_msleep(300, TRUE);

      for(i = 0; i < cfg.allthreadsnr; i++)
      {
/*
    pthread_cancel(cfg.allthreads[i]);
    pthread_kill(cfg.allthreads[i], SIGQUIT);
*/
        pthread_join(cfg.allthreads[i], NULL);
      }

      DEBUG_TRACE("mt_semaphore_destroy(cfg.nrunning_sem)\n");
      mt_semaphore_destroy(&cfg.nrunning_sem);
      _free(cfg.allthreads);
      cfg.allthreadsnr = 0;
      gui_mt_thread_end(0);
    }
#endif
  }
}

#endif
