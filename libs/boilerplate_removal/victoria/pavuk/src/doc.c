/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "url.h"
#include "doc.h"
#include "tools.h"
#include "mime.h"
#include "http.h"
#include "ftp.h"
#include "gopher.h"
#include "decode.h"
#include "abstract.h"
#include "mode.h"
#include "times.h"
#include "dinfo.h"
#include "errcode.h"
#include "log.h"
#include "gui_api.h"
#include "html.h"
#include "re.h"

#ifdef I_FACE
static void doc_set_info(doc *);
#endif

static void show_progress(doc *, ssize_t, int);
static double compute_speed_rate(time_t, ssize_t);

int doc_download_init(doc *docu, int load)
{
#if 0
  if (docu->datasock)
  {
    bufio_close(docu->datasock);
    docu->datasock = NULL;
  }
#endif

  docu->remove_lock = TRUE;
  docu->lock_fn = NULL;
  docu->is_parsable = cfg.enable_js && (docu->doc_url->status & URL_ISSCRIPT);
  docu->contents = NULL;
  docu->mime = NULL;
  docu->request_header = NULL;
  docu->request_data = NULL;
  docu->request_datalen = 0;
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  if (cfg.hammer_mode == HAMMER_RECORD_AND_REPLAY)
  {
    DEBUG_HAMMER("prepare recording another (new) action\n");
    docu->actrec_ref = get_registered_activityrecord(docu->actrec_ref, docu->doc_url);
  }
  else
  {
    docu->actrec_ref = NULL;
  }
#endif
  docu->type_str = NULL;
  docu->save_online = FALSE;
  docu->size = 0;
  docu->current_size = 0;
  docu->totsz = -1;
  docu->origsize = 0;
  docu->rest_pos = 0;
  docu->stime = time(NULL);
  docu->s_sock = NULL;
  docu->is_http11 = FALSE;
  docu->is_chunked = FALSE;
  docu->is_persistent = FALSE;
  docu->read_chunksize = FALSE;
  docu->read_trailer = FALSE;
  docu->doreget = FALSE;
  docu->origtime = docu->dtime;
  docu->adj_sz = 0;
  docu->load = load;
  docu->http_proxy_10 = FALSE;
  docu->ftp_data_con_finished = FALSE;
  docu->ftp_dir_type = FALSE;

  docu->num_auth = cfg.auth_reuse_nonce ? 1 : 0;
  docu->num_proxy_auth = cfg.auth_reuse_proxy_nonce ? 1 : 0;

  docu->is_http_transfer =
    docu->doc_url->type == URLT_HTTP
    || docu->doc_url->type == URLT_HTTPS
    || (docu->doc_url->type == URLT_FTP
        && priv_cfg.ftp_proxy && cfg.ftp_via_http && !cfg.ftp_dirtyp)
    || (docu->doc_url->type == URLT_GOPHER
        && priv_cfg.gopher_proxy && cfg.gopher_via_http);

  /*** just default value, later will be assigned properly ***/
  docu->request_type = HTTP_REQ_UNKNOWN;

  if (docu->is_http_transfer && !docu->http_proxy)
  {
    char *proxy = NULL;
    unsigned short port = 0;

    switch (docu->doc_url->type)
    {
    case URLT_HTTP:
      {
        http_proxy *pr = NULL;

        LOCK_PROXY;
        pr = http_proxy_get();
        if (pr)
        {
          http_proxy_check(pr, docu);
          proxy = tl_strdup(pr->addr);
          port = pr->port;
          docu->http_proxy_10 = (pr->is_10 != 0);
        }
        UNLOCK_PROXY;
      }
      break;

#ifdef USE_SSL
    case URLT_HTTPS:
      if (priv_cfg.ssl_proxy)
      {
        proxy = tl_strdup(priv_cfg.ssl_proxy);
        port = cfg.ssl_proxy_port;
      }
      break;

#endif
    case URLT_FTP:
      if (priv_cfg.ftp_proxy)
      {
        proxy = tl_strdup(priv_cfg.ftp_proxy);
        port = cfg.ftp_proxy_port;
      }
      break;

    case URLT_GOPHER:
      if (priv_cfg.gopher_proxy)
      {
        proxy = tl_strdup(priv_cfg.gopher_proxy);
        port = cfg.gopher_proxy_port;
      }
      break;

    default:
      proxy = NULL;
      port = 0;
      break;
    }
    docu->http_proxy = proxy;
    docu->http_proxy_port = port;
  }

#if 0
  /* FIXED: this seems like a very dirty hack to me [i_a]; separate s_sock and dump_fd! */
  if (cfg.dump_fd->fd >= 0)
  {
    docu->remove_lock = FALSE;
    if (cfg.dump_after)
    {
      docu->load = TRUE;
      docu->save_online = FALSE;
      docu->s_sock = NULL;
    }
    else
    {
      docu->save_online = TRUE;
      ASSERT(cfg.dump_fd->fd > 0);
      docu->s_sock = bufio_dupfd(cfg.dump_fd->fd);
      if (!docu->s_sock)
      {
        xperror("bufio_dupfd()");
        docu->errcode = ERR_STORE_DOC;
        return -1;
      }
    }
  }
#endif

  gettimeofday(&docu->hr_start_time, NULL);
  timerclear(&docu->redirect_time);
  timerclear(&docu->dns_time);
  timerclear(&docu->connect_time);
  timerclear(&docu->first_byte_time);
  timerclear(&docu->end_time);

  return 0;
}

static int doc_check_quotas(doc *docu, ssize_t len, ssize_t totallen)
{
  int retcode = 0;

#define KILL_PERSISTANT_CONNECTION                                              \
  if (docu->doc_url->type == URLT_FTP || docu->doc_url->type == URLT_FTPS) \
    docu->ftp_fatal_err = TRUE;\
  if (docu->is_http11) \
    docu->is_persistent = FALSE

  if (cfg.minrate > 0.0 && (docu->doc_url->type != URLT_FILE
                            && !(docu->doc_url->status & URL_REDIRECT)))
  {
    time_t _tm = doc_etime(docu, FALSE);
    double _rt = compute_speed_rate(_tm, totallen);
    if (_rt < (cfg.minrate * 1024.0))
    {
      KILL_PERSISTANT_CONNECTION;
      docu->errcode = ERR_LOW_TRANSFER_RATE;
      retcode = -1;
    }
  }

  if (cfg.max_time > 0.0)
  {
    if ((cfg.start_time + (int)(60.0 * cfg.max_time)) < time(NULL))
    {
      KILL_PERSISTANT_CONNECTION;
      docu->errcode = ERR_QUOTA_TIME;
      retcode = -1;
    }
  }

  if (docu->doc_url->type != URLT_FILE
      && !(docu->doc_url->status & URL_REDIRECT))
  {
    cfg.trans_size += len;
  }

  if (cfg.file_quota && ((cfg.file_quota * 1024) <= totallen)
      && (docu->doc_url->type != URLT_FILE)
      && !(docu->doc_url->status & URL_REDIRECT))
  {
    KILL_PERSISTANT_CONNECTION;
    docu->errcode = ERR_QUOTA_FILE;
    retcode = 1;
  }

  if (cfg.trans_quota && ((cfg.trans_quota * 1024) <= cfg.trans_size))
  {
    KILL_PERSISTANT_CONNECTION;
    docu->errcode = ERR_QUOTA_TRANS;
    retcode = -1;
  }

#if defined HAVE_FSTATFS || defined HAVE_FSTATVFS
  if (cfg.fs_quota
      && (docu->doc_url->type != URLT_FILE)
      && !(docu->doc_url->status & URL_REDIRECT)
      && docu->s_sock
      /* && cfg.dump_fd->fd < 0 [i_a]: dupe condition */)
  {
#ifdef HAVE_FSTATVFS
    struct statvfs fss;
    if (fstatvfs(bufio_getfd(docu->s_sock), &fss))
    {
      xperror("fstatvfs");
    }
#else
    struct statfs fss;
    if (fstatfs(bufio_getfd(docu->s_sock), &fss))
    {
      xperror("fstatfs");
    }
#endif
    else
    {
      long freespace = (fss.f_bsize * fss.f_bavail) / 1024;

      if (freespace < cfg.fs_quota)
      {
        KILL_PERSISTANT_CONNECTION;
        docu->errcode = ERR_QUOTA_FS;
        retcode = -1;
      }
    }
  }
#endif
  return retcode;
}

