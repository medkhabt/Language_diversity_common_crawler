/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "doc.h"
#include "file.h"
#include "http.h"
#include "ftp.h"
#include "gopher.h"
#include "url.h"
#include "mode.h"
#include "nscache.h"
#include "iecache.h"
#include "mozcache.h"
#include "errcode.h"
#include "abstract.h"

static void abs_sleep(void)
{
  int st;

  if(cfg.sleep)
  {
    if(cfg.rsleep)
      st = rand() % cfg.sleep;
    else
      st = cfg.sleep;

    xprintf(1, gettext("Suspending download for %d seconds.\n"), st);
    tl_sleep(st);
  }
}

/********************************************************/
/* parameter  -  URL dokumentu                          */
/* vracia deskriptor soketu alebo suboru pre dokument   */
/* osetrenie vyskytu v lokalnom strome                  */
/* FIXME: translate me                                  */
/********************************************************/
bufio *abs_get_data_socket(doc * docp)
{
  char *fn;
  bufio *sock;
  struct stat estat;
  url *urlr = docp->doc_url;

  docp->errcode = ERR_NOERROR;

  urlr->status &= ~URL_REDIRECT;

  if(cfg.mode != MODE_SYNC && cfg.mode != MODE_MIRROR)
  {
    fn = url_to_filename(urlr, TRUE);

    if(!tl_access(fn, R_OK) && urlr->type != URLT_FILE)
    {
      urlr->status |= URL_REDIRECT;
      if(tl_stat(fn, &estat) == 0)
      {
        if(!S_ISDIR(estat.st_mode))
        {
          if(!(sock = bufio_open(fn, O_BINARY | O_RDONLY)))
          {
            xperror(mk_native(fn));
            docp->errcode = ERR_FILE_OPEN;
          }
          if(!cfg.progress || cfg.verbose)
          {
            xprintf(1, gettext("File redirect\n"));
          }

          docp->totsz = estat.st_size;
          if(docp->datasock)
          {
            docp->is_persistent = FALSE;
            DEBUG_DEVEL("abs_close_socket call @ [%d/%s]\n", __LINE__, __FILE__);
            abs_close_socket(docp, FALSE);
          }
          return sock;
        }
      }
    }
#ifdef HAVE_BDB_18x
    else if(cfg.ns_cache_dir && urlr->type != URLT_FILE)
    {
      char *cfn;
      char *urlstr = url_to_urlstr(urlr, FALSE);

      cfn = ns_cache_find_localname(urlstr);
      _free(urlstr);

      if(cfn)
      {
        sock = bufio_open(cfn, O_BINARY | O_RDONLY);

        if(sock)
        {
          if(docp->datasock)
          {
            docp->is_persistent = FALSE;
            DEBUG_DEVEL("abs_close_socket call @ [%d/%s]\n", __LINE__, __FILE__);
            abs_close_socket(docp, FALSE);
          }
          /*!!! clever will be to look at MIME type !!! */
          if(file_is_html(cfn))
            urlr->status |= URL_ISHTML;
          urlr->status |= URL_INNSCACHE;
          xprintf(1, gettext("Loading copy from local NS cache - %s\n"), cfn);
          if(tl_stat(cfn, &estat) == 0)
            docp->totsz = estat.st_size;
          _free(cfn);
          return sock;
        }
        _free(cfn);
      }
    }
    else if(cfg.moz_cache_dir && urlr->type != URLT_FILE)
    {
      char *cfn;
      char *urlstr = url_to_urlstr(urlr, FALSE);

      cfn = moz_cache_find_localname(urlstr);
      _free(urlstr);

      if(cfn)
      {
        sock = bufio_open(cfn, O_BINARY | O_RDONLY);

        if(sock)
        {
          if(docp->datasock)
          {
            docp->is_persistent = FALSE;
            DEBUG_DEVEL("abs_close_socket call @ [%d/%s]\n", __LINE__, __FILE__);
            abs_close_socket(docp, FALSE);
          }
          /*!!! clever will be to look at MIME type !!! */
          if(file_is_html(cfn))
            urlr->status |= URL_ISHTML;
          urlr->status |= URL_INNSCACHE;
          xprintf(1, gettext("Loading copy from local Mozilla cache - %s\n"), cfn);
          if(tl_stat(cfn, &estat) == 0)
            docp->totsz = estat.st_size;
          _free(cfn);
          return sock;
        }
        _free(cfn);
      }
    }
#endif
#if defined(__CYGWIN__) || defined(WIN32) || defined(__WIN32)
    else if(cfg.ie_cache && urlr->type != URLT_FILE)
    {
      char *cfn;
      char *urlstr = url_to_urlstr(urlr, FALSE);

      cfn = ie_cache_find_localname(urlstr);
      _free(urlstr);

      if(cfn)
      {
        sock = bufio_open(cfn, O_BINARY | O_RDONLY);

        if(sock)
        {
          if(docp->datasock)
          {
            docp->is_persistent = FALSE;
            DEBUG_DEVEL("abs_close_socket call @ [%d/%s]\n", __LINE__, __FILE__);
            abs_close_socket(docp, FALSE);
          }
          if(file_is_html(cfn))
            urlr->status |= URL_ISHTML;
          urlr->status |= URL_INNSCACHE;
          xprintf(1, gettext("Loading copy from local MSIE cache - %s\n"), cfn);
          if(tl_stat(cfn, &estat) == 0)
            docp->totsz = estat.st_size;
          _free(cfn);
          return sock;
        }
        _free(cfn);
      }
    }
#endif
  }

  if(docp->is_http_transfer)
  {
    abs_sleep();
    urlr->status &= ~URL_REDIRECT;
    return http_get_data_socket(docp);
  }
  else if(urlr->type == URLT_FTP || urlr->type == URLT_FTPS)
  {
    abs_sleep();
    urlr->status &= ~URL_REDIRECT;
    return ftp_get_data_socket(docp);
  }
  else if(urlr->type == URLT_GOPHER)
  {
    abs_sleep();
    urlr->status &= ~URL_REDIRECT;
    return gopher_get_data_socket(docp);
  }
  else if(urlr->type == URLT_FILE)
  {
    urlr->status &= ~URL_REDIRECT;
    return get_file_data_socket(docp);
  }
  xprintf(1, gettext("Unsupported URL\n"));
  return NULL;
}

