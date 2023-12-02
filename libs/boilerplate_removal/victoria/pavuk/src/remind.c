/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "remind.h"

#include "bufio.h"
#include "url.h"
#include "mime.h"
#include "times.h"
#include "http.h"
#include "dllist.h"
#include "tools.h"
#include "gcinfo.h"
#include "gui_api.h"

static dllist *remind_urls = NULL;

static int remind_check_if_newer(doc * docp, time_t modtime, time_t * nmdtm)
{
  char *p;
  int retv = 0;
  http_response *rsp;
  char *ustr;

  ustr = url_to_urlstr(docp->doc_url, FALSE);

  xprintf(1, gettext("Checking: %s\n"), ustr);

  if(!(docp->doc_url->type == URLT_HTTP
#ifdef USE_SSL
      || docp->doc_url->type == URLT_HTTPS
#endif
    ))
  {
    xprintf(1, "This URL type is not supported in reminder mode\n");
    return -1;
  }

  /*** to init properly proxy ... ***/
  doc_download_init(docp, FALSE);

  docp->request_type = HTTP_REQ_HEAD;
  docp->datasock = http_get_data_socket(docp);

  if(!docp->datasock)
    return -1;

  if(!docp->mime)
  {
    xprintf(1, "Bad response on HEAD request\n");
    return -1;
  }

  rsp = http_get_response_info(docp->mime);

  if(cfg.use_http11 /* && !docu->http_proxy_10 */  &&
    rsp->ver_maj == 1 && rsp->ver_min == 1)
  {
    docp->is_http11 = TRUE;
    docp->is_persistent = TRUE;
  }
  else
  {
    docp->is_http11 = FALSE;
    docp->is_persistent = FALSE;
  }

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  if(docp->actrec_ref)
  {
    activity_record *rec = docp->actrec_ref;

    DEBUG_DEVEL("recording response code: %d [%d/%s]\n", rsp->ret_code, __LINE__, __FILE__);
    rec->expected_response_code = rsp->ret_code;
  }
#endif

  p = get_mime_param_val_str("Connection:", docp->mime);
  if(p)
  {
    if(!strcasecmp(p, "close"))
      docp->is_persistent = FALSE;
    else if(!strcasecmp(p, "keep-alive"))
      docp->is_persistent = TRUE;
    _free(p);
  }

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  if(docp->actrec_ref)
  {
    activity_record *rec = docp->actrec_ref;

    DEBUG_DEVEL("set persistent flag: %d [%d/%s]\n", docp->is_persistent, __LINE__, __FILE__);
    rec->expect_persistent = docp->is_persistent;
  }
#endif

  if(!docp->is_persistent)
  {
    bufio_close(docp->datasock);
    docp->datasock = NULL;
  }

  if(rsp->ret_code == 304)
  {
    retv = 0;
  }
  else if((p = get_mime_param_val_str("Last-Modified:", docp->mime)))
  {
    *nmdtm = scntime(p, FALSE);
#if defined(HAVE_DEBUG_FEATURES)
	  if(cfg.debug && (DEBUG_TRACE_BITMASK & cfg.debug_level))
	  {
    LOCK_TIME;
    DEBUG_TRACE("remind: Last-Modified date decoded: '%s' (original timestamp text: '%s')\n", ctime(nmdtm), p);
    UNLOCK_TIME;
	  }
#endif
	  if (*nmdtm == (time_t)-1)
	  {
		  xprintf(1, "ERROR: [remind] cannot decode Last-Modified date '%s'\n", p);
		  *nmdtm = scntime(p, TRUE);
		  /* retv = -1; */
	  }
    else if(modtime && (difftime(*nmdtm, modtime) > 0))
    {
      retv = 1;
    }
    _free(p);
  }
  else
  {
    retv = -1;
  }

  _free(rsp->text);
  _free(rsp);
  return retv;
}

static void remind_check_entry(doc * docp, remind_entry * e)
{
  time_t t = 0L;
  int rv;

  rv = remind_check_if_newer(docp, e->mdtm, &t);

  if(!(cfg.stop || cfg.rbreak))
  {
    if(t)
      e->mdtm = t;

    if(rv == 1)
      e->status |= REMIND_MODIFIED;
    else if(rv == -1)
      e->status |= REMIND_ERROR;
  }
}