static int doc_transfer_data(doc *docu)
{
  char *buf;
  int bufsize;
  ssize_t len;
  ssize_t min_len;
  ssize_t totallen = 0;
  int retcode = 0;

  if (docu->report_size)
    gui_set_status(gettext("Transfering data"));

  show_progress(docu, docu->adj_sz, FALSE);

  bufsize = (cfg.bufsize > 0 ? cfg.bufsize : 1) * 1024;
  buf = _malloc(bufsize);

#ifdef SO_RCVBUF
#ifndef __QNX__
  if (bufio_is_sock(docu->datasock))
  {
    if (setsockopt(bufio_getfd(docu->datasock),
                   SOL_SOCKET, SO_RCVBUF, (char *)&bufsize, sizeof(bufsize)))
    {
      xperror(gettext("setsockopt: SO_RCVBUF failed"));
    }
  }
#endif
#endif
  if (docu->save_online)
  {
    DEBUG_USER("Storing to file: %s\n", mk_native(url_to_filename(docu->doc_url, TRUE)));
  }

#if 0
  /* #if 0: moved to earlier moments; now also dumped when erroneous data is received */
  if (docu->mime && cfg.dump_resp && cfg.dump_fd->fd >= 0 && !cfg.dump_after)
  {
    ASSERT(docu->s_sock != NULL);
    bufio_write(docu->s_sock, docu->mime, strlen(docu->mime));
  }
#endif

#ifdef DEBUG
  if (cfg.dump_resp && bufio_is_open(cfg.dump_fd) && !cfg.dump_after)
  {
    bufio *fd;
    ASSERT(cfg.dump_fd);
    ASSERT(bufio_is_open(cfg.dump_fd));
    VERIFY(!bufio_lock(cfg.dump_fd, FALSE, &fd));
    VERIFY(!bufio_release(cfg.dump_fd, &fd));
  }
#endif

  /*
   * Make sure zero-size documents also produce a non-NULL docu->contents content buffer.
   *
   * To that end, we also accept zero(0) bytes read inside the loop, while making sure
   * _that_ situation does _not_ continue as it's the end anyway by bumping up min_len
   * once we have fetched some data for this item...
   */
  min_len = 0;
  while ((len = abs_read_data(docu, docu->datasock, buf, bufsize)) >= min_len)
  {
    min_len = 1;

    if (len > 0)
    {
      if (cfg.dump_resp && bufio_is_open(cfg.dump_fd) && !cfg.dump_after)
      {
        ASSERT(cfg.dump_fd);
        ASSERT(bufio_is_open(cfg.dump_fd));

        /* [i_a] 2006 expect load=TRUE for duped dumped block! maybe even 'is_sock=YES but that's
         * a side-effect */
#if 0
        bufio_printf(cfg.dump_fd,
                     "\ndocu->load: %d, docu->is_parsable: %d, bufio_is_sock(s) = %d\n",
                     (int)docu->load, (int)docu->is_parsable,
                     (int)bufio_is_sock(docu->datasock));
#endif

        /*
         * [i_a] only dump data if it was just recently fetched. Conditional here prevents this data from
         * being appended to actual (probably non-parsable) data dumped just previously.
         *
         * Should really only dump if load=0
         */
        ASSERT(!docu->load ? TRUE : !docu->is_parsable);
        ASSERT(!docu->load ? TRUE : !bufio_is_sock(docu->datasock));
        if (!docu->load || docu->is_parsable || bufio_is_sock(docu->datasock))
        {
          bufio_asciidump(cfg.dump_fd, buf, len, NULL);
        }
      }

      if (docu->save_online)
      {
        if (!cfg.no_disc_io && docu->s_sock != NULL)
        {
          if (write(bufio_getfd(docu->s_sock), buf, len) != len)
          {
            docu->errcode = ERR_STORE_DOC;
            xperror(gettext("storing document"));
            retcode = -1;
            if (docu->doc_url->type == URLT_FTP
                || docu->doc_url->type == URLT_FTPS)
              docu->ftp_fatal_err = TRUE;
            if (docu->is_http11)
              docu->is_persistent = FALSE;
            break;
          }
        }
      }

      totallen += len;
      docu->current_size += len;

      if (cfg.maxrate > 0.0
          && (docu->doc_url->type != URLT_FILE
              && !(docu->doc_url->status & URL_REDIRECT)))
      {
        time_t _tm = doc_etime(docu, FALSE);
        double _rt = compute_speed_rate(_tm, totallen);
        if (_rt > (cfg.maxrate * 1024.0))
        {
          tl_msleep((int)((1000.0 * ((double)totallen) / (cfg.maxrate * 1024.0)) - _tm), FALSE);
        }
      }
    }

    docu->size = totallen;
    show_progress(docu, docu->adj_sz, FALSE);

    if (docu->load || docu->is_parsable
        || ((docu->doc_url->type == URLT_FTP
             || docu->doc_url->type == URLT_FTPS)
            && docu->doc_url->p.ftp.dir)
        || (docu->doc_url->type == URLT_GOPHER
            && (docu->doc_url->p.gopher.selector[0] == '1'
                || docu->doc_url->p.gopher.selector[0] == 'h')))
    {
      docu->contents = _realloc(docu->contents, totallen + 1);
      if (len > 0)
      {
        memmove(docu->contents + totallen - len, buf, len);
      }
      docu->contents[totallen] = 0;
    }

    if (len > 0)
    {
      retcode = doc_check_quotas(docu, len, totallen);
      if (retcode)
      {
        if (retcode == 1)
          retcode = 0;
        break;
      }
    }

    if (docu->totsz > 0 && docu->totsz <= docu->current_size)
      break;
  }

  show_progress(docu, docu->adj_sz, TRUE);

#if 0                           /* seems to be the second part of the dump_fd hack */
  if (cfg.dump_fd->fd >= 0 && !cfg.dump_after)
  {
    ASSERT(docu->s_sock != NULL);
    bufio_close(docu->s_sock);
    docu->s_sock = NULL;
    docu->save_online = FALSE;
  }
#endif

  if (cfg.progress && docu->report_size
#ifdef I_FACE
      && !cfg.xi_face
#endif
  )
  {
    switch (cfg.progress_mode)
    {
    case 0:
      xprintf(0, "\n");
      break;

    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    default:
      break;
    }
  }

  if (len < 0 || ((docu->totsz > 0) && (docu->totsz != (docu->size + docu->rest_pos))))
  {
    xvperror(DBGvars(NULL), gettext("Document transfer data [%d / %d / %d]\n"), len, docu->totsz,
             (docu->size + docu->rest_pos));
    if ((docu->doc_url->type == URLT_HTTP || docu->doc_url->type == URLT_HTTPS)
        && (!(docu->doc_url->status & URL_REDIRECT)))
    {
      docu->errcode = ERR_HTTP_TRUNC;
    }
    else if ((docu->doc_url->type == URLT_FTP
              || docu->doc_url->type == URLT_FTPS)
             && (!(docu->doc_url->status & URL_REDIRECT)))
    {
      docu->errcode = ERR_FTP_TRUNC;
    }
    else if (!docu->errcode)
    {
      docu->errcode = ERR_READ;
    }

    docu->remove_lock = FALSE;
    retcode = -1;
  }

  if (docu->report_size)
  {
    gui_set_status(gettext("Data transfer done"));
  }

  if ((docu->doc_url->type == URLT_FTP
       || docu->doc_url->type == URLT_FTPS) && docu->errcode == ERR_FTP_TRUNC)
  {
    docu->remove_lock = FALSE;
    retcode = -1;
  }

  /*
   * if transfer was not from beginning, reread
   * document content to memory from local file
   * to be sure we will process whole document
   */
  if (!retcode && docu->rest_pos && (docu->load || docu->is_parsable)
      && docu->s_sock
      && bufio_is_open(docu->s_sock) /* && (cfg.dump_fd->fd < 0) */)
  {
    _free(docu->contents);
    docu->contents = NULL;
    totallen = 0;
    ASSERT(docu->s_sock != NULL);
    bufio_seek(docu->s_sock, 0, SEEK_SET);
    bufio_reset(docu->s_sock);  /* MANDATORY to prevent write buffer from flowing into read buffer! */

    while ((len = bufio_read(docu->s_sock, buf, bufsize)) > 0)
    {
      totallen += len;
      docu->contents = _realloc(docu->contents, totallen + 1);
      memmove(docu->contents + totallen - len, buf, len);
    }
  }

  if (docu->contents)
    *(docu->contents + totallen) = '\0';

  _free(buf);
  docu->size = totallen;

  /*
   * check content for 'bad_content' strings. Assume BINARY content!
   */
  if (cfg.bad_content && (docu->errcode == ERR_NOERROR) && docu->contents)
  {
    const char **cpp;

    for (cpp = cfg.bad_content; *cpp && !retcode; cpp++)
    {
      const char *cp = *cpp;
      int n = docu->size;
      const char *dp = docu->contents;
      int cn = strlen(cp);
      const char *ep = dp + n - cn;
      const char *idx;
      if (ep >= dp)
      {
        for (idx = (const char *)memchr(dp, *cp, ep - dp);
             idx && !retcode;
             idx = (const char *)((ep - ++idx > 0) ? memchr(idx, *cp, ep - idx) : NULL))
        {
          if (!memcmp(idx, cp, cn))
          {
            /* found a match! */
            char pom[1024];
            snprintf(pom, sizeof(pom),
                     gettext("fetch document: match '%s' at index %d in content"),
                     cp, (int)(idx - dp));
            pom[sizeof(pom) - 1] = 0;
            /* KILL_PERSISTANT_CONNECTION; */
            docu->errcode = ERR_BAD_CONTENT;
            report_error(docu, pom);
            retcode = -1;
            break;
          }
        }
      }
    }
  }
  return retcode;
}