/*
   Return TRUE _only_ when the connection related to this document
   (being downloaded) is allowed to remain open so pavuk can
   re-use it to transmit another request to the server.

   This is only allowed when the (web) server is RFC2616 compliant,
   no errors have occurred yet and when the user did not disable
   the use of persistent connections alltogether.
 */
int should_keep_persistent(doc * docp)
{
  return (cfg.persistent && docp->is_persistent &&
    !(docp->doc_url->status & URL_REDIRECT) &&
    docp->errcode != ERR_STORE_DOC &&
    docp->errcode != ERR_UNKNOWN &&
    docp->errcode != ERR_READ &&
    docp->errcode != ERR_BIGGER &&
    docp->errcode != ERR_NOMIMET &&
    docp->errcode != ERR_BREAK &&
    docp->errcode != ERR_OUTTIME &&
    docp->errcode != ERR_SMALLER &&
    docp->errcode != ERR_LOW_TRANSFER_RATE &&
    docp->errcode != ERR_QUOTA_FILE &&
    docp->errcode != ERR_QUOTA_TRANS &&
    docp->errcode != ERR_QUOTA_FS &&
    docp->errcode != ERR_QUOTA_TIME &&
    docp->errcode != ERR_BAD_CONTENT &&
    docp->errcode != ERR_HTTP_UNKNOWN &&
    docp->errcode != ERR_HTTP_TRUNC &&
    docp->errcode != ERR_HTTP_SNDREQ &&
    docp->errcode != ERR_HTTP_NOREGET &&
    docp->errcode != ERR_HTTP_CLOSURE && 
    docp->errcode != ERR_HTTP_TIMEOUT);
}