void remind_load_db(void)
{
  bufio *fd;
  char buf[TL_MAX(PATH_MAX, BUFIO_ADVISED_READLN_BUFSIZE)];

  snprintf(buf, sizeof(buf), "%s%s%s", cfg.path_to_home, 
	  (tl_is_dirname(cfg.path_to_home) ? "" : "/"), PAVUK_REMIND_DB_FILENAME);
  buf[sizeof(buf) - 1] = 0;

  while(remind_urls)
  {
    free_deep_url(((remind_entry *) remind_urls->data)->urlp);
    _free(remind_urls->data);
    remind_urls = dllist_remove_entry(remind_urls, remind_urls);
  }

  fd = bufio_copen(buf, O_BINARY | O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if(!fd)
  {
    xperror(mk_native(buf));
    return;
  }

  bufio_flock(fd, TRUE);

  while(bufio_readln(fd, buf, sizeof(buf)) > 0)
  {
    time_t t;
    url *purl;
    char *eptr;
    remind_entry *entry;

    strip_nl(buf);
    t = strtol(buf, &eptr, 10);
    if(!t && errno == ERANGE)
    {
      xprintf(1, gettext("Bad reminder db entry - %s\n"), buf);
      continue;
    }

    while(*eptr && tl_ascii_isspace(*eptr))
      eptr++;
    purl = url_parse(eptr);

    entry = _malloc(sizeof(*entry));
    entry->urlp = purl;
    entry->mdtm = t;
    entry->status = 0;
    remind_urls = dllist_append(remind_urls, (dllist_t) entry);
  }

  bufio_funlock(fd);
  bufio_close(fd);
  fd = NULL;
}

void remind_save_db(void)
{
  bufio *fd;
  char buf[PATH_MAX];
  dllist *ptr;

  snprintf(buf, sizeof(buf), "%s%s%s", cfg.path_to_home, 
	  (tl_is_dirname(cfg.path_to_home) ? "" : "/"), PAVUK_REMIND_DB_FILENAME);
  buf[sizeof(buf) - 1] = 0;

  fd = bufio_copen(buf, O_BINARY | O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if(!fd)
  {
    xperror(mk_native(buf));
    return;
  }

  bufio_flock(fd, TRUE);

  bufio_truncate(fd, 0);
  for(ptr = remind_urls; ptr; ptr = ptr->next)
  {
    char *p;

    p = url_to_urlstr(((remind_entry *) ptr->data)->urlp, FALSE);
    bufio_printf(fd, "%ld %s\n", ((remind_entry *) ptr->data)->mdtm, p);
    _free(p);
  }

  bufio_funlock(fd);
  bufio_close(fd);
  fd = NULL;
}

static remind_entry *remind_find_entry(url * urlp)
{
  dllist *ptr = remind_urls;
  remind_entry *rv = NULL;
  char *ustr;

  ustr = url_to_urlstr(urlp, FALSE);

  while(ptr && !rv)
  {
    remind_entry *e = (remind_entry *) ptr->data;
    char *p;

    p = url_to_urlstr(e->urlp, FALSE);

    if(!strcmp(ustr, p))
      rv = e;
    _free(p);
    ptr = ptr->next;
  }
  _free(ustr);
  return rv;
}

void remind_start_add(void)
{
  dllist *ptr = cfg.request;

  while(ptr)
  {
    url_info *ui = (url_info *) ptr->data;
    url *urlp = url_parse(ui->urlstr);

    if(!remind_find_entry(urlp))
    {
      remind_entry *e = _malloc(sizeof(*e));

      e->urlp = urlp;
      e->status = 0;
      e->mdtm = 0L;

      remind_urls = dllist_append(remind_urls, (dllist_t) e);
    }
    else
    {
      free_deep_url(urlp);
      _free(urlp);
    }

    ptr = ptr->next;
  }
}

#ifdef HAVE_MT
static void _remind_do(int thnr)
#else
void remind_do(void)
#endif
{
  dllist *ptr;
  remind_entry *e;
  doc docu;
  global_connection_info con_info;

  init_global_connection_data(&con_info);

  for(ptr = remind_urls; ptr; ptr = ptr->next)
  {
    e = (remind_entry *) ptr->data;

    if(e->status & REMIND_PROCESSED)
      continue;

    doc_init(&docu, e->urlp);
#ifdef HAVE_MT
    docu.threadnr = thnr;
    pthread_setspecific(cfg.currdoc_key, (void *) (&docu));
    pthread_setspecific(cfg.herrno_key, (void *) (&(docu.__herrno)));
#endif
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
    docu.cyclenr = 0;
#endif
    restore_global_connection_data(&con_info, &docu);

    remind_check_entry(&docu, e);

    save_global_connection_data(&con_info, &docu);

#ifdef HAVE_MT
    doc_finish_processing(&docu);
#endif

    if(cfg.stop || cfg.rbreak)
    {
      break;
    }
    else
    {
      e->status |= REMIND_PROCESSED;
    }
  }

  kill_global_connection_data(&con_info);
}

#ifdef HAVE_MT
static void _remind_do_thrd(int thrnr)
{
#ifdef I_FACE
  _config_struct_priv_t privcfg;

#if defined (__OSF__) || defined (__osf__)
#define __builtin_try
#define __builtin_finally
#endif

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

    signal(SIGINT, pavuk_sigintthr);
#ifdef SIGQUIT
    signal(SIGQUIT, pavuk_sigquitthr);
#endif
#ifdef SIGXFSZ
    signal(SIGXFSZ, pavuk_sigfilesizethr);
#endif
  }

  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  pthread_setspecific(cfg.currdoc_key, (void *) NULL);
  pthread_setspecific(cfg.thrnr_key, (void *) thrnr);
  DEBUG_MTTHR("starting thread(%ld) %d\n", pthread_t2long(pthread_self()), thrnr);

#ifdef I_FACE
  privcfg_make_copy(&privcfg);
  pthread_setspecific(cfg.privcfg_key, (void *) (&privcfg));
  pthread_cleanup_push((void *) privcfg_free, (void *) (&privcfg));
#endif

  for(; !cfg.rbreak && !cfg.stop;)
  {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    DEBUG_MTTHR("thread %d awaking\n", thrnr);
    _remind_do(thrnr);
    gui_clear_status();
    DEBUG_MTTHR("thread %d sleeping\n", thrnr);
    gui_set_status(gettext("Sleeping ..."));
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    DEBUG_TRACE("mt_semaphore_up(cfg.nrunning_sem)\n");
    mt_semaphore_up(&cfg.nrunning_sem);
    while(!cfg.stop && !cfg.rbreak && mt_semaphore_timed_wait(&cfg.urlstack_sem, 400) < 0)
    {
      DEBUG_TRACE("wait for cfg.urlstack_sem: v = %ld\n", cfg.urlstack_sem.v);
    }
    mt_semaphore_decrement(&cfg.nrunning_sem);
  }
  mt_semaphore_up(&cfg.nrunning_sem);

#ifdef I_FACE
  pthread_cleanup_pop(TRUE);
#endif
  DEBUG_MTTHR("thread %d exiting\n", thrnr);
  gui_set_status(gettext("Exiting ..."));
  pthread_exit(NULL);
}

void remind_do(void)
{
  pthread_attr_t thrdattr;
  int i;
  int num = cfg.nthr;

  cfg.allthreadsnr = 0;
  cfg.allthreads = _malloc(num * sizeof(cfg.allthreads[0]));

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

    signal(SIGINT, pavuk_sigintthr);
#ifdef SIGQUIT
    signal(SIGQUIT, pavuk_sigquitthr);
#endif
#ifdef SIGXFSZ
    signal(SIGXFSZ, pavuk_sigfilesizethr);
#endif
  }

  pthread_attr_init(&thrdattr);
  pthread_attr_setscope(&thrdattr, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setstacksize(&thrdattr, MT_STACK_SIZE);
  DEBUG_TRACE("mt_semaphore_init(cfg.nrunning_sem)\n");
  mt_semaphore_initv(&cfg.nrunning_sem, 1);
  mt_semaphore_initv(&cfg.urlstack_sem, 0);
  /* signal each URL available in the queue */
  {
    dllist *dptr;

    LOCK_CFG_URLSTACK;
    for(dptr = cfg.urlstack; dptr; dptr = dptr->next)
    {
      mt_semaphore_up(&cfg.urlstack_sem);
    }
    UNLOCK_CFG_URLSTACK;
  }

  if(num <= 0)
    num = 1;

  for(i = 0; i < num; i++)
  {
    if(!pthread_create(&(cfg.allthreads[cfg.allthreadsnr]), &thrdattr, (void *(*)(void *)) _remind_do_thrd, (void *) cfg.allthreadsnr))
    {
      cfg.allthreadsnr++;
      gui_mt_thread_start(cfg.allthreadsnr);
      DEBUG_TRACE("mt_semaphore_dec(cfg.nrunning_sem)\n");
      mt_semaphore_decrement(&cfg.nrunning_sem);
#if 0
      if(cfg.rbreak || cfg.stop)
        break;
#endif
    }
    else
    {
      xprintf(1, "Create downloading thread %d\n", i);
    }

    if(cfg.rbreak || cfg.stop)
      break;
  }

#if 0
  DEBUG_TRACE("mt_semaphore_down(cfg.nrunning_sem)\n");
  mt_semaphore_down(&cfg.nrunning_sem);
#endif

  DEBUG_TRACE("mt_semaphore_timed_down(cfg.nrunning_sem)\n");
  while(!cfg.stop && !cfg.rbreak && mt_semaphore_timed_wait(&cfg.nrunning_sem, 500) < 0)
  {
    DEBUG_TRACE("wait for cfg.nrunning_sem: v = %ld\n", cfg.nrunning_sem.v);
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

  _free(cfg.allthreads);
  cfg.allthreadsnr = 0;
  gui_mt_thread_end(0);
}
#endif

void remind_send_result(void)
{
  FILE *fp;
  char buf[PATH_MAX];
  char *p, *dp;
  int cnt;
  dllist *ptr;

  if(cfg.remind_cmd)
  {
    for(dp = buf, p = cfg.remind_cmd; *p; p++)
    {
      if(*p == '%')
      {
        p++;
        switch (*(p))
        {
        case '%':
          *dp = *p;
          dp++;
          break;
        case 'e':
          strcpy(dp, priv_cfg.from);
          while(*dp)
            dp++;
          break;
        case 'd':
          {
            time_t t = time(NULL);
            char pom[100];

            LOCK_TIME;
            strftime(pom, sizeof(pom), "%a %H:%M %d.%m.%Y", localtime(&t));
            UNLOCK_TIME;
            strcpy(dp, pom);
            while(*dp)
              dp++;
          }
          break;
        default:
          *dp = *p;
          dp++;
          p--;
        }
      }
      else
      {
        *dp = *p;
        dp++;
      }
    }
    *dp = '\0';
  }
  else
  {
    snprintf(buf, sizeof(buf), "mailx %s -s \"pavuk reminder result\"\n", priv_cfg.from);
    buf[sizeof(buf) - 1] = 0;
  }

#ifdef HAVE_POPEN
  fp = popen(buf, "w");
#else
  fp = NULL;
  errno = ENOSUP;
#endif
  if(!fp)
  {
    xperror1("popen", buf);
    return;
  }

  fprintf(fp, gettext("This is the result of running pavuk reminder mode\n\n"));

  cnt = 0;
  for(ptr = remind_urls; ptr; ptr = ptr->next)
  {
    remind_entry *e = (remind_entry *) ptr->data;

    if(e->status & REMIND_MODIFIED)
    {
      if(!cnt)
      {
        fprintf(fp, gettext("Changed URLs\n"));
        fprintf(fp, "-------------------------\n");
        cnt++;
      }
      p = url_to_urlstr(e->urlp, FALSE);
      LOCK_TIME;
      fprintf(fp, "%s %s", p, ctime(&e->mdtm));
      UNLOCK_TIME;
      _free(p);
    }
  }

  fprintf(fp, "\n\n");

  cnt = 0;
  for(ptr = remind_urls; ptr; ptr = ptr->next)
  {
    remind_entry *e = (remind_entry *) ptr->data;

    if(e->status & REMIND_ERROR)
    {
      if(!cnt)
      {
        fprintf(fp, gettext("URLs with some errors\n"));
        fprintf(fp, "-------------------------\n");
        cnt++;
      }
      p = url_to_urlstr(e->urlp, FALSE);
      fprintf(fp, "%s\n", p);
      _free(p);
    }
  }

#ifdef HAVE_PCLOSE
  pclose(fp);
#endif
}