static int doc_check_doc_file(doc *docu, int *rv)
{
  char *fn;
  struct stat estat;

  fn = url_to_filename(docu->doc_url, TRUE);

  if (cfg.mode != MODE_SYNC && cfg.mode != MODE_MIRROR)
  {
    if (docu->doc_url->type != URLT_FILE && (tl_access(fn, R_OK) != -1))
    {
      if (!tl_stat(fn, &estat))
      {
        if (!S_ISDIR(estat.st_mode))
        {
          docu->doc_url->status |= URL_REDIRECT;
        }
        else
        {
          char *pom;
          char *savepath = url_get_path(docu->doc_url);

          pom = tl_str_concat(DBGvars(NULL), fn,
                              (tl_is_dirname(fn) ? "" : "/"),
                              priv_cfg.index_name, NULL);

          if (!tl_stat(pom, &estat))
          {
            _free(pom);
            if (!S_ISDIR(estat.st_mode))
            {
              url *newurl = url_dup_url(docu->doc_url);
              if (newurl->type != URLT_FILE)
                pom = tl_str_concat(DBGvars(NULL), savepath,
                                    (tl_is_dirname(savepath) ? "" : "/"),
                                    NULL);
              if (newurl->type == URLT_FTP || newurl->type == URLT_FTPS)
                newurl->p.ftp.dir = TRUE;

              url_set_path(newurl, pom);
              _free(pom);

              if (url_redirect_to(docu->doc_url, newurl, FALSE))
                docu->errcode = ERR_HTTP_CYCLIC;
              else
                docu->errcode = ERR_HTTP_REDIR;

              *rv = -1;
              return -1;
            }
          }
          _free(pom);
          fn = url_to_filename(docu->doc_url, TRUE);
        }
      }
    }

    if ((docu->doc_url->type == URLT_FILE
         || (docu->doc_url->status & URL_REDIRECT)) && !docu->load)
    {
      if (!tl_stat(fn, &estat))
      {
        if (S_ISDIR(estat.st_mode))
        {
          docu->errcode = ERR_DIR_URL;
          *rv = -1;
          return -1;
        }
      }
      else
      {
        docu->errcode = ERR_FILE_OPEN;
        *rv = -1;
        return -1;
      }

      if ((!cfg.ftp_html
           && strcmp(tl_get_basename(fn), priv_cfg.index_name)
           && (docu->doc_url->type == URLT_FTP
               || docu->doc_url->type == URLT_FTPS)
           && !docu->doc_url->p.ftp.dir) || !file_is_html(fn))
      {
        docu->is_parsable = FALSE;
        docu->save_online = TRUE;
        docu->size = estat.st_size;
#ifdef I_FACE
        if (cfg.xi_face)
        {
          doc_set_info(docu);
        }
#endif
        if (!cfg.progress || cfg.verbose)
        {
          xprintf(1, gettext("File redirect\n"));
        }
        *rv = 0;
        return -1;
      }
      else
      {
        if (!strcasecmp("css", tl_get_extension(fn)))
          docu->doc_url->status |= URL_STYLE;
        docu->is_parsable = TRUE;
      }
    }
  }
  else
  {
    if (!tl_stat(fn, &estat))
    {
      docu->origsize = estat.st_size;
      /*
       * pro: somehow it must have been forgotten to set the
       * time as well...
       */
      docu->origtime = estat.st_mtime;
    }
  }

  return 0;
}

static int doc_open_existing_in_file(doc *docu, int b_lock, int *rv)
{
  char *inname;
  struct stat estat;

  if (!cfg.no_disc_io
      && (inname = url_to_in_filename(docu->doc_url)))
  {
    if (!tl_stat(inname, &estat) && !S_ISDIR(estat.st_mode))
    {
      if (doc_lock(docu, b_lock))
      {
        docu->errcode = ERR_STORE_DOC;
        _free(inname);
        *rv = -1;
        return -1;
      }

      docu->rest_pos = estat.st_size - cfg.rollback;

      if (docu->rest_pos)
      {
        xprintf(1, gettext("Trying to resume from position %d\n"), docu->rest_pos);

        docu->origtime = estat.st_mtime;
        docu->stime = estat.st_mtime;
        docu->doreget = TRUE;
        docu->remove_lock = FALSE;
      }
    }
    _free(inname);
  }

  return 0;
}

static int doc_open_new_in_file(doc *docu, int b_lock)
{
  if ( /* (cfg.dump_fd->fd < 0) && */ !docu->s_sock && !cfg.no_disc_io)
  {
    if (cfg.post_update && docu->type_str)
    {
      /*
       *  dirty hack, but is required to support
       * file naming by its MIME type
       */
#if 0
      url_forget_filename(docu->doc_url);
      url_to_filename_with_type(docu->doc_url, docu->type_str, TRUE);
#endif
    }

    if (doc_lock(docu, b_lock))
    {
      docu->errcode = ERR_STORE_DOC;
      return -1;
    }
  }

  return 0;
}

/*
 * Download/read the [content] data into 'docu->contents'.
 *
 * When the data being received needs preprocessing
 * (Gopher directory listing, FTP directory listing,
 *  HTTP compressed content), perform the preprocessing.
 *
 * Return a negative error code is something went wrong
 * along the way. A zero(0) return value means success.
 */