/********************************************************/
/* close socket for current document if should          */
/********************************************************/
void abs_close_socket(doc * docp, int read_status)
{
  url *urlr = docp->doc_url;
  bool_t keep_persistent = FALSE;

  if(!docp->datasock)
    return;

  switch (urlr->type)
  {
  case URLT_FILE:
    bufio_close(docp->datasock);
    docp->datasock = NULL;
    break;

  case URLT_HTTP:
  case URLT_HTTPS:
    if(should_keep_persistent(docp))
    {
      keep_persistent = TRUE;
      DEBUG_NET("Leaving persistent HTTP connection open\n");
      break;
    }
    else
    {
      bufio_close(docp->datasock);
      docp->datasock = NULL;
    }
    break;

  case URLT_GOPHER:
    if(docp->is_http_transfer && should_keep_persistent(docp))
    {
      keep_persistent = TRUE;
      DEBUG_NET("Leaving persistent HTTP connection open\n");
      break;
    }
    else
    {
      bufio_close(docp->datasock);
      docp->datasock = NULL;
    }
    break;

  case URLT_FTP:
  case URLT_FTPS:
    if(docp->is_http_transfer && should_keep_persistent(docp))
    {
      keep_persistent = TRUE;
      DEBUG_NET("Leaving persistent HTTP connection open\n");
      break;
    }
    else
    {
      bufio_close(docp->datasock);
      docp->datasock = NULL;
    }

    if(urlr->status & URL_REDIRECT)
      break;

    if(docp->ftp_control && read_status)
    {
		int reply;

      if((reply = ftp_get_response(docp, NULL, FALSE)) >= 400)
      {
		  if (reply == 600)
		  {
			xprintf(1, gettext("Warning: broken ftp transfer ...\n"));
		  }
		  else
		  {
			xprintf(1, gettext("Warning: broken ftp transfer (FTP server response code %03d) ...\n"),
				reply);
		  }
        docp->errcode = ERR_FTP_TRUNC;
        docp->ftp_fatal_err = TRUE;
      }
    }

    if((docp->errcode == ERR_NOERROR) && cfg.del_after)
    {
      if(ftp_remove(docp))
      {
        xprintf(1, gettext("Error removing FTP document from remote server\n"));
      }
    }

    if(docp->ftp_control && docp->ftp_fatal_err)
    {
      bufio_close(docp->ftp_control);
      docp->ftp_control = NULL;
    }
    break;

  default:
    bufio_close(docp->datasock);
    docp->datasock = NULL;
    break;
  }

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  if(docp->actrec_ref)
  {
    activity_record *rec = docp->actrec_ref;

    DEBUG_DEVEL("abs_close_socket: set persistent flag: %d [%d/%s]\n", keep_persistent, __LINE__, __FILE__);
    rec->expect_persistent = keep_persistent;
  }
#endif
}

int abs_read(bufio * sock, char *buf, size_t bufsize)
{
  return bufio_nbfread(sock, buf, bufsize);
}

int abs_readln(bufio * sock, char *buf, size_t bufsize)
{
  return bufio_readln(sock, buf, bufsize);
}

int abs_write(bufio * sock, char *buf, size_t bufsize)
{
#if 1
/* #if 0 : this call is VERY dangerous and WILL cause chunky to fail: bufio_write
now uses the buf cache too! :-( */
  return bufio_write(sock, buf, bufsize);
#else
  return bufio_tl_write4abs(sock, buf, bufsize);
#endif
}