static int doc_download_helper(doc *docu, int load, int b_lock)
{
  char *p = NULL;
  ssize_t len = 0;
  int retcode = 0;
  int rv;
  struct stat estat;
  bufio *saved_datasock = NULL;

  ASSERT(!docu->contents);
  if (doc_download_init(docu, load))
    return -1;

  gettimeofday(&docu->hr_start_time, NULL);

  if (doc_check_doc_file(docu, &rv))
    return rv;

  if (doc_open_existing_in_file(docu, b_lock, &rv))
    return rv;

  if (docu->report_size)
  {
    gui_set_status(gettext("Opening connection"));
  }

  if (!(docu->datasock = abs_get_data_socket(docu))
      /*
       * pro: add test for errcodes. The errcode tests are needed;
       * otherwise the "if" some lines later will never get
       * anything to do...
       */
      && docu->errcode != ERR_FTP_ACTUAL && docu->errcode != ERR_HTTP_ACTUAL)
  {
    if (docu->mime
        && docu->doc_url->type != URLT_FILE
        && !(docu->doc_url->status & URL_REDIRECT))
    {
      dinfo_save(docu);
    }
    docu->remove_lock = FALSE;
    DEBUG_DEVEL("abs_close_socket call @ [%d/%s]\n", __LINE__, __FILE__);
    abs_close_socket(docu, FALSE);
    return -1;
  }

  doc_etime(docu, TRUE);

  if (docu->errcode == ERR_HTTP_ACTUAL || docu->errcode == ERR_FTP_ACTUAL)
  {
    DEBUG_DEVEL("abs_close_socket call @ [%d/%s]\n", __LINE__, __FILE__);
    abs_close_socket(docu, FALSE);
    saved_datasock = docu->datasock;

    if (docu->load || docu->is_parsable)
    {
      if (!cfg.progress || cfg.verbose)
      {
        xprintf(1, gettext("Loading local copy\n"));
      }

      p = url_to_filename(docu->doc_url, TRUE);
      if (!(docu->datasock = bufio_open(p, O_BINARY | O_RDONLY)))
      {
        docu->datasock = saved_datasock;
        docu->errcode = ERR_FILE_OPEN;
        return -1;
      }
      docu->doc_url->status |= URL_REDIRECT;
      docu->doc_url->status |= URL_ISLOCAL;
      docu->save_online = FALSE;
    }
    else
    {
      docu->doc_url->status |= URL_REDIRECT;
      return 1;
    }
  }
  else if (docu->errcode == ERR_HTTP_NOREGET)
  {
    if (!cfg.freget)
    {
      docu->is_persistent = FALSE;
      DEBUG_DEVEL("abs_close_socket call @ [%d/%s]\n", __LINE__, __FILE__);
      abs_close_socket(docu, FALSE);
      docu->remove_lock = FALSE;
      docu->ftp_fatal_err = TRUE;
      return -1;
    }
    else
    {
      docu->rest_pos = 0;
    }
  }
  else if (docu->errcode == ERR_FTP_NOREGET)
  {
    if (!cfg.freget)
    {
      docu->is_persistent = FALSE;
      DEBUG_DEVEL("abs_close_socket call @ [%d/%s]\n", __LINE__, __FILE__);
      abs_close_socket(docu, FALSE);
      docu->remove_lock = FALSE;
      docu->ftp_fatal_err = TRUE;
      return -1;
    }
    else
    {
      docu->rest_pos = 0;
    }
  }
  else if (docu->errcode)
  {
    if (docu->mime
        && docu->doc_url->type != URLT_FILE
        && !(docu->doc_url->status & URL_REDIRECT))
    {
      dinfo_save(docu);
    }
    docu->is_persistent = FALSE;
    docu->ftp_fatal_err = TRUE;
    DEBUG_DEVEL("abs_close_socket call @ [%d/%s]\n", __LINE__, __FILE__);
    abs_close_socket(docu, FALSE);
    return -1;
  }

  if (doc_open_new_in_file(docu, b_lock))
  {
    docu->is_persistent = FALSE;
    docu->ftp_fatal_err = TRUE;
    DEBUG_DEVEL("abs_close_socket call @ [%d/%s]\n", __LINE__, __FILE__);
    abs_close_socket(docu, FALSE);
    return -1;
  }

  if (docu->doc_url->type != URLT_FILE
      && !(docu->doc_url->status & URL_REDIRECT))
  {
    dinfo_save(docu);
  }

  if ((((docu->doc_url->type == URLT_FTP
         || docu->doc_url->type == URLT_FTPS)
        && !docu->doc_url->p.ftp.dir)
       || (docu->doc_url->type == URLT_GOPHER
           && (docu->doc_url->p.gopher.selector[0] != '1'
               || docu->doc_url->p.gopher.selector[0] != 'h'))
       || (docu->doc_url->type == URLT_HTTP
           || docu->doc_url->type == URLT_HTTPS))
      && !(docu->doc_url->status & URL_REDIRECT))
  {
#if 0
    if (cfg.dump_fd->fd >= 0 && cfg.dump_after)
      docu->save_online = FALSE;
    else
#endif
    docu->save_online = TRUE;
  }

  if (cfg.ftp_html
      && (docu->doc_url->type == URLT_FTP
          || docu->doc_url->type == URLT_FTPS)
      && ext_is_html(docu->doc_url->p.ftp.path))
  {
    docu->is_parsable = TRUE;
  }

  if (docu->doc_url->status & URL_INNSCACHE)
  {
    if (0 != fstat(bufio_getfd(docu->datasock), &estat))
    {
      xperror1(gettext("Failed to stat info for local NetScape Cached copy"),
               docu->doc_url->local_name);
    }
    else
    {
      docu->totsz = estat.st_size;
      docu->is_parsable = ((docu->doc_url->status & URL_ISHTML) != 0);
    }
  }
  else if (((docu->errcode == ERR_HTTP_ACTUAL)
            || (docu->errcode == ERR_FTP_ACTUAL))
#if 0
           && ((URL_REDIRECT | URL_ISLOCAL) ==
               (docu->doc_url->status & (URL_REDIRECT | URL_ISLOCAL)))
#endif
           && (docu->doc_url->status & URL_ISLOCAL)
           /* && docu->save_online */)
  {
    /*
     * [i_a] fixup for loading LOCAL copy of an item which was transmitted
     * to us in gzip/compress-ed format before: the HTTP Header size info
     * as available in docu->totsz will be incorrect then (too small).
     */
    ASSERT(docu->datasock != NULL);
    ASSERT(docu->datasock->fd > 0);
    ASSERT(docu->doc_url->local_name);
    if (0 != fstat(bufio_getfd(docu->datasock), &estat))
    {
      xperror1(gettext("Failed to stat info for local copy"), docu->doc_url->local_name);
    }
    else
    {
      /*
       * Note: docu->totsz as derived from the HTTP/FTP response may be actually
       * LARGER than the filesize for the local copy on disc: this _can_ be
       * due to URL-rewriting in the local copy, but other causes are possible
       * too. Hence, the sizecheck ASSERT below has been disabled.
       */
#if 0
      ASSERT(docu->totsz <= estat.st_size);
#endif
      docu->totsz = estat.st_size;
    }
  }

  if (docu->errcode == ERR_HTTP_FAILREGET)
  {
    docu->rest_pos = 0;
    docu->save_online = FALSE;
  }

  if (docu->save_online)
  {
    /*    if(cfg.dump_fd->fd < 0) { */
    if (!cfg.no_disc_io && docu->s_sock != NULL)
    {
      bufio_truncate(docu->s_sock, docu->rest_pos);
      bufio_seek(docu->s_sock, docu->rest_pos, SEEK_SET);
      bufio_reset(docu->s_sock);
    }
  }

  /*
   * We measure time-to-first-byte here again, to add all the processing
   * timing noise (1-2ms) to the large value -- FB, which is typically
   * on the order of 100ms, rather then to the time-to-last-byte,
   * which is often around 0ms
   */
  gettimeofday(&docu->first_byte_time, NULL);

  retcode = doc_transfer_data(docu);

#if 0 /* only dump RAW content when it is processed in this routine! */
#if defined (HAVE_DEBUG_FEATURES)
  {
    char dumpbuf[2048];

    DEBUG_PROTOD("********************* Content: RAW **************\n");
    DEBUG_PROTOD("%s\n",
                 tl_asciidump_content(dumpbuf, NUM_ELEM(dumpbuf), docu->contents, docu->size));
    DEBUG_PROTOD("***************************************************\n");
  }
#endif
#endif

  DEBUG_DEVEL("abs_close_socket call @ [%d/%s]\n", __LINE__, __FILE__);
  abs_close_socket(docu, TRUE);

  if (!retcode)
    docu->remove_lock = TRUE;

  if (docu->errcode == ERR_HTTP_ACTUAL)
  {
    docu->doc_url->status &= ~URL_REDIRECT;
    docu->datasock = saved_datasock;
  }

  if (!retcode && docu->doc_url->status & URL_INNSCACHE)
  {
    docu->is_parsable = ((docu->doc_url->status & URL_ISHTML) != 0);
  }
  else if (!retcode
           && docu->doc_url->type == URLT_GOPHER
           && !(docu->doc_url->status & URL_REDIRECT)
           && !(priv_cfg.gopher_proxy && cfg.gopher_via_http))
  {
    docu->is_parsable = FALSE;

    /**** convert Gopher directory to HTML ****/
    if (docu->doc_url->p.gopher.selector[0] == '1')
    {
      if (!(docu->doc_url->status & URL_REDIRECT))
      {
        /* only dump RAW content when it is processed in this routine! */
#if defined (HAVE_DEBUG_FEATURES)
        {
          char dumpbuf[2048];

          DEBUG_PROTOD("************ Content: RAW (Gopher Dir) ************\n");
          DEBUG_PROTOD("%s\n",
                       tl_asciidump_content(dumpbuf, NUM_ELEM(dumpbuf), docu->contents, docu->size));
          DEBUG_PROTOD("***************************************************\n");
        }
#endif

        gopher_dir_to_html(docu);
      }
      docu->is_parsable = TRUE;
    }
    else if (docu->doc_url->p.gopher.selector[0] == 'h')
    {
      docu->is_parsable = TRUE;
    }
  }
  else if (!retcode
           && (docu->doc_url->type == URLT_FTP
               || docu->doc_url->type == URLT_FTPS)
           && !(priv_cfg.ftp_proxy && cfg.ftp_via_http && !cfg.ftp_dirtyp)
           && !(docu->doc_url->status & URL_REDIRECT))
  {
    docu->is_parsable = (ext_is_html(docu->doc_url->p.ftp.path) != 0);

    /*** convert FTP directory listing to HTML ***/
    if (docu->doc_url->p.ftp.dir)
    {
      if (!(docu->doc_url->status & URL_REDIRECT))
      {
        /* only dump RAW content when it is processed in this routine! */
#if defined (HAVE_DEBUG_FEATURES)
        {
          char dumpbuf[2048];

          DEBUG_PROTOD("************ Content: RAW (FTP Dir) ************\n");
          DEBUG_PROTOD("%s\n",
                       tl_asciidump_content(dumpbuf, NUM_ELEM(dumpbuf), docu->contents, docu->size));
          DEBUG_PROTOD("***************************************************\n");
        }
#endif

        ftp_dir_to_html(docu);
      }
      docu->is_parsable = TRUE;
    }
  }
  else if (docu->is_http_transfer && !retcode
           && !(docu->doc_url->status & URL_REDIRECT))
  {
    http_response *resp;

    /*** get HTTP response status info ***/
    resp = http_get_response_info(docu->mime);
    if (resp)
    {
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
      if (docu->actrec_ref)
      {
        activity_record *rec = docu->actrec_ref;

        DEBUG_DEVEL("recording response code: %d [%d/%s]\n", resp->ret_code, __LINE__, __FILE__);
        rec->expected_response_code = resp->ret_code;
      }
#endif

      /*** set proper HTTP error code ***/
      if (resp->ret_code >= 400)
      {
        docu->errcode = HTTP_BASE_ERRVAL + resp->ret_code;
        http_response_free(resp);
        return -1;
      }

      /*** redirect to other URL ***/
      if (resp->ret_code == HTTP_RSP_SEE_OTHER
          || resp->ret_code == HTTP_RSP_FOUND
          || resp->ret_code == HTTP_RSP_REDIR2
          || resp->ret_code == HTTP_RSP_MV_PERMANENT)
      {
        /* only dump RAW content when it is processed in this routine! */
#if defined (HAVE_DEBUG_FEATURES)
        {
          char dumpbuf[2048];

          DEBUG_PROTOD("************ Content: HTTP REDIRECT ************\n");
          DEBUG_PROTOD("%s\n",
                       tl_asciidump_content(dumpbuf, NUM_ELEM(dumpbuf), docu->contents, docu->size));
          DEBUG_PROTOD("***************************************************\n");
        }
#endif

        http_handle_redirect(docu, resp->ret_code);
        http_response_free(resp);
        if (docu->is_persistent)
        {
          if (docu->doc_url->moved_to
              && ((url_get_port(docu->doc_url) !=
                   url_get_port(docu->doc_url->moved_to))
                  || strcmp(url_get_site(docu->doc_url),
                            url_get_site(docu->doc_url->moved_to))))
          {
            DEBUG_DEVEL("abs_close_socket call @ [%d/%s]\n", __LINE__, __FILE__);
            abs_close_socket(docu, TRUE);
          }
          docu->is_persistent = FALSE;
        }
        return -1;
      }

      http_response_free(resp);
    }


    /*
     * check if document was downloaded whole
     * when we know real document size and no
     * other error was detected before
     */
    if (cfg.check_size && docu->totsz > 0 && docu->errcode == ERR_NOERROR)
    {
      /*
       * if docu->contents && docu->rest_pos
       *
       * document was reread from file and
       * docu->size is total len
       */
      if (docu->totsz != docu->size + (docu->contents ? 0 : docu->rest_pos))
      {
        docu->errcode = ERR_HTTP_TRUNC;
        docu->remove_lock = FALSE;
        retcode = -1;
        xprintf(1, gettext("File may be truncated\n"));
      }
    }

    /*** handle encoded document and decode   ***/
    /*** it if possible and user requested it ***/
    /*
     * [i_a] FIXUP: do NOT decompress any files which are compressed themselves, or
     * we'd 'corrupt' the contents ==> allow_decompress
     *
     * ADDITIONAL FIXUP: also do NOT try to decompress files which were just loaded
     * from local storage as pavuk discovered the item at the remote server hasn't
     * changed at all.
     */
    p = get_mime_param_val_str("Content-Encoding:", docu->mime);
    if (cfg.use_enc && !retcode && p
#if 0
        && (!strncasecmp(docu->type_str, "text/plain", 10)
            || !strncasecmp(docu->type_str, "text/css", 8)
            || !strncasecmp(docu->type_str, "text/html", 9))
#else
        && (!strncasecmp(docu->type_str, "application/xml", 15)
            || !strncasecmp(docu->type_str, "text/", 5) /* [i_a] also accept gzipped text/javascript et al */
        )
#endif
    )
    {
      char *urls = url_to_request_urlstr(docu->doc_url, TRUE);
      bool_t allow_decompress = !ext_is_gzipped_file(urls);
      bool_t comes_from_local_depacked_store = (((docu->errcode == ERR_HTTP_ACTUAL)
                                                 || (docu->errcode == ERR_FTP_ACTUAL))
                                                && (docu->doc_url->status & URL_ISLOCAL)
                                                /* && docu->save_online */);
      if (!comes_from_local_depacked_store && allow_decompress)
      {
        /* only dump RAW content when it is processed in this routine! */
#if defined (HAVE_DEBUG_FEATURES)
        {
          char dumpbuf[2048];

          DEBUG_PROTOD("********** Content: RAW (HTTP compressed) *********\n");
          DEBUG_PROTOD("%s\n",
                       tl_asciidump_content(dumpbuf, NUM_ELEM(dumpbuf), docu->contents, docu->size));
          DEBUG_PROTOD("***************************************************\n");
        }
#endif

        if ((!strcasecmp(p, "x-gzip"))
            || (!strcasecmp(p, "gzip"))
            || (!strcasecmp(p, "x-compress"))
            || (!strcasecmp(p, "compress")))
        {
          char *p1 = NULL;

          if (!gzip_decode(docu->contents, docu->size, &p1, &len,
                           (docu->contents ? NULL : docu->lock_fn)))
          {
            docu->size = len;
            _free(docu->contents);
            docu->contents = p1;
            if (!cfg.progress || cfg.verbose)
            {
              xprintf(1, gettext("Decoding document - OK\n"));
            }
          }
          else
          {
            xperror(gettext("Decoding document - failed"));
            _free(p1);
          }
        }
        else if (!strcasecmp(p, "deflate"))
        {
          char *p1 = NULL;

          if (!inflate_decode(docu->contents, docu->size, &p1, &len,
                              (docu->contents ? NULL : docu->lock_fn)))
          {
            docu->size = len;
            _free(docu->contents);
            docu->contents = p1;
            if (!cfg.progress || cfg.verbose)
            {
              xprintf(1, gettext("Deflating document - OK\n"));
            }
          }
          else
          {
            xperror(gettext("Deflating document - failed"));
            _free(p1);
          }
        }
        else
        {
          xprintf(1, gettext("Unsupported document encoding\n"));
        }
      }
      _free(urls);
    }
    else if (p && !retcode)
    {
      xprintf(1, gettext("Received Encoded file but decoding not allowed (untouched) [%s : %s]\n"),
              docu->type_str, p);
    }
    _free(p);
  }
  else
  {
    if (docu->doc_url->type == URLT_FILE
        || (docu->doc_url->status & URL_REDIRECT))
    {
      char *p1 = url_to_filename(docu->doc_url, TRUE);

      if (file_is_html(p1))
      {
        docu->is_parsable = TRUE;
      }
    }
    else
    {
      docu->is_parsable = FALSE;
    }
  }

  if (docu->totsz > 0
      && docu->size == 0
      && (docu->doc_url->type == URLT_HTTP || docu->doc_url->type == URLT_HTTPS))
  {
    if (!docu->errcode)
      docu->errcode = ERR_ZERO_SIZE;
    docu->remove_lock = FALSE;
    retcode = -1;
  }
#ifdef I_FACE
  if (cfg.xi_face)
    doc_set_info(docu);
#endif
  if (!retcode && docu->lock_fn && docu->save_online
      && /* (cfg.dump_fd->fd < 0) && */ !docu->contents
      && (cfg.mode != MODE_NOSTORE)
      && (cfg.mode != MODE_FTPDIR)
      && !(docu->doc_url->status & URL_REDIRECT)
      && (docu->doc_url->type != URLT_FILE))
  {
    char *p1 = url_to_filename(docu->doc_url, TRUE);

    if (!tl_access(p1, F_OK))
    {
      if (tl_unlink(p1))
        xperror1("Cannot unlink/delete file", mk_native(p1));
    }

    if (tl_link(docu->lock_fn, p1))
    {
#if defined (__CYGWIN__) && (defined (WIN32) || defined (__WIN32))
      if (errno != EPERM && errno != EACCES)
#elif (defined (WIN32) || defined (_WIN32))
      /* no working link() on native Win32 */
      if (errno != EPERM && errno != EACCES && errno != EBADF
          && errno != ENOSUP /* no working link() in Win32 native? */)
#elif __BEOS__
      /* ?? no working link() on BeOS ?? */
      if (FALSE)
#else
      if (errno != EPERM)
#endif
      {
        xperror1("Cannot create [symbolic] link for file", mk_native(p1));
      }
      else
      {
        ASSERT(docu->s_sock != NULL);
        if (copy_fd_to_file(bufio_getfd(docu->s_sock), p1))
        {
          /* xperror(mk_native(p1)); */
        }
      }
    }

#ifdef HAVE_UTIME
    if (cfg.preserve_time && docu->dtime)
    {
      struct utimbuf utmbf;

      tl_stat(p1, &estat);
      utmbf.actime = estat.st_atime;
      utmbf.modtime = docu->dtime;
      utime(p1, &utmbf);
    }
#endif

    if (cfg.preserve_perm
        && (docu->doc_url->type == URLT_FTP
            || docu->doc_url->type == URLT_FTPS)
        && docu->doc_url->extension
        && (((ftp_url_extension *)docu->doc_url->extension)->perm > 0))
    {
      chmod(p1, ((ftp_url_extension *)docu->doc_url->extension)->perm);
    }
  }

  return retcode;
}

int doc_download(doc *docu, int load, int b_lock)
{
  int rc = doc_download_helper(docu, load, b_lock);

  gettimeofday(&docu->end_time, NULL);
  time_log(docu, 0, NULL);
  return rc;
}

/********************************************************/
/* ulozi dokument ak je to potrebne vytvori adresare    */
/* FIXME: Translate me!                                 */
/********************************************************/
int doc_store(doc *docu, int overwrite)
{
  char *pom;
  int f;

#ifdef HAVE_UTIME
  struct utimbuf utmbf;
#endif
#ifdef HAVE_UTIME
  struct stat estat;
#endif

  if (cfg.mode == MODE_NOSTORE || cfg.mode == MODE_FTPDIR)
    return 0;

  /*** don't store directory indexes ***/
  if (!cfg.store_index && url_is_dir_index(docu->doc_url))
    return 0;

  pom = url_to_filename(docu->doc_url, TRUE);
  if (makealldirs(pom))
    xperror1("Cannot create directory chain for file", mk_native(pom));

  if (!tl_access(pom, R_OK) && !overwrite)
  {
    return 0;
  }

  /*
   * pro: before we open the file we unlink it. This way we assure that
   * other directory that have a hard link to our (old) file will still
   * have a hard link to the old file.
   */
  if (cfg.remove_before_store)
  {
    tl_unlink(pom);
  }

  if ((f = tl_open(pom, O_BINARY | O_CREAT | O_TRUNC | O_WRONLY,
                   S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR)) == -1)
  {
    if (!tl_access(pom, R_OK))
      tl_unlink(pom);
    xperror1("Failed to open file", mk_native(pom));
    return -1;
  }

  ASSERT(docu->contents);
  if (write(f, docu->contents, docu->size) != docu->size)
  {
    if (!tl_access(pom, R_OK))
      tl_unlink(pom);
    xperror1("Failed to write to file", mk_native(pom));
    close(f);
    return -1;
  }

  close(f);

#ifdef HAVE_UTIME
  if (docu->dtime && cfg.preserve_time)
  {
    utmbf.modtime = docu->dtime;
  }
  else
  {
    utmbf.modtime = docu->stime;
  }

  tl_stat(pom, &estat);
  utmbf.actime = estat.st_atime;
  utime(pom, &utmbf);
#endif

  if ((docu->doc_url->type == URLT_FTP
       || docu->doc_url->type == URLT_FTPS)
      && docu->doc_url->extension
      && cfg.preserve_perm
      && (((ftp_url_extension *)docu->doc_url->extension)->perm > 0))
  {
    chmod(pom, ((ftp_url_extension *)docu->doc_url->extension)->perm);
  }

  return 0;
}