int abs_read_data(doc * docp, bufio * sock, char *buf, size_t bufsize)
{
  int rv;

  if(docp->is_http11 && docp->is_chunked)
  {
    char pombuf[BUFIO_ADVISED_READLN_BUFSIZE];

    rv = 0;
    if(docp->read_chunksize)
    {
      char *endp;

      rv = abs_readln(sock, pombuf, sizeof(pombuf) - 1);
      if(rv <= 0)
      {
        if(rv == 0 && !errno && cfg.ignore_RFC2616_chunk_bug)
        {
          rv = 0;
          docp->read_chunksize = FALSE;
          docp->read_trailer = FALSE;
          /*
             Make sure no further requests are transmitted across this
             (possibly persistent!) connection
           */
          docp->is_persistent = FALSE;
          xprintf(1, gettext("Ignoring RFC2616 chunked transfer protocol error while reading document.\n"));
        }
        else
        {
          xprintf(1,
            gettext
            ("Error reading document with \"chunked\" transfer encoding! (The \"-ignore_chunk_bug\" commandline option may help.)\n"));
          rv = -1;
        }
      }
      else
      {
        if(cfg.dump_resp && bufio_is_open(cfg.dump_fd) && !cfg.dump_after)
        {
          ASSERT(cfg.dump_fd);
          ASSERT(bufio_is_open(cfg.dump_fd));

          /* [i_a] only dump data if it was just recently fetched. Conditional here prevents this data from
             being appended to actual (probably non-parsable) data dumped just previously.

             Should really only dump if load=0
           */
          ASSERT(!docp->load ? TRUE : !docp->is_parsable);
          ASSERT(!docp->load ? TRUE : !bufio_is_sock(docp->datasock));
          if(!docp->load || docp->is_parsable || bufio_is_sock(docp->datasock))
          {
            bufio_asciidump(cfg.dump_fd, pombuf, rv, NULL);
          }
        }

        /*
           read size of chunk in HEX, ignore the RFC2616
           optional chunk-extension following it (RFC2616, sect 3.6.1):
         */
        docp->chunk_size = strtol(pombuf, &endp, 16);
        docp->read_chunksize = FALSE;
        if(docp->chunk_size == 0)
          docp->read_trailer = TRUE;
        rv = 0;
      }
    }
    if(!rv && docp->read_trailer)
    {
      /*
         read any optional trailer lines sent by the server (RFC2616, sect 3.6.1, item (b)):

         The 'trailer' is terminated by an empty line (= CRLF only).
       */
      while((rv = abs_readln(sock, pombuf, sizeof(pombuf) - 1)) >= 0)
      {
        if(!rv)
        {
          xprintf(1, gettext("Error while reading RFC2616 chunk trailer for document with \"chunked\" transfer encoding!\n"));
          rv = -1;
          break;
        }
        else
        {
          if(cfg.dump_resp && bufio_is_open(cfg.dump_fd) && !cfg.dump_after)
          {
            ASSERT(cfg.dump_fd);
            ASSERT(bufio_is_open(cfg.dump_fd));

            /* [i_a] only dump data if it was just recently fetched. Conditional here prevents this data from
               being appended to actual (probably non-parsable) data dumped just previously.

               Should really only dump if load=0
             */
            ASSERT(!docp->load ? TRUE : !docp->is_parsable);
            ASSERT(!docp->load ? TRUE : !bufio_is_sock(docp->datasock));
            if(!docp->load || docp->is_parsable || bufio_is_sock(docp->datasock))
            {
              bufio_asciidump(cfg.dump_fd, pombuf, rv, NULL);
            }
          }
        }
        if(strcspn(pombuf, "\r\n") == 0)
        {
          rv = 0;
          break;
        }
      }
    }
    if(!rv && docp->chunk_size > 0)
    {
      size_t rs;
      rs = (bufsize < (size_t) docp->chunk_size) ? bufsize : (size_t) docp->chunk_size;
      rv = bufio_nbfread(sock, buf, rs);

      if(rv > 0)
        docp->chunk_size -= rv;

      if(docp->chunk_size == 0)
      {
        /* do NOT touch 'rv' while reading the 'chunk'-terminating CRLF here (RFC2616) */
        int len = abs_readln(sock, pombuf, sizeof(pombuf) - 1);

        if(len > 0 && cfg.dump_resp && bufio_is_open(cfg.dump_fd) && !cfg.dump_after)
        {
          ASSERT(cfg.dump_fd);
          ASSERT(bufio_is_open(cfg.dump_fd));

          /* [i_a] only dump data if it was just recently fetched. Conditional here prevents this data from
             being appended to actual (probably non-parsable) data dumped just previously.

             Should really only dump if load=0
           */
          ASSERT(!docp->load ? TRUE : !docp->is_parsable);
          ASSERT(!docp->load ? TRUE : !bufio_is_sock(docp->datasock));
          if(!docp->load || docp->is_parsable || bufio_is_sock(docp->datasock))
          {
            bufio_asciidump(cfg.dump_fd, pombuf, rv, NULL);
          }
        }

        docp->read_chunksize = TRUE;
      }
    }
  }
  else if((docp->is_persistent ||
      (cfg.check_size && docp->doc_url->type == URLT_HTTP)) &&
    !(docp->doc_url->status & URL_REDIRECT) && docp->totsz >= 0)
  {
    size_t rs;

    rs = (docp->totsz - docp->rest_pos) - docp->size;
    if(rs > bufsize)
      rs = bufsize;

    if(rs)
    {
      rv = bufio_nbfread(sock, buf, rs);
    }
    else
    {
      rv = 0;
    }
  }
  else
  {
    rv = bufio_nbfread(sock, buf, bufsize);
  }

  return rv;
}