/*** remove improper documents if required ***/
int doc_remove(url *urlr)
{
  char *fn;

#ifdef HAVE_DEBUG_FEATURES
  if (cfg.verbose)
  {
    fn = url_to_urlstr(urlr, FALSE);
    xprintf(1, gettext("Removing improper document : %s\n"), fn);
    _free(fn);
  }
#endif

  fn = url_to_filename(urlr, TRUE);

  if (urlr->type == URLT_FTP || urlr->type == URLT_FTPS)
  {
    char *p;

    p = strrchr(fn, '/');
    if (p)
      p++;
    else
      p = fn;

    /*** if URL FTPdir index ***/
    if (!strcmp(p, priv_cfg.index_name))
      *p = '\0';

    if (cfg.enable_info)
      dinfo_remove(fn);
    return unlink_recursive(fn);
  }
  else
  {
    if (cfg.enable_info)
      dinfo_remove(fn);

    if (!tl_access(fn, F_OK) && tl_unlink(fn))
    {
      xperror1("Cannot unlink/delete file", mk_native(fn));
      return -1;
    }
  }

  return 0;
}

#ifdef I_FACE
/********************************************************/
/* nastavenie info dokumentu pre informaciu pouzivatela */
/* FIXME: Translate me!                                 */
/********************************************************/
static void doc_set_info(doc *docp)
{
#ifdef WITH_TREE
  url_prop *prp = _malloc(sizeof(*prp));

  prp->size = docp->size;
  prp->mdtm = docp->dtime;
  prp->type = NULL;
  switch (docp->doc_url->type)
  {
  case URLT_HTTP:
#ifdef USE_SSL
  case URLT_HTTPS:
#endif
    if (docp->type_str)
      prp->type = tl_strdup(docp->type_str);
    break;

  case URLT_FILE:
    prp->type = tl_strdup(gettext_nop("Local file"));
    break;

  case URLT_GOPHER:
    switch (docp->doc_url->p.gopher.selector[0])
    {
    case '0':
      prp->type = tl_strdup(gettext_nop("Gopher/Text File"));
      break;

    case '1':
      prp->type = tl_strdup(gettext_nop("Gopher/Directory"));
      break;

    case '2':
      prp->type = tl_strdup(gettext_nop("Gopher/CSO phone book"));
      break;

    case '3':
      prp->type = tl_strdup(gettext_nop("Gopher/Error"));
      break;

    case '4':
      prp->type = tl_strdup(gettext_nop("Gopher/BINHEX"));
      break;

    case '5':
      prp->type = tl_strdup(gettext_nop("Gopher/DOS bin"));
      break;

    case '6':
      prp->type = tl_strdup(gettext_nop("Gopher/UUencoded"));
      break;

    case '7':
      prp->type = tl_strdup(gettext_nop("Gopher/Search index"));
      break;

    case '8':
      prp->type = tl_strdup(gettext_nop("Gopher/Telnet session"));
      break;

    case '9':
      prp->type = tl_strdup(gettext_nop("Gopher/bin"));
      break;

    case '+':
      prp->type = tl_strdup(gettext_nop("Gopher/Duplicated server"));
      break;

    case 'T':
      prp->type = tl_strdup(gettext_nop("Gopher/TN3270"));
      break;

    case 'g':
      prp->type = tl_strdup(gettext_nop("Gopher/GIF"));
      break;

    case 'I':
      prp->type = tl_strdup(gettext_nop("Gopher/Image"));
      break;
    }
    break;

  case URLT_FTP:
    if (docp->doc_url->p.ftp.dir)
      prp->type = tl_strdup(gettext_nop("FTP/Directory"));
    else
      prp->type = tl_strdup(gettext_nop("FTP/File"));
    break;

  case URLT_FTPS:
    if (docp->doc_url->p.ftp.dir)
      prp->type = tl_strdup(gettext_nop("FTPS/Directory"));
    else
      prp->type = tl_strdup(gettext_nop("FTPS/File"));
    break;

  default:
    prp->type = tl_strdup(gettext_nop("Unsupported type"));
    break;
  }

  if (!prp->type)
    prp->type = tl_strdup(gettext_nop("Local file"));

  docp->doc_url->prop = prp;
#endif
}
#endif

void doc_init(doc *docp, url *urlp)
{
  memset(docp, 0, sizeof(*docp));
  docp->current_size = 0;
  docp->doreget = FALSE;
  docp->origtime = 0;
  docp->adj_sz = 0;
  docp->load = 0;
  docp->request_header = NULL;
  docp->request_data = NULL;
  docp->request_datalen = 0;
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  if (cfg.hammer_mode == HAMMER_RECORD_AND_REPLAY)
  {
    DEBUG_HAMMER("prepare recording another (new) action\n");
    docp->actrec_ref = get_registered_activityrecord(NULL, urlp);
  }
  else
  {
    docp->actrec_ref = NULL;
  }
#endif

  docp->doc_nr = 0;
  docp->doc_url = urlp;
  docp->mime = NULL;
  docp->type_str = NULL;
  docp->is_parsable = cfg.enable_js && (docp->doc_url->status & URL_ISSCRIPT);
  docp->size = 0;
  docp->totsz = -1;
  docp->contents = NULL;
  docp->save_online = FALSE;
  docp->dtime = 0L;
  docp->stime = 0L;
  docp->rest_pos = 0;
  docp->rest_end_pos = -1;
  docp->etag = NULL;
  docp->errcode = ERR_NOERROR;
  docp->origsize = 0;
  docp->ftp_fatal_err = FALSE;
  docp->ftp_respc = 0;
  docp->ftp_pasv_host = NULL;
  docp->ftp_pasv_port = 0;
  docp->ftp_data_con_finished = FALSE;
  docp->ftp_dir_type = FALSE;
  docp->datasock = NULL;
  docp->ftp_control = NULL;
  docp->s_sock = NULL;
#ifdef USE_SSL
  memset(&docp->ssl_data_con, '\0', sizeof(ssl_connection));
#endif
  docp->num_auth = 0;
  docp->num_proxy_auth = 0;
  docp->auth_digest = NULL;
  docp->auth_proxy_digest = NULL;
  docp->lock_fn = NULL;
  docp->report_size = TRUE;
  docp->check_limits = TRUE;
  docp->remove_lock = FALSE;
  docp->is_http11 = FALSE;
  docp->chunk_size = 0;
  docp->is_chunked = FALSE;
  docp->read_chunksize = FALSE;
  docp->read_trailer = FALSE;
  docp->is_persistent = FALSE;
#ifdef HAVE_MT
  docp->__herrno = 0;
  docp->msgbuf = NULL;
  docp->threadnr = 0;
#endif
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  docp->cyclenr = 0;
#endif
  docp->is_robot = FALSE;
  docp->additional_headers = NULL;
  docp->is_http_transfer = FALSE;
  docp->http_proxy = NULL;
  docp->http_proxy_port = DEFAULT_HTTP_PROXY_PORT;
  docp->http_proxy_10 = FALSE;
  docp->request_type = HTTP_REQ_UNKNOWN;
  docp->connect_host = NULL;
  docp->connect_port = 0;

  timerclear(&docp->hr_start_time);
  timerclear(&docp->redirect_time);
  timerclear(&docp->dns_time);
  timerclear(&docp->connect_time);
  timerclear(&docp->first_byte_time);
  timerclear(&docp->end_time);
}

static char *get_rate_str(char *str, double rate)
{
  if (rate <= 1024.0)
    sprintf(str, "%5.0f  B/s", rate);
  else if (rate <= 1048576.0)
    sprintf(str, "%5.1f kB/s", rate / 1024.0);
  else if (rate <= 1073741824.0)
    sprintf(str, "%5.1f MB/s", rate / 1048576.0);
  else
    sprintf(str, "%5.1f GB/s", rate / 1073741824.0);

  return str;
}

static char *get_time_str(char *str, time_t tm)
{
  sprintf(str, "%ld:%02ld:%02ld", tm / 3600000, (tm % 3600000) / 60000, (tm % 60000) / 1000);

  return str;
}

static char *get_size_str(char *str, int total, int actual)
{
  if (total)
  {
    if (total < 1000000)
      sprintf(str, "%6d / %d B [%5.1f%%]",
              actual, total, (100.0 * (double)actual / (double)total));
    else
      sprintf(str, "%7d / %d kB [%5.1f%%]",
              actual / 1024, total / 1024,
              (100.0 * (double)actual / (double)total));
  }
  else
  {
    if (actual < 1000000)
      sprintf(str, "%6d B", actual);
    else
      sprintf(str, "%6d kB", actual / 1024);
  }
  return str;
}

time_t doc_etime(doc *docp, int init)
{
  /* #ifdef HAVE_GETTIMEOFDAY -- [i_a] removed! */
  if (init)
  {
    gettimeofday(&docp->start_time, NULL);
    return 0;
  }
  else
  {
    struct timeval t;
    gettimeofday(&t, NULL);

    return 1000 * (t.tv_sec - docp->start_time.tv_sec) +
           (t.tv_usec - docp->start_time.tv_usec) / 1000;
  }
}

static double compute_speed_rate(time_t etime, ssize_t size)
{
  return (double)size * 1000.0 / (etime == 0.0 ? 1.0 : etime);
}

static void show_progress(doc *docp, ssize_t adjsz, int dolog)
{
  time_t etime = doc_etime(docp, FALSE);
  double rate = compute_speed_rate(etime, docp->size + adjsz);
  char s_rate[30] = "", s_etime[30] = "", s_rtime[30] = "", s_size[30] = "";
  ftp_url_extension *fe;

  if (docp->doc_url->type == URLT_FTP || docp->doc_url->type == URLT_FTPS)
    fe = (ftp_url_extension *)docp->doc_url->extension;
  else
    fe = NULL;


  if (docp->totsz >= 0 || (fe && fe->size > 0))
  {
    int size = docp->totsz >= 0 ? docp->totsz : fe->size;

    time_t rtime = (time_t)((double)(size - docp->rest_pos)
                            / (double)(docp->size ? docp->size : 10)
                            * (double)((etime != 0.0) ? etime : 1.0))
                   - etime;

    get_time_str(s_rtime, rtime);
    get_size_str(s_size, size, docp->size + docp->rest_pos);
  }
  else
  {
    get_size_str(s_size, 0, docp->size + docp->rest_pos);
  }

  get_rate_str(s_rate, rate);
  get_time_str(s_etime, etime);

  if (cfg.progress && docp->report_size && !cfg.quiet && !cfg.bgmode
#ifdef I_FACE
      && !cfg.xi_face
#endif
  )
  {
    switch (cfg.progress_mode)
    {
    case 0:
      if (*s_rtime)
        xprintf(0, gettext("S: %s [R: %s] [ET: %s] [RT: %s]"),
                s_size, s_rate, s_etime, s_rtime);
      else
        xprintf(0, gettext("S: %s [R: %s] [ET: %s]"), s_size, s_rate, s_etime);
      xprintf(0, " \r");
      break;

    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    default:
      break;
    }
  }

#ifdef I_FACE
  if (docp->report_size && cfg.xi_face)
  {
    gui_set_progress(s_size, s_rate, s_etime, s_rtime);
  }
#endif
}

int doc_lock(doc *docp, int b_lock)
{
  char *lock_name;
  int cyclenr = 0;
  bufio *s_sock = NULL;

  if (cfg.no_disc_io)
    return 0;

  if (!(lock_name = url_to_in_filename(docp->doc_url)))
    return -1;

  do
  {
    if (makealldirs(lock_name))
    {
      if (errno != ENOENT)
      {
        xperror1("Cannot create directory chain for file", mk_native(lock_name));
        docp->errcode = ERR_STORE_DOC;
        break;
      }
      else
        continue;
    }
    else
    {
      if (cyclenr == 1 && (!cfg.progress || cfg.verbose))
      {
        xprintf(0, gettext("Waiting to release document lock on: %s\n"), lock_name);
      }

      s_sock = bufio_copen(lock_name, O_BINARY | O_RDWR | O_CREAT | O_SHORT_LIVED,
                           S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
      if (!s_sock)
      {
        if (errno != ENOENT)
        {
          xperror(mk_native(lock_name));
          docp->errcode = ERR_STORE_DOC;
          break;
        }
      }
      if (s_sock)
      {
        if (bufio_flock(s_sock, FALSE))
        {
          if (!b_lock)
            docp->errcode = ERR_LOCKED;
          bufio_close(s_sock);
          s_sock = NULL;
        }
        else
        {
          tl_msleep(50, FALSE);
          if (tl_access(lock_name, F_OK))
          {
            if (!b_lock)
            {
              docp->errcode = ERR_LOCKED;
            }
            else
            {
              bufio_close(s_sock);
              s_sock = NULL;
            }
          }
        }
      }

      if (b_lock && !s_sock)
      {
        tl_sleep(1);
        cyclenr++;
      }
    }
  } while (b_lock && !s_sock);

  if (!s_sock)
  {
    _free(lock_name);
    docp->s_sock = NULL;
  }
  else
  {
    docp->s_sock = s_sock;
    docp->lock_fn = lock_name;
  }

  return s_sock == NULL;
}

/****************************************************/
/* Unlock document and remove lock file if required */
/****************************************************/
void doc_remove_lock(doc *docp)
{
#ifdef HAVE_UTIME
  struct utimbuf utmbf;
#endif

  if (docp->s_sock)
  {
    DEBUG_LOCKS("Unlocking document %s\n", docp->lock_fn);
    /*_funlock(bufio_getfd(docp->s_sock));*/
    bufio_close(docp->s_sock);
    docp->s_sock = NULL;

    /*
     * required because close() causes modification time
     * change on hard linked file on w2k(maybe generally
     * on winnt or just on ntfs ???
     */
#ifdef HAVE_UTIME
    if (cfg.preserve_time && docp->dtime)
    {
      struct utimbuf utmbf;
      struct stat estat;

      tl_stat(docp->lock_fn, &estat);
      utmbf.actime = estat.st_atime;
      utmbf.modtime = docp->dtime;
      utime(docp->lock_fn, &utmbf);
    }
#endif

    if (docp->remove_lock)
    {
      tl_unlink(docp->lock_fn);
    }
    else
    {
#ifdef HAVE_UTIME
      utmbf.actime = time(NULL);

      if (docp->dtime && cfg.preserve_time)
        utmbf.modtime = docp->dtime;
      else
        utmbf.modtime = docp->stime;

      utime(docp->lock_fn, &utmbf);
#endif
    }

    _free(docp->lock_fn);
  }
}

static void doc_make_clean_dir(doc *docp)
{
  char *p, *ustr;

  ustr = tl_strdup(url_to_filename(docp->doc_url, FALSE));

  if (!docp->mime && cfg.enable_info)
    dinfo_remove(ustr);

  p = strrchr(ustr, '/');
  if (p)
    *p = '\0';

  while (strlen(ustr) > strlen(priv_cfg.cache_dir))
  {
    if (tl_rmdir(ustr))
    {
      if (errno != ENOTEMPTY && errno != ENOENT && errno != EEXIST)
        xperror1("Cannot remove directory", mk_native(ustr));
      break;
    }

    p = strrchr(ustr, '/');
    if (p)
      *p = '\0';
  }

  _free(ustr);
}

void doc_cleanup(doc *docu)
{
  gui_finish_document(docu);

  short_log(docu, docu->doc_url, NULL);

  LOCK_FAILCNT;
  if (!((docu->doc_url->status & URL_DOWNLOADED)
        || (docu->doc_url->status & URL_REDIRECT)))
    cfg.fail_cnt++;

  cfg.process_cnt++;
  UNLOCK_FAILCNT;

#if defined I_FACE
  if (cfg.xi_face)
  {
    gui_tree_set_icon_for_doc(docu);
  }
#endif

  if (docu->errcode && !cfg.no_disc_io)
  {
    char *infn, *fn;
    fn = url_to_filename(docu->doc_url, FALSE);
    infn = url_to_in_filename(docu->doc_url);
    if (tl_access(fn, F_OK) && tl_access(infn, F_OK))
    {
      doc_make_clean_dir(docu);
      url_forget_filename(docu->doc_url);
    }
    _free(infn);
  }

  _free(docu->mime);
  _free(docu->type_str);
  _free(docu->contents);
  _free(docu->request_header);
  _free(docu->request_data);
  _free(docu->etag);
  _free(docu->ftp_pasv_host);
  _free(docu->additional_headers);
  _free(docu->http_proxy);

  /* docu->actrec_ref = NULL; */

  if (!cfg.auth_reuse_nonce)
  {
    if (docu->auth_digest)
      http_digest_deep_free(docu->auth_digest);
    docu->auth_digest = NULL;
  }

  if (!cfg.auth_reuse_proxy_nonce)
  {
    if (docu->auth_proxy_digest)
      http_digest_deep_free(docu->auth_proxy_digest);
    docu->auth_proxy_digest = NULL;
  }

  gui_clear_status();
}

void doc_destroy(doc *docu)
{
  short_log(docu, docu->doc_url, NULL);

  if (docu->s_sock)
  {
    bufio_close(docu->s_sock);
    docu->s_sock = NULL;
  }
  if (docu->datasock)
  {
    bufio_close(docu->datasock);
    docu->datasock = NULL;
  }
  _free(docu->mime);
  _free(docu->type_str);
  _free(docu->contents);
  _free(docu->request_header);
  _free(docu->request_data);
  _free(docu->etag);
  _free(docu->ftp_pasv_host);
  _free(docu->additional_headers);
  _free(docu->http_proxy);
  /*  docu->actrec_ref = NULL; */

  if (!cfg.auth_reuse_nonce)
  {
    if (docu->auth_digest)
      http_digest_deep_free(docu->auth_digest);
    docu->auth_digest = NULL;
  }

  if (!cfg.auth_reuse_proxy_nonce)
  {
    if (docu->auth_proxy_digest)
      http_digest_deep_free(docu->auth_proxy_digest);
    docu->auth_proxy_digest = NULL;
  }
}

#ifdef HAVE_MT
void doc_finish_processing(doc *docp)
{
  dllist *ptr = docp->msgbuf;
  char *logstr = NULL;

  pthread_setspecific(cfg.currdoc_key, (void *)NULL);
  if (ptr)
  {
    bool_t out = bufio_is_open(cfg.logfile);
    LOCK_OUTPUT;
    while (ptr)
    {
      doc_msg *dm = (doc_msg *)ptr->data;

      if (dm->log && out)
      {
        logstr = tl_str_concat(DBGvars(logstr), dm->msg, NULL);
      }
      xprintf(0, "%s", dm->msg);

      _free(dm->msg);
      _free(dm);
      ptr = dllist_remove_entry(ptr, ptr);
    }
    UNLOCK_OUTPUT;
    if (logstr)
    {
      log_str(logstr);
    }
  }
  _free(logstr);
}
#endif /* HAVE_MT */

void doc_update_parent_links(doc *docu)
{
  if ((cfg.mode != MODE_NOSTORE)
      /* (cfg.dump_fd->fd < 0) */
      && (docu->doc_url->type != URLT_FILE)
      && !(docu->doc_url->status & URL_REDIRECT)
      && !(docu->doc_url->status & URL_ISLOCAL)
      && docu->doc_url->parent_url)
  {
    if (cfg.rewrite_links
        && !cfg.all_to_local && !cfg.sel_to_local
        && !cfg.all_to_remote)
    {
      gui_set_status(gettext("Rewriting links inside parent documents"));
      rewrite_parents_links(docu->doc_url, NULL);
    }
  }
}

