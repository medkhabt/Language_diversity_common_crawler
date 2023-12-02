/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "url.h"
#include "doc.h"
#include "tools.h"
#include "html.h"
#include "http.h"
#include "ftp.h"
#include "myssl.h"
#include "abstract.h"
#include "recurse.h"
#include "mime.h"
#include "robots.h"
#include "mode.h"
#include "times.h"
#include "stats.h"
#include "errcode.h"
#include "cookie.h"
#include "log.h"
#include "gui_api.h"
#include "form.h"
#include "ainterface.h"
#include "gcinfo.h"




#define SETNEXTURL                \
  doc_cleanup(docu);              \
  _free(pstr);                    \
  return docu->errcode



static void reschedule_url(url *urlp)
{
  DEBUG_MISC(gettext("Rescheduling locked URL as no. %d\n"), cfg.total_cnt);

  LOCK_CFG_URLSTACK;
  cfg.urlstack = dllist_append(cfg.urlstack, (dllist_t)urlp);
#ifdef HAVE_MT
  mt_semaphore_up(&cfg.urlstack_sem);
#endif
  UNLOCK_CFG_URLSTACK;
  LOCK_TCNT;
  cfg.total_cnt++;
  UNLOCK_TCNT;
}

static void run_post_command(doc *docp)
{
  char *urlstr;
  char *cmd;

  DEBUG_MISC(gettext("Running post-processing command\n"));
  urlstr = url_to_urlstr(docp->doc_url, TRUE);

  cmd = tl_str_concat(DBGvars(NULL), priv_cfg.post_cmd, " \'",
                      url_to_filename(docp->doc_url, FALSE),
                      docp->is_parsable ? "\' 1 \'" : "\' 0 \'", urlstr, "\'", NULL);

  _free(urlstr);

  tl_system(cmd);

  _free(cmd);
}

/*
 * [i_a] 'ui' may be NULL: will fill all fields from HTML form only
 */
static void add_matching_form(doc *docp, int nform, url_info *ui)
{
  char *ftext;
  int flen;
  form_info *fi;
  dllist *ptr, *fields, *sfields;
  cookie_entry *cookies = NULL;
  url_info *nui;

  if (!(ftext = form_get_text(nform, docp->contents, docp->size, &flen)))
  {
    return;
  }

  fi = form_parse(ftext, flen, ui);
  if (!fi)
    return;

  /* copy all fields supplied on cmdln */
  fields = NULL;
  cookies = NULL;
  if (ui)
  {
    cookie_entry *cptr;

    for (ptr = ui->fields; ptr; ptr = ptr->next)
    {
      fields = dllist_prepend(fields, (dllist_t)form_field_duplicate((form_field *)ptr->data));
    }
    for (cptr = ui->cookies; cptr; cptr = cptr->next)
    {
      cookie_entry *c = dup_cookie(cptr);
      c->next = cookies;
      if (cookies)
        cookies->prev = c;
      cookies = c;
    }
  }

  /* copy all suitable fields from HTML form */
  sfields = NULL;
  form_get_default_successful(NULL, fi->infos, &sfields);

  for ( ; sfields; sfields = dllist_remove_entry(sfields, sfields))
  {
    form_field *ff = (form_field *)sfields->data;

    if (dllist_find2(fields, (dllist_t)ff, form_field_compare_name))
    {
      _free(ff->name);
      _free(ff->value);
      _free(ff);
    }
    else
    {
      fields = dllist_prepend(fields, (dllist_t)ff);
    }
  }

  nui = url_info_new(fi->action);
  nui->type = URLI_FORM;
  nui->fields = fields;
  nui->cookies = cookies;
  nui->encoding = fi->encoding;
  nui->method = fi->method;
  if (ui)
  {
    nui->localname = tl_strdup(ui->localname);
  }
  else
  {
    nui->localname = NULL;
  }

  form_free(fi);

#ifdef HAVE_DEBUG_FEATURES
  if (cfg.verbose)
  {
    char *dbgmsg = cvt_url_info2diag_str(nui);
    DEBUG_DEVEL("add_matching_form[%d]: before append_starting_url: URL INFO = %s", nform, dbgmsg);
    _free(dbgmsg);
  }
#endif
  append_starting_url(nui, docp->doc_url);
#ifdef HAVE_DEBUG_FEATURES
  if (cfg.verbose)
  {
    char *dbgmsg = cvt_url_info2diag_str(nui);
    DEBUG_DEVEL("add_matching_form[%d]: after append_starting_url: URL INFO = %s", nform, dbgmsg);
    _free(dbgmsg);
  }
#endif

  url_info_free(nui);
  nui = NULL;
}

static void add_matching_forms(doc *docp, dllist *formlist)
{
  dllist *fptr, *uptr;
  int nform;

  for (fptr = formlist, nform = 0; fptr; fptr = fptr->next, nform++)
  {
    url *urlp;
    bool_t found = FALSE;

    DEBUG_DEVEL("add_matching_forms[%d]: '%s'\n", nform, (char *)fptr->data);

    urlp = url_parse((char *)fptr->data);

    ASSERT(urlp->type != URLT_FROMPARENT);

    if ((urlp->type != URLT_HTTP) && (urlp->type != URLT_HTTPS))
    {
      free_deep_url(urlp);
      _free(urlp);
      continue;
    }
    free_deep_url(urlp);
    _free(urlp);

    for (uptr = priv_cfg.formdata; uptr; uptr = uptr->next)
    {
      url_info *ui = (url_info *)uptr->data;

      DEBUG_DEVEL("forms match check: '%s' =?= '%s'\n", ui->urlstr, (char *)fptr->data);

      if (!strcmp(ui->urlstr, (char *)fptr->data))
      {
        DEBUG_HTMLFORM("forms match check: Added matched form: '%s'\n", ui->urlstr);
        /* include form multiple times, if matched by different formdata items */
#ifdef HAVE_DEBUG_FEATURES
        if (cfg.verbose)
        {
          char *dbgmsg = cvt_url_info2diag_str(ui);
          DEBUG_HTMLFORM("add_matching_forms[%d]: Added matched form: URL INFO = %s",
                         nform,
                         dbgmsg);
          _free(dbgmsg);
        }
#endif
        add_matching_form(docp, nform, ui);
        found = TRUE;
      }
    }

    /* [i_a] addition: just add all non-matched forms too! */
    if (!found)
    {
      DEBUG_HTMLFORM("%s unmatched form '%s'\n", (cfg.try_all_forms ? "Added" : "Skipped"),
                     (char *)fptr->data);
      if (cfg.try_all_forms)
      {
        add_matching_form(docp, nform, NULL);
      }
    }
  }
}

int process_document(doc *docu, int check_lim)
{
  url *urlr;
  int nreget = 0;
  int nredir = 0;
  int pokus = 0;
  time_t atm;
  char cpom[64];
  char *pstr = NULL;
  int store_stat;
  struct stat estat;

  urlr = docu->doc_url;

  docu->check_limits = check_lim;

  _Xt_Serve;

  if (docu->check_limits)
    docu->check_limits = (urlr->parent_url != NULL);

  while (!cfg.stop && !cfg.rbreak)
  {
    _free(docu->ftp_pasv_host);
    docu->errcode = ERR_NOERROR;
    docu->mime = NULL;
    docu->type_str = NULL;
    docu->doc_url = urlr;
    docu->dtime = 0L;
    docu->contents = NULL;
    docu->request_header = NULL;
    docu->request_data = NULL;
    docu->request_datalen = 0;
//    docu->actrec_ref = NULL;
    docu->is_chunked = FALSE;
    docu->read_chunksize = FALSE;
    docu->read_trailer = FALSE;
    docu->ftp_fatal_err = FALSE;
    pstr = url_to_urlstr(urlr, FALSE);

    if (pokus)
    {
      xprintf(1, gettext("retry no. %d/%d\n"), pokus, cfg.nretry);
    }

    if (cfg.progress || cfg.verbose)
    {
      switch (cfg.progress_mode)
      {
      case 0:
#ifdef HAVE_MT
        xprintf(1, gettext("URL[%2d]: %5d(%d) of %5d  %s\n"), docu->threadnr + 1, docu->doc_nr, cfg.fail_cnt, cfg.total_cnt, pstr);
#else
        xprintf(1, gettext("URL: %5d(%d) of %5d  %s\n"), docu->doc_nr, cfg.fail_cnt, cfg.total_cnt, pstr);
#endif
        break;

      case 1:
      default:
        print_progress_style(0, NULL);
        break;
      }
    }

#ifdef I_FACE
    if (cfg.xi_face)
    {
      gui_set_doccounter();

      gui_set_url(pstr);

      gui_set_status(gettext("Starting download"));
    }
#endif

    /*** to be able to revisit moved documents ***/
    /*** especially for authorization purposes ***/
    if ((urlr->status & URL_PROCESSED) && urlr->moved_to && nredir)
    {
      urlr->status &= ~URL_PROCESSED;
    }

    if (docu->check_limits)
    {
      cond_info_t condp;

      condp.level = 2;
      condp.urlnr = docu->doc_nr;
      condp.size = 0;
      condp.time = 0L;
      condp.mimet = NULL;
      condp.full_tag = NULL;
      condp.params = NULL;
      condp.html_doc = NULL;
      condp.html_doc_offset = 0;
      condp.tag = NULL;
      condp.attrib = NULL;

      if (urlr->status & URL_PROCESSED)
      {
        xprintf(1, gettext("Already processed\n"));
        docu->errcode = ERR_PROCESSED;
        SETNEXTURL;
      }

      if (urlr->status & URL_USER_DISABLED)
      {
        xprintf(1, gettext("Disallowed by user\n"));
        docu->errcode = ERR_UDISABLED;
        SETNEXTURL;
      }

      if (!prottable[urlr->type].supported
          || (urlr->parent_url
              && (urlr->type == URLT_FTP
                  || urlr->type == URLT_FTPS)
              && urlr->p.ftp.dir
              && !cfg.condition.ftpdir)
          || (urlr->parent_url
              && !url_append_condition(urlr, &condp)))
      {
        if (!cfg.verbose)
        {
          xprintf(1, gettext("Disallowed by rules\n"));
        }
        else
        {
          char *urlstr = NULL;
          char *urldbgstr = NULL;
          char *typ_str = "UNKNOWN";

          switch (urlr->type)
          {
          case URLT_HTTP:
            typ_str = "HTTP";
            break;

          case URLT_HTTPS:
            typ_str = "HTTPS";
            break;

          case URLT_FTP:
            typ_str = "FTP";
            break;

          case URLT_FTPS:
            typ_str = "FTPS";
            break;

          case URLT_FILE:
            typ_str = "FILE";
            break;

          case URLT_GOPHER:
            typ_str = "GOPHER";
            break;

          case URLT_FROMPARENT:
            typ_str = "FROMPARENT";
            break;

          default:
            ASSERT(strcmp(typ_str, "UNKNOWN") == 0);
            break;
          }

          cvt_url2diag_str(urlr, &urldbgstr, &urlstr);
          xprintf(1, gettext("Disallowed by rules:\n%s"
                             "-- Supported type: %d (%s)\n"), urldbgstr,
                  prottable[urlr->type].supported, typ_str);
          _free(urldbgstr);
          _free(urlstr);
        }

        urlr->status |= URL_REJECTED;
        docu->errcode = ERR_RDISABLED;
        SETNEXTURL;
      }

      if (may_do_a_robots_check(urlr))
      {
        gui_set_status(gettext("Checking \"robots.txt\""));

        if (!robots_check(urlr))
        {
          xprintf(1, gettext("Disallowed by \"robots.txt\"\n"));
          urlr->status |= URL_REJECTED;
          docu->errcode = ERR_RDISABLED;
          SETNEXTURL;
        }
      }
    }

    if (cfg.mode == MODE_FTPDIR && (urlr->type != URLT_FTP && urlr->type != URLT_FTPS))
    {
      xprintf(1, gettext("This URL type is not supported with ftpdir mode\n"));

      urlr->status |= URL_REJECTED;
      docu->errcode = ERR_RDISABLED;
      SETNEXTURL;
    }

    _Xt_Serve;

    if (cfg.mode == MODE_SYNC)
    {
      char *pp = url_to_filename(urlr, TRUE);

      if (!tl_stat(pp, &estat) && !S_ISDIR(estat.st_mode))
      {
        atm = time(NULL) - 86400 * cfg.ddays;
        /*
         * pro: We do not want the message
         * "No transfer - file not expired"
         * if the server's clock is ahead of our clock.
         * If no parameter cfg.ddays is given, then
         * we do not compare the file modification times.
         */
        if (cfg.ddays == 0 || estat.st_mtime < atm)
          docu->dtime = estat.st_mtime;
        else
        {
          xprintf(1, gettext("No transfer - file not expired\n"));
          urlr->status |= URL_REJECTED;
          docu->errcode = ERR_RDISABLED;
          SETNEXTURL;
        }
        urlr->status |= URL_ISLOCAL;
        docu->origsize = estat.st_size;
      }
    }

    if (cfg.show_time)
    {
      atm = time(NULL);
      LOCK_TIME;
      strftime(cpom, sizeof(cpom), "%H:%M:%S", localtime(&atm));
      UNLOCK_TIME;
      xprintf(1, gettext("Starting time :  %s\n"), cpom);
    }

#ifdef I_FACE
    if (cfg.stop || cfg.rbreak)
    {
      _free(pstr);
      break;
    }
#endif

    _Xt_Serve;

    if ((urlr->type == URLT_FTP || urlr->type == URLT_FTP)
        && urlr->extension
        && ((ftp_url_extension *)urlr->extension)->type == FTP_TYPE_L
        && ((ftp_url_extension *)urlr->extension)->slink)
    {
      if (cfg.retrieve_slink)
      {
        /** need to kill extension, because we must **/
        /** guess the file type beside the symlink  **/
        ftp_url_ext_free(urlr->extension);
        urlr->extension = NULL;
      }
      else
      {
        ftp_make_symlink(urlr);
        urlr->status |= URL_PROCESSED;
        docu->errcode = ERR_NOERROR;
        SETNEXTURL;
      }
    }

    gui_set_status(gettext("Starting download"));

    ASSERT(!docu->contents);
    if (doc_download(docu, FALSE, FALSE))
    {
#ifdef HAVE_DEBUG_FEATURES
      char dumpbuf[2048];
#endif

      DEBUG_PROTOD("********************* Response: Content **************\n");
      DEBUG_PROTOD("%s\n", tl_asciidump_content(dumpbuf, NUM_ELEM(dumpbuf), docu->contents, docu->size));
      DEBUG_PROTOD("***************************************************\n");

      if (cfg.show_time)
      {
        atm = time(NULL);
        LOCK_TIME;
        strftime(cpom, sizeof(cpom), "%H:%M:%S", localtime(&atm));
        UNLOCK_TIME;
        xprintf(1, gettext("Ending time :    %s\n"), cpom);
      }

      _Xt_Serve;
      doc_remove_lock(docu);
      _free(docu->contents);
      _free(docu->request_header);
      _free(docu->request_data);
      //     docu->actrec_ref = NULL;

      DEBUG_USER("Error status code - (%d)\n", docu->errcode);
      report_error(docu, gettext("download"));

      if ((nreget < cfg.nreget
           && (docu->errcode == ERR_HTTP_TRUNC
               || docu->errcode == ERR_FTP_TRUNC
               || docu->errcode == ERR_LOW_TRANSFER_RATE
               || docu->errcode == ERR_HTTP_FAILREGET
               || docu->errcode == ERR_HTTP_TIMEOUT
               || docu->errcode == ERR_HTTP_GW_TIMEOUT))
          || (nredir < cfg.nredir
              && docu->errcode == ERR_HTTP_REDIR)
          || (docu->errcode == ERR_HTTP_AUTH)
          || (docu->errcode == ERR_HTTP_PROXY_AUTH))
      {
        if (docu->errcode == ERR_HTTP_REDIR)
        {
          urlr->status |= URL_PROCESSED;
          if ((urlr->moved_to->status & URL_PROCESSED) && (!urlr->moved_to->moved_to))
          {
            SETNEXTURL;
          }
          else
          {
#ifdef I_FACE
            if (cfg.xi_face)
              gui_tree_set_icon_for_doc(docu);
#endif
            urlr = urlr->moved_to;
          }
        }

        if (docu->errcode == ERR_HTTP_TRUNC)
        {
          urlr->status |= URL_TRUNCATED;
          _free(docu->etag);

          docu->etag = get_mime_param_val_str("ETag:", docu->mime);
          if (!docu->etag)
            docu->etag = get_mime_param_val_str("Content-Location:", docu->mime);
          if (!docu->etag)
            docu->etag = get_mime_param_val_str("Last-Modified", docu->mime);
        }

        if (docu->errcode == ERR_HTTP_AUTH)
        {
          docu->doc_url->status |= URL_PROCESSED;
          docu->doc_url->status |= URL_ERR_REC;
          SETNEXTURL;
        }

        if (docu->errcode == ERR_HTTP_PROXY_AUTH)
        {
          docu->doc_url->status |= URL_PROCESSED;
          docu->doc_url->status |= URL_ERR_REC;
          SETNEXTURL;
        }

        _free(docu->mime);
        _free(docu->type_str);

        nreget += (docu->errcode == ERR_HTTP_TRUNC ||
                   docu->errcode == ERR_FTP_TRUNC) && cfg.mode != MODE_SREGET;
        nredir += (docu->errcode == ERR_HTTP_REDIR);
        _free(pstr);
        continue;
      }

      if (docu->errcode == ERR_FTP_UNKNOWN
          || docu->errcode == ERR_FTP_CONNECT
          || docu->errcode == ERR_FTP_DATACON
          || docu->errcode == ERR_FTPS_CONNECT
          || docu->errcode == ERR_FTPS_DATASSLCONNECT
          || docu->errcode == ERR_HTTP_UNKNOWN
          || docu->errcode == ERR_HTTP_CONNECT
          || docu->errcode == ERR_HTTP_SNDREQ
          || docu->errcode == ERR_HTTP_SNDREQDATA
          || docu->errcode == ERR_HTTP_RCVRESP
          || docu->errcode == ERR_HTTP_SERV
          || docu->errcode == ERR_HTTP_TIMEOUT
          || docu->errcode == ERR_HTTP_PROXY_CONN
          || docu->errcode == ERR_HTTPS_CONNECT
          || docu->errcode == ERR_READ
          || docu->errcode == ERR_ZERO_SIZE
          || docu->errcode == ERR_GOPHER_CONNECT
          || docu->errcode == ERR_PROXY_CONNECT
          || docu->errcode == ERR_HTTP_SERV)
      {
        urlr->status |= URL_ERR_REC;
        pokus++;
        /*** retry only when allowed ***/
        if (pokus >= cfg.nretry)
        {
          urlr->status |= URL_PROCESSED;
          SETNEXTURL;
        }
        _free(pstr);
        _free(docu->mime);
        _free(docu->type_str);
        continue;
      }
      else if (docu->errcode == ERR_LOCKED)
      {
        LOCK_CFG_URLSTACK;
        if (!cfg.urlstack)
        {
          UNLOCK_CFG_URLSTACK;
          xprintf(1, gettext("last document locked -> sleeping for 5 seconds\n"));
          tl_sleep(5);
        }
        else
        {
          UNLOCK_CFG_URLSTACK;
        }
        reschedule_url(urlr);
        SETNEXTURL;
      }
      else if (docu->errcode == ERR_BIGGER
               || docu->errcode == ERR_SMALLER
               || docu->errcode == ERR_NOMIMET
               || docu->errcode == ERR_OUTTIME || docu->errcode == ERR_SCRIPT_DISABLED)
      {
        urlr->status |= URL_PROCESSED;
        urlr->status |= URL_ERR_REC;
        SETNEXTURL;
      }
      else
      {
        /*** remove improper documents if required ***/
        if ((cfg.remove_old
             && (cfg.mode == MODE_SYNC
                 || cfg.mode == MODE_MIRROR))
            && (((docu->errcode == ERR_FTP_GET
                  || docu->errcode == ERR_FTP_BDIR
                  || docu->errcode == ERR_FTP_NODIR)
                 && docu->ftp_respc == 550)
                || docu->errcode == ERR_HTTP_NFOUND
                || docu->errcode == ERR_HTTP_GONE))
        {
          doc_remove(docu->doc_url);
        }

        urlr->status |= URL_ERR_UNREC;
        urlr->status |= URL_PROCESSED;

        SETNEXTURL;
      }
    }
    else
    {
#ifdef HAVE_DEBUG_FEATURES
      char dumpbuf[2048];

      DEBUG_PROTOD("******************** Response: Content OK *************\n");
      DEBUG_PROTOD("%s\n", tl_asciidump_content(dumpbuf, NUM_ELEM(dumpbuf), docu->contents, docu->size));
      DEBUG_PROTOD("***************************************************\n");
#endif
    }

    _Xt_Serve;

    if (urlr->status & URL_TRUNCATED)
      urlr->status &= ~URL_TRUNCATED;

    if (urlr->status & URL_ERR_REC)
      urlr->status &= ~URL_ERR_REC;

    if (cfg.show_time)
    {
      atm = time(NULL);
      LOCK_TIME;
      strftime(cpom, sizeof(cpom), "%H:%M:%S", localtime(&atm));
      UNLOCK_TIME;
      xprintf(1, gettext("Ending time :    %s\n"), cpom);
    }

    report_error(docu, gettext("download"));

    _Xt_Serve;

    if (docu->contents)
    {
      if (docu->is_parsable)
      {
        dllist *formlist = NULL;
        dllist *urls;

        gui_set_status(gettext("Relocating and scanning HTML document"));

        DEBUG_TRACE(
          "process_document: html_process_document: docp->contents:\n"
          "-----------------------------------------------------\n"
          "%s\n"
          "-----------------------------------------------------\n",
          docu->contents);
        urls = html_process_document(docu, priv_cfg.formdata ? &formlist : NULL);

        _Xt_Serve;

        if (urls && bufio_is_open(cfg.dump_urlfd))
        {
          url *tmp_urlr;
          char *tmp_pstr = NULL;
          tmp_urlr = docu->doc_url;
          tmp_pstr = url_to_urlstr(tmp_urlr, FALSE);
          DEBUG_TRACE("url list dump for URL '%s'\n", tmp_pstr);

          dump_urls_list(urls, docu);
          _free(tmp_pstr);
        }
        if (priv_cfg.formdata && bufio_is_open(cfg.dump_fd))
        {
          url *tmp_urlr;
          char *tmp_pstr = NULL;
          tmp_urlr = docu->doc_url;
          tmp_pstr = url_to_urlstr(tmp_urlr, FALSE);
          DEBUG_TRACE("formdata dump for URL '%s'\n", tmp_pstr);

          dump_cfg_formdata(tmp_pstr, priv_cfg.formdata);
          _free(tmp_pstr);
        }
        if (formlist && bufio_is_open(cfg.dump_fd))
        {
          url *tmp_urlr;
          char *tmp_pstr = NULL;
          tmp_urlr = docu->doc_url;
          tmp_pstr = url_to_urlstr(tmp_urlr, FALSE);
          DEBUG_TRACE("formlist dump for URL '%s'\n", tmp_pstr);

          dump_formlist(tmp_pstr, formlist);
          _free(tmp_pstr);
        }

        if ((priv_cfg.formdata || cfg.try_all_forms) && formlist)
        {
          add_matching_forms(docu, formlist);
          while (formlist)
          {
            if (formlist->data)
              _free(formlist->data);
            formlist = dllist_remove_entry(formlist, formlist);
          }
        }

        if (cfg.mode != MODE_SREGET && cfg.mode != MODE_FTPDIR &&
            !(docu->doc_url->status & URL_NORECURSE))
        {
          gui_tree_add_start();
          cat_links_to_url_list(urls);
          gui_tree_add_end();
        }
        else if (cfg.mode == MODE_FTPDIR)
        {
          dump_ftp_list(urls);
        }
        else
        {
          while (urls)
          {
            free_deep_url((url *)urls->data);
            _free(urls->data);
            urls = dllist_remove_entry(urls, urls);
          }
        }

        _Xt_Serve;
      }

      store_stat = 0;

      if (bufio_is_open(cfg.dump_fd) && cfg.dump_after)
      {
        bufio *fd;

        gui_set_status(gettext("Dumping processed document"));
        VERIFY(!bufio_lock(cfg.dump_fd, FALSE, &fd));
        if (docu->request_header && cfg.dump_req)
        {
          bufio_asciidump(fd, docu->request_header, strlen(docu->request_header), DUMP_SECTION_SEPSTR);
        }
        if (docu->request_data && cfg.dump_req)
        {
          bufio_asciidump(fd, docu->request_data, docu->request_datalen, NULL);
        }
        if (docu->mime && cfg.dump_resp)
        {
          bufio_puts(fd, DUMP_SECTION_SEPSTR);
          bufio_puts(fd, docu->mime);
        }
        if (docu->contents && cfg.dump_resp)
        {
          bufio_asciidump(fd, docu->contents, docu->size, NULL);
        }
        VERIFY(!bufio_release(cfg.dump_fd, &fd));
      }
      /* else */ if ((docu->doc_url->type != URLT_FILE)
                     && !(docu->doc_url->status & URL_REDIRECT)
                     && (docu->errcode != ERR_HTTP_ACTUAL)
                     && (docu->errcode != ERR_FTP_ACTUAL) && (cfg.mode != MODE_NOSTORE)
                     && /* (cfg.dump_fd->fd < 0) && */ (cfg.mode != MODE_FTPDIR))
      {
        gui_set_status(gettext("Storing document"));

        store_stat = doc_store(docu, TRUE);
        if (store_stat)
        {
          xprintf(1, gettext("Store failed\n"));
          urlr->status &= ~URL_ERR_REC;
        }
      }

      _Xt_Serve;

      if (priv_cfg.post_cmd)
        run_post_command(docu);

      doc_remove_lock(docu);

      doc_update_parent_links(docu);
    }
    else
    {
      if (priv_cfg.post_cmd)
        run_post_command(docu);

      doc_remove_lock(docu);

      doc_update_parent_links(docu);
    }

    urlr->status |= URL_DOWNLOADED;
    urlr->status |= URL_PROCESSED;
    SETNEXTURL;
  }
  return ERR_UNKNOWN;
}

#ifdef I_FACE
int download_single_doc(url *urlp)
{
  int rv;
  doc docu;
  global_connection_info con_info;

#if defined (HAVE_MT) && defined (I_FACE)
  _config_struct_priv_t privcfg;

#if defined (__OSF__) || defined (__osf__)
#define __builtin_try
#define __builtin_finally
#endif

  privcfg_make_copy(&privcfg);
  pthread_setspecific(cfg.privcfg_key, (void *)(&privcfg));
  pthread_cleanup_push((void *)privcfg_free, (void *)(&privcfg));
#endif

  gui_start_download(FALSE);

#ifdef HAVE_MT
  {
#if defined (HAVE_SIGEMPTYSET) && defined (HAVE_SIGADDSET) && defined (HAVE_PTHREAD_SIGMASK)
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
  pthread_setspecific(cfg.thrnr_key, (void *)0);
  DEBUG_MTTHR("starting thread(%ld) %d\n", pthread_t2long(pthread_self()), 0);

  cfg.allthreadsnr = 0;
  gui_mt_thread_start(cfg.allthreadsnr);
#endif

  cfg.rbreak = FALSE;
  cfg.stop = FALSE;
  cfg.processing = TRUE;

  doc_init(&docu, urlp);
#ifdef HAVE_MT
  docu.threadnr = 0;
  pthread_setspecific(cfg.currdoc_key, (void *)NULL);
  pthread_setspecific(cfg.herrno_key, (void *)(&(docu.__herrno)));
#endif
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  docu.cyclenr = 0;
#endif
  rv = process_document(&docu, FALSE);

  init_global_connection_data(&con_info);
  save_global_connection_data(&con_info, &docu);
  kill_global_connection_data(&con_info);

  cfg.processing = FALSE;

  cfg.rbreak = FALSE;
  cfg.stop = FALSE;

#if defined (HAVE_MT) && defined (I_FACE)
  pthread_cleanup_pop(TRUE);
#endif
#ifdef HAVE_MT
  doc_finish_processing(&docu);
  cfg.allthreadsnr = 0;
  gui_mt_thread_end(0);
#endif
  gui_beep();
  gui_set_msg(gettext("Done"), 0);

  return rv;
}
#endif


/*********************************************/
/* rekurzivne prechadzanie stromu dokumentov */
/* FIXME: Translate me!                      */
/*********************************************/
#ifdef HAVE_MT
static void _recurse(int thnr)
#else
void recurse(int thnr)
#endif
{
  bool_t rbreaksave, stopsave;

  global_connection_info con_info;

  if (cfg.urlstack == NULL)
    return;

  init_global_connection_data(&con_info);

/**** obsluzenie vsetkych URL v zozname ****/
/**** FIXME: Translate me!              ****/
  while (cfg.urlstack && !cfg.stop)
  {
    doc docu;
    url *urlp;

    LOCK_CFG_URLSTACK;
    /*
     * WARNING! this condition may look rediculous, given the same condition in the
     * while() above, but we're working with multithreaded code here: a different thread
     * executing this same code may nullify 'cfg.urlstack' during the time it took
     * _this_ thread to arrive here. (And believe me, that happens quite often!)
     *
     * So we check again to make sure inside this 'critical section' which is blocked off
     * by the [UN]LOCK_CFG_URLSTACK calls.
     */
    if (cfg.urlstack)
    {
      urlp = (url *)cfg.urlstack->data;
      cfg.urlstack = dllist_remove_entry(cfg.urlstack, cfg.urlstack);
#ifdef HAVE_MT
      mt_semaphore_decrement(&cfg.urlstack_sem);
#endif
      UNLOCK_CFG_URLSTACK;
    }
    else
    {
      UNLOCK_CFG_URLSTACK;
      continue;
    }

    doc_init(&docu, urlp);

#ifdef HAVE_MT
    docu.threadnr = thnr;
    pthread_setspecific(cfg.currdoc_key, (void *)(&docu));
    pthread_setspecific(cfg.herrno_key, (void *)(&(docu.__herrno)));
#endif
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
    docu.cyclenr = 0;
#endif

    LOCK_DCNT;
    cfg.docnr++;
    docu.doc_nr = cfg.docnr;
    UNLOCK_DCNT;

    restore_global_connection_data(&con_info, &docu);

    process_document(&docu, TRUE);

    save_global_connection_data(&con_info, &docu);

#ifdef HAVE_MT
    doc_finish_processing(&docu);
#endif

    if (docu.errcode == ERR_QUOTA_FS
        || docu.errcode == ERR_QUOTA_TRANS
        || docu.errcode == ERR_QUOTA_TIME || cfg.rbreak)
    {
      LOCK_CFG_URLSTACK;
      cfg.docnr--;
      cfg.urlstack = dllist_prepend(cfg.urlstack, (dllist_t)urlp);
#ifdef HAVE_MT
      mt_semaphore_up(&cfg.urlstack_sem);
#endif
      UNLOCK_CFG_URLSTACK;
      break;
    }
  }
#if defined (I_FACE) && !defined (HAVE_MT)
  if (cfg.xi_face)
  {
    gui_set_status(gettext("Done"));
  }
#endif

#ifdef I_FACE
  if (cfg.xi_face)
  {
    gui_set_doccounter();
  }
#endif

  LOCK_CFG_URLSTACK;            /* use this lock for the next sector; could've used another lock too, no matter */
  stopsave = cfg.stop;
  rbreaksave = cfg.rbreak;
  cfg.stop = FALSE;
  cfg.rbreak = FALSE;

  kill_global_connection_data(&con_info);

  DEBUG_TRACE("kill global connection!\n");

  if (cfg.update_cookies)
  {
    cookie_update_file(TRUE);
  }

  cfg.stop = stopsave;
  cfg.rbreak = rbreaksave;
  UNLOCK_CFG_URLSTACK;

  if (!cfg.rbreak && !cfg.stop && cfg.stats_file)
  {
    stats_fill_spage(cfg.stats_file, NULL);
  }
}

#ifdef HAVE_MT
static void _recurse_thrd(int thrnr)
{
/*  bool_t init = (thrnr == 0); */
#ifdef I_FACE
  _config_struct_priv_t privcfg;
#endif

  {
#if defined (HAVE_SIGEMPTYSET) && defined (HAVE_SIGADDSET) && defined (HAVE_PTHREAD_SIGMASK)
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
  pthread_setspecific(cfg.currdoc_key, (void *)NULL);
  pthread_setspecific(cfg.thrnr_key, (void *)thrnr);
  DEBUG_MTTHR("starting thread(%ld) %d\n", pthread_t2long(pthread_self()), thrnr);

#ifdef I_FACE
  privcfg_make_copy(&privcfg);
  pthread_setspecific(cfg.privcfg_key, (void *)(&privcfg));
  pthread_cleanup_push((void *)privcfg_free, (void *)(&privcfg));
#endif

  for ( ; !cfg.rbreak && !cfg.stop;)
  {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    DEBUG_MTTHR("thread %d awaking\n", thrnr);

    _recurse(thrnr);
    /* init = FALSE; */
    gui_clear_status();
    DEBUG_MTTHR("thread %d sleeping\n", thrnr);
    gui_set_status(gettext("Sleeping ..."));
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    DEBUG_TRACE("mt_semaphore_up(cfg.nrunning_sem)\n");
    mt_semaphore_up(&cfg.nrunning_sem);
    /*
     * UN-critical section; also signal that this thread isn't really active now.
     *
     * The effect of this is that the main thread can monitor if anybody is still
     * working on some URL; if all threads are waiting in the next loop ffor another
     * URL to arrive, the main thread will discover this after a while and will then
     * be able to signal all threads to stop as everything is done.
     */

    while (!cfg.stop && !cfg.rbreak && mt_semaphore_timed_wait(&cfg.urlstack_sem, 400) < 0)
    {
      DEBUG_TRACE("[%d/%d] wait for cfg.urlstack_sem: v = %ld\n",
                  thrnr,
                  cfg.nthr,
                  cfg.urlstack_sem.v);
    }

    DEBUG_TRACE("mt_semaphore_dec(cfg.nrunning_sem)\n");
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

void recurse(int dumb)
{
  pthread_attr_t thrdattr;
  int i;
  int num = cfg.nthr;

  {
#if defined (HAVE_SIGEMPTYSET) && defined (HAVE_SIGADDSET) && defined (HAVE_PTHREAD_SIGMASK)
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

  DEBUG_TRACE("mt_semaphore_init(nrunning/urlstack)\n");
  mt_semaphore_destroy(&cfg.nrunning_sem);
  mt_semaphore_destroy(&cfg.urlstack_sem);
  mt_semaphore_initv(&cfg.nrunning_sem, 1);
  mt_semaphore_initv(&cfg.urlstack_sem, 0);
  /* signal each URL available in the queue */
  {
    dllist *dptr;

    LOCK_CFG_URLSTACK;
    for (dptr = cfg.urlstack; dptr; dptr = dptr->next)
    {
      mt_semaphore_up(&cfg.urlstack_sem);
    }
    UNLOCK_CFG_URLSTACK;
  }

  if (num <= 0)
    num = 1;

  cfg.allthreadsnr = 0;
  cfg.allthreads = _malloc(num * sizeof(cfg.allthreads[0]));

  /* mt_semaphore_decrement(&cfg.urlstack_sem); */

  for (i = 0; i < num; i++)
  {
    if (!pthread_create(&(cfg.allthreads[cfg.allthreadsnr]), &thrdattr,
                        (void *(*)(void *))_recurse_thrd, (void *)cfg.allthreadsnr))
    {
      cfg.allthreadsnr++;
      gui_mt_thread_start(cfg.allthreadsnr);
      DEBUG_TRACE("mt_semaphore_dec(cfg.nrunning_sem)\n");
      mt_semaphore_decrement(&cfg.nrunning_sem);
    }
    else
    {
      xprintf(1, "Create downloading thread %d\n", i);
    }
    if (cfg.rbreak || cfg.stop)
      break;
  }

  DEBUG_TRACE("mt_semaphore_timed_wait(cfg.nrunning_sem)\n");
  while (!cfg.stop && !cfg.rbreak && mt_semaphore_timed_wait(&cfg.nrunning_sem, 500) < 0)
  {
    DEBUG_TRACE("wait for cfg.nrunning_sem: v = %ld\n", cfg.nrunning_sem.v);
  }

  DEBUG_DEVEL("cfg.stop = TRUE @ %s, line #%d\n", __FILE__, __LINE__);
  /*
   * signal all processing threads to stop as each one of them is only waiting
   * for another URL to arrive in the URL stack/queue right now - which will never
   * happen as all URLs have been processed.
   */
  cfg.stop = TRUE;

  tl_msleep(300, TRUE);

  for (i = 0; i < cfg.allthreadsnr; i++)
  {
#if 0
    pthread_cancel(cfg.allthreads[i]);
    pthread_kill(cfg.allthreads[i], SIGQUIT);
#endif
    pthread_join(cfg.allthreads[i], NULL);
  }

  DEBUG_TRACE("mt_semaphore_destroy(cfg.nrunning_sem)\n");
  mt_semaphore_destroy(&cfg.nrunning_sem);
  _free(cfg.allthreads);
  cfg.allthreadsnr = 0;
  gui_mt_thread_end(0);
}
#endif

void dump_ftp_list(dllist *urllst)
{
  dllist *ptr = urllst;

  while (ptr)
  {
    url *urlp = (url *)ptr->data;
    void *dupl;

    dupl = dllist_find2(ptr->next, (dllist_t)urlp, dllist_url_compare);

    if (!dupl && !(urlp->status & URL_INLINE_OBJ) &&
        (urlp->type == URLT_FTP || urlp->type == URLT_FTPS))
    {
      char *p, *pp;

      p = url_get_path(urlp);
      pp = strrchr(p, '/');
      if (pp)
      {
        pp++;
        if (! * pp)
        {
          pp -= 2;
          while (pp > p && *pp != '/')
            pp--;
          pp++;
        }
        if (urlp->extension)
        {
          ftp_url_extension *fe = urlp->extension;

          if (fe->type == FTP_TYPE_F)
            xprintf(1, gettext("\t%s    (%d bytes)\n"), pp, fe->size);
          else if (fe->type == FTP_TYPE_L)
            xprintf(1, "\t%s    -> %s\n", pp, fe->slink);
          else if (fe->type == FTP_TYPE_D)
            xprintf(1, "\t%s/\n", pp, fe->slink);
        }
        else
          xprintf(1, "\t%s\n", pp);
      }
    }
    free_deep_url(urlp);
    _free(urlp);
    ptr = dllist_remove_entry(ptr, ptr);
  }
}





void cvt_url2diag_str(url *urlp, char **urldbgstr_ref, char **urlstr_ref)
{
  char *p;

#ifdef WITH_TREE
#ifdef I_FACE
  url_prop *prp;
#endif
#endif
  char pom[4096];
  char *urlstr = NULL;
  char *urldbgstr = NULL;
  int status;

  if (urldbgstr_ref)
    *urldbgstr_ref = NULL;
  if (urlstr_ref)
    *urlstr_ref = NULL;
  if (urldbgstr_ref || urlstr_ref)
  {
#ifdef WITH_TREE
#ifdef I_FACE
    prp = urlp->prop;
#endif
#endif

    p = url_to_urlstr(urlp, FALSE);
    snprintf(pom, sizeof(pom), "URL:\"%s\"", p);
    pom[sizeof(pom) - 1] = 0;
    _free(p);
    urlstr = tl_str_concat(DBGvars(urlstr), pom, NULL);
    urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", pom, "\n", NULL);

    if (urlp->type == URLT_HTTP || urlp->type == URLT_HTTPS)
    {
      urlstr = tl_str_concat(DBGvars(urlstr), " METHOD:", NULL);
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "Request type: ", NULL);
      if (!(urlp->status & URL_FORM_ACTION))
      {
        urlstr = tl_str_concat(DBGvars(urlstr), "GET", NULL);
        urldbgstr = tl_str_concat(DBGvars(urldbgstr), "GET", "\n", NULL);
      }
      else
      {
        form_info *fi = (form_info *)urlp->extension;
        dllist *ptr;

        if (fi->method == FORM_M_GET)
        {
          urlstr = tl_str_concat(DBGvars(urlstr), "GET", NULL);
          urldbgstr = tl_str_concat(DBGvars(urldbgstr), "GET", "\n", NULL);
        }
        else
        {
          urlstr = tl_str_concat(DBGvars(urlstr), "POST", NULL);
          urldbgstr = tl_str_concat(DBGvars(urldbgstr), "POST", "\n", NULL);

          urlstr = tl_str_concat(DBGvars(urlstr), " ENCODING:",
                                 ((fi->encoding == FORM_E_MULTIPART) ? "m" : "u"), NULL);
          urldbgstr =
            tl_str_concat(DBGvars(urldbgstr), "-- ", "Request encoding: ",
            ((fi->encoding == FORM_E_MULTIPART) ? "form/multipart" : "application/x-www-urlencoded"), "\n", NULL);
        }
        ptr = fi->infos;
        if (ptr)
        {
          urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "Query values:\n", NULL);
        }
        while (ptr)
        {
          form_field *ff = (form_field *)ptr->data;
          char *name, *value;
          const char *typestr;

          name = form_encode_urlencoded_str(ff->name);
          value = form_encode_urlencoded_str(ff->value);

          urlstr = tl_str_concat(DBGvars(urlstr), " ",
            ((ff->type == FORM_T_FILE) ? "FILE:\"" : "FIELD:\""), name, "=", value, "\"", NULL);
          switch (ff->type)
          {
          case FORM_T_TEXT:
            typestr = "TEXT";
            break;

          case FORM_T_PASSWORD:
            typestr = "PASSWORD";
            break;

          case FORM_T_CHECKBOX:
            typestr = "CHECKBOX";
            break;

          case FORM_T_RADIO:
            typestr = "RADIO";
            break;

          case FORM_T_SUBMIT:
            typestr = "SUBMIT";
            break;

          case FORM_T_RESET:
            typestr = "RESET";
            break;

          case FORM_T_FILE:
            typestr = "FILE";
            break;

          case FORM_T_HIDDEN:
            typestr = "HIDDEN";
            break;

          case FORM_T_IMAGE:
            typestr = "IMAGE";
            break;

          case FORM_T_BUTTON:
            typestr = "BUTTON";
            break;

          case FORM_T_SELECT:
            typestr = "SELECT";
            break;

          case FORM_T_OPTION:
            typestr = "OPTION";
            break;

          case FORM_T_OPTGROUP:
            typestr = "OPTGROUP";
            break;

          case FORM_T_TEXTAREA:
            typestr = "TEXTAREA";
            break;

          case FORM_T_NONFORM:
            typestr = "NONFORM";
            break;

          case FORM_T_UNKNOWN:
          default:
            typestr = "UNKNOWN";
          }
          urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", typestr, ":\"", name, "=", value, "\"n", NULL);

          _free(name);
          _free(value);

          ptr = ptr->next;
        }
      }
#if 0
      if (urlp->cookies)         /* [i_a] */
      {
        cookie_entry *centry;

        /* FIXED: show url->cookies: cookie_entry * */
        urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "Cookies:\n", NULL);

        for (centry = urlp->cookies; centry; centry = centry->next)
        {
          char *s = cvt_cookie_to_setstr(centry);
          ASSERT(centry->domain);
          ASSERT(centry->path);
          urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "Cookie = ", s, NULL);
        }
      }
#endif
    }

    urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "Status: ", NULL);

    status = urlp->status;
    if (!(status & URL_PROCESSED))
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "not processed yet\n", NULL);
    }
    else if (status & URL_PROCESSED)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "already processed\n", NULL);
    }
    status &= ~URL_PROCESSED;
    if (status & URL_INNSCACHE)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "loaded from NS cache\n", NULL);
    }
    status &= ~URL_INNSCACHE;
    if (status & URL_REDIRECT)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "loaded from local URL tree\n", NULL);
    }
    status &= ~URL_REDIRECT;
    if (status & URL_MOVED)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "moved to another URL\n", NULL);
    }
    status &= ~URL_MOVED;
    if (status & URL_NOT_FOUND)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "URL not found on remote server\n", NULL);
    }
    status &= ~URL_NOT_FOUND;
    if (status & URL_TRUNCATED)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "truncated\n", NULL);
    }
    status &= ~URL_TRUNCATED;
    if (status & URL_DOWNLOADED)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "downloaded OK\n", NULL);
    }
    status &= ~URL_DOWNLOADED;
    if (status & URL_REJECTED)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "rejected by rules\n", NULL);
    }
    status &= ~URL_REJECTED;
    if (status & URL_USER_DISABLED)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "disabled by user\n", NULL);
    }
    status &= ~URL_USER_DISABLED;
    if (status & URL_ERR_REC)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "probably recoverable error\n", NULL);
    }
    status &= ~URL_ERR_REC;
    if (status & URL_ERR_UNREC)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "unrecoverable error\n", NULL);
    }
    status &= ~URL_ERR_UNREC;
    if (status & URL_INLINE_OBJ)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "inline object\n", NULL);
    }
    status &= ~URL_INLINE_OBJ;
    if (status & URL_STYLE)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "style sheet\n", NULL);
    }
    status &= ~URL_STYLE;
    if (status & URL_ISHTML)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "is a HTML page\n", NULL);
    }
    status &= ~URL_ISHTML;
    if (status & URL_ISLOCAL)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "local\n", NULL);
    }
    status &= ~URL_ISLOCAL;
    if (status & URL_NORECURSE)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "do not recurse this URL\n", NULL);
    }
    status &= ~URL_NORECURSE;
    if (status & URL_FORM_ACTION)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "is a FORM\n", NULL);
    }
    status &= ~URL_FORM_ACTION;
    if (status & URL_HAVE_FORMS)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "contains forms\n", NULL);
    }
    status &= ~URL_HAVE_FORMS;
    if (status & URL_ISFIRST)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "1st entry: The First\n", NULL);
    }
    status &= ~URL_ISFIRST;
    if (status & URL_ISSTARTING)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "is a starting point\n", NULL);
    }
    status &= ~URL_ISSTARTING;
    if (status & URL_ISSCRIPT)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "is JavaScript\n", NULL);
    }
    status &= ~URL_ISSCRIPT;
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
    if (status & URL_ISHAMMER)
    {
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", "will be hammered\n", NULL);
    }
    status &= ~URL_ISHAMMER;
#endif
    if (status)
    {
      snprintf(pom, sizeof(pom), "unknown. (bits: 0x%08X)\n", status);
      pom[sizeof(pom) - 1] = 0;
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "    ", pom, NULL);
    }

    if (urlp->moved_to)
    {
      char *ps = url_to_urlstr(urlp->moved_to, FALSE);

      snprintf(pom, sizeof(pom), gettext("Moved to URL: %s\n"), ps);
      pom[sizeof(pom) - 1] = 0;
      _free(ps);
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", pom, NULL);
    }

#ifdef WITH_TREE
#ifdef I_FACE
    if (prp)
    {
      if (prp->type)
        snprintf(pom, sizeof(pom), gettext("Type: %s\n"), gettext(prp->type));
      else
        snprintf(pom, sizeof(pom), gettext("Type: unknown\n"));
      pom[sizeof(pom) - 1] = 0;
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", pom, NULL);

      sprintf(pom, gettext("Size: %d\n"), (int)prp->size);
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", pom, NULL);

      if (prp->mdtm)
      {
        LOCK_TIME;
        strftime(pom, sizeof(pom), gettext("Modification time: %a, %d %b %Y %H:%M:%S %Z\n"), localtime(&prp->mdtm));
        pom[sizeof(pom) - 1] = 0;
        UNLOCK_TIME;
        urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", pom, NULL);
      }
    }
#endif
#endif

    if (urlp->local_name)
    {
      snprintf(pom, sizeof(pom), gettext("Local filename: %s\n"), urlp->local_name);
      pom[sizeof(pom) - 1] = 0;
      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", pom, NULL);
    }

    if (urlp->ref_cnt)
    {
      dllist *ptr;
      char *us;

      urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", "Parent URLs:\n", NULL);
      LOCK_URL(urlp);
      for (ptr = urlp->parent_url; ptr; ptr = ptr->next)
      {
        url *pomurl = (url *)ptr->data;

        us = url_to_urlstr(pomurl, FALSE);
        snprintf(pom, sizeof(pom), "      %s\n", us);
        pom[sizeof(pom) - 1] = 0;
        _free(us);
        urldbgstr = tl_str_concat(DBGvars(urldbgstr), "-- ", pom, NULL);
      }
      UNLOCK_URL(urlp);
    }
    if (urldbgstr_ref)
      *urldbgstr_ref = urldbgstr;
    if (urlstr_ref)
      *urlstr_ref = urlstr;
  }
}





char *cvt_url_info2diag_str(url_info *ui)
{
  char *retv = NULL;
  dllist *fptr;
  cookie_entry *cptr;

  if (!ui)
  {
    retv = tl_strdup("(null)");
  }
  else
  {
    char tmp[40];
    char *method_str = "unknown";
    char *enc_str = "unknown";
    if (ui->method == FORM_M_GET)
      method_str = "GET";
    else if (ui->method == FORM_M_POST)
      method_str = "POST";
    if (ui->encoding == FORM_E_URLENCODED)
      enc_str = "application/x-www-form-urlencoded";
    else if (ui->encoding == FORM_E_MULTIPART)
      enc_str = "multipart/form-data";
    sprintf(tmp, "%d", ui->type);
    retv = tl_str_concat(DBGvars(retv), "--formdata:\n",
                         "--          urlstr: '", (ui->urlstr ? ui->urlstr : "(null)"), "'\n",
                         "--          type: '", tmp, "'\n",
                         "--          method: '", method_str, "'\n",
                         "--          encoding: '", enc_str, "'\n",
                         "--          fields:\n", NULL);
    fptr = ui->fields;
    while (fptr)
    {
      char *p3[3];
      form_field *ff = (form_field *)fptr->data;

      switch (ff->type)
      {
      case FORM_T_FILE:
        p3[0] = "FILE";
        break;

      case FORM_T_TEXTAREA:
        p3[0] = "LONG TEXT";
        break;

      default:
        p3[0] = "TEXT";
      }

      p3[1] = form_encode_urlencoded_str(ff->name);
      p3[2] = form_encode_urlencoded_str(ff->value);

      retv = tl_str_concat(DBGvars(retv),
                           "--          - field: type: ", p3[0], ", name: '", p3[1],
                           "', value: '", p3[2], "'\n", NULL);

      _free(p3[1]);
      _free(p3[2]);
      fptr = fptr->next;
    }
    cptr = ui->cookies;
    ASSERT(cptr ? !cptr->prev : TRUE);
    while (cptr)
    {
      char *cmsg;

      cmsg = cvt_cookie_to_setstr(cptr);
      retv = tl_str_concat(DBGvars(retv), "--          - cookie: ", cmsg, "\n", NULL);
      _free(cmsg);

      cptr = cptr->next;
    }
    retv = tl_str_concat(DBGvars(retv),
                         "--          localname: '", (ui->localname ? ui->localname : "(null)"),
                         "'\n", NULL);
  }

  return retv;
}

void dump_url(url *urlp)
{
  if (bufio_is_open(cfg.dump_urlfd) || bufio_is_open(cfg.dump_fd))
  {
    char *urlstr = NULL;
    char *urldbgstr = NULL;

    cvt_url2diag_str(urlp, &urldbgstr, &urlstr);

    if (bufio_is_open(cfg.dump_urlfd))
    {
      bufio_puts_m(cfg.dump_urlfd, urlstr, "\n", NULL);
    }
    if (bufio_is_open(cfg.dump_fd))
    {
      bufio_puts_m(cfg.dump_fd, urldbgstr, "\n", NULL);
    }
    _free(urlstr);
    _free(urldbgstr);
  }
}


void dump_urls_list(dllist *urls, doc *docu)
{
  dllist *ptr;
  void *dupl;

  if (bufio_is_open(cfg.dump_urlfd) || bufio_is_open(cfg.dump_fd))
  {
    if (bufio_is_open(cfg.dump_urlfd))
    {
      bufio_puts(cfg.dump_urlfd, "\n---------------------- URL DUMP ----------------------\n");
    }
    if (bufio_is_open(cfg.dump_fd))
    {
      bufio_puts(cfg.dump_fd, "\n---------------------- URL DUMP ----------------------\n");
    }

    dump_url(docu->doc_url);
    if (bufio_is_open(cfg.dump_urlfd))
    {
      bufio_puts(cfg.dump_urlfd, "  -->\n");
    }
    if (bufio_is_open(cfg.dump_fd))
    {
      bufio_puts(cfg.dump_fd, "  -->\n");
    }

    for (ptr = urls; ptr; ptr = ptr->next)
    {
      dupl = dllist_find2(ptr->next, (dllist_t)ptr->data, dllist_url_compare);
      if (!dupl)
      {
        dump_url((url *)ptr->data);
      }
    }
  }
}

void dump_cfg_formdata(const char *urlstr, dllist *formdata)
{
  dllist *uptr;
  dllist *fptr;
  cookie_entry *cptr;

  if (bufio_is_open(cfg.dump_fd))
  {
    bufio *fd;

#ifdef HAVE_MT
    ASSERT(cfg.dump_fd->mutex);
#endif
    VERIFY(!bufio_lock(cfg.dump_fd, FALSE, &fd));
    for (uptr = formdata; uptr; uptr = uptr->next)
    {
      url_info *ui = (url_info *)uptr->data;
      if (ui)
      {
        char *method_str = "unknown";
        char *enc_str = "unknown";
        if (ui->method == FORM_M_GET)
          method_str = "GET";
        else if (ui->method == FORM_M_POST)
          method_str = "POST";
        if (ui->encoding == FORM_E_URLENCODED)
          enc_str = "application/x-www-form-urlencoded";
        else if (ui->encoding == FORM_E_MULTIPART)
          enc_str = "multipart/form-data";
        bufio_printf(fd, "--formdata: URL: '%s'\n", urlstr);
        bufio_printf(fd, "--          urlstr: '%s'\n", (ui->urlstr ? ui->urlstr : "(null)"));
        bufio_printf(fd, "--          type: '%d'\n", ui->type);
        bufio_printf(fd, "--          method: '%s'\n", method_str);
        bufio_printf(fd, "--          encoding: '%s'\n", enc_str);
        bufio_printf(fd, "--          fields:\n");
        fptr = ui->fields;
        while (fptr)
        {
          char *p3[3];
          form_field *ff = (form_field *)fptr->data;

          switch (ff->type)
          {
          case FORM_T_FILE:
            p3[0] = "FILE";
            break;

          case FORM_T_TEXTAREA:
            p3[0] = "LONG TEXT";
            break;

          default:
            p3[0] = "TEXT";
          }

          p3[1] = form_encode_urlencoded_str(ff->name);
          p3[2] = form_encode_urlencoded_str(ff->value);

          bufio_printf(fd,
                       "--          - field: type: %s, name: '%s', value: '%s'\n",
                       p3[0],
                       p3[1],
                       p3[2]);

          _free(p3[1]);
          _free(p3[2]);
          fptr = fptr->next;
        }
        cptr = ui->cookies;
        ASSERT(cptr ? !cptr->prev : TRUE);
        while (cptr)
        {
          char *cmsg;

          cmsg = cvt_cookie_to_setstr(cptr);
          bufio_printf(fd, "--          - cookie: %s\n", cmsg);
          _free(cmsg);

          cptr = cptr->next;
        }
        bufio_printf(fd, "--          localname: '%s'\n",
                     (ui->localname ? ui->localname : "(null)"));
      }
    }
    VERIFY(!bufio_release(cfg.dump_fd, &fd));
  }
}

void dump_formlist(const char *urlstr, dllist *formlist)
{
  dllist *fptr;
  int nform;

  if (bufio_is_open(cfg.dump_fd))
  {
    bufio *fd;

#ifdef HAVE_MT
    ASSERT(cfg.dump_fd->mutex);
#endif
    VERIFY(!bufio_lock(cfg.dump_fd, FALSE, &fd));
    for (fptr = formlist, nform = 0; fptr; fptr = fptr->next, nform++)
    {
      url *urlp;
      char *typ_str = "???";

      bufio_printf(fd, "-- formlist: RAW URL: '%s'\n", (char *)fptr->data);
      DEBUG_TRACE("dump_formlist:url_parse:enter\n");
      urlp = url_parse((char *)fptr->data);
      DEBUG_TRACE("dump_formlist:url_parse:leave\n");
      /* ASSERT(urlp->type != URLT_FROMPARENT); */

      if (urlp)
      {
        switch (urlp->type)
        {
        case URLT_HTTP:
          typ_str = "HTTP";
          break;

        case URLT_HTTPS:
          typ_str = "HTTPS";
          break;

        case URLT_FTP:
          typ_str = "FTP";
          break;

        case URLT_FTPS:
          typ_str = "FTPS";
          break;

        case URLT_FILE:
          typ_str = "FILE";
          break;

        case URLT_GOPHER:
          typ_str = "GOPHER";
          break;

        case URLT_FROMPARENT:
          typ_str = "FROMPARENT";
          break;

        default:
          typ_str = "UNKNOWN";
          break;
        }
      }
      bufio_printf(fd, "--           TYPE: %s\n", typ_str);
      if (urlp)
      {
        if (urlp->parent_url)
        {
          bufio_printf(fd, "--           has parent_utl\n");
        }
        if (urlp->moved_to)
        {
          bufio_printf(fd, "--           has moved to: '%s'\n", urlp->moved_to->local_name);
        }
        bufio_printf(fd, "--           level: %d\n", urlp->level);
        bufio_printf(fd, "--           ref count: %d\n", urlp->ref_cnt);
        bufio_printf(fd, "--           status bits: 0x%08X\n", urlp->status);
        bufio_printf(fd, "--           local name: '%s'\n", urlp->local_name);
        bufio_printf(fd, "--           extension: %p\n", urlp->extension);
      }
      free_deep_url(urlp);
      _free(urlp);
    }
    VERIFY(!bufio_release(cfg.dump_fd, &fd));
  }
}

void get_urls_to_resume(char *dirname)
{
  DIR *dir;
  struct dirent *dent;
  char next_dir[PATH_MAX];
  struct stat estat;
  url *purl;

  if (!(dir = tl_opendir(dirname)))
  {
    xperror1("Cannot open/read directory content", mk_native(dirname));
    return;
  }

  gui_set_msg(gettext("Searching for files to resume"), 0);

  while ((dent = readdir(dir)))
  {
    _Xt_Serve;

    snprintf(next_dir, sizeof(next_dir), "%s%s%s", dirname, (tl_is_dirname(dirname) ? "" : "/"), dent->d_name);
    next_dir[sizeof(next_dir) - 1] = 0;
    if (!strcmp(dent->d_name, "."))
      continue;
    if (!strcmp(dent->d_name, ".."))
      continue;
    if (tl_lstat(next_dir, &estat))
    {
      xperror(mk_native(next_dir));
      continue;
    }

    if (S_ISDIR(estat.st_mode))
    {
      if (!strcmp(dent->d_name, PAVUK_INFO_DIRNAME) && cfg.enable_info)
        continue;

      get_urls_to_resume(next_dir);
    }
    else if (!strncmp(PAVUK_FNAME_PREFIX, dent->d_name, strlen(PAVUK_FNAME_PREFIX)))
    {
      snprintf(next_dir, sizeof(next_dir), "%s%s%s", dirname, (tl_is_dirname(dirname) ? "" : "/"),
               dent->d_name + strlen(PAVUK_FNAME_PREFIX));
      next_dir[sizeof(next_dir) - 1] = 0;
      if ((purl = filename_to_url(next_dir)))
      {
        if (cfg.mode != MODE_MIRROR)
        {
          xprintf(1, gettext("Adding %s to resume list\n"), next_dir);
        }
        purl->status |= URL_ISSTARTING;
        url_set_filename(purl, tl_strdup(next_dir));
        append_url_to_list(purl);
      }
    }

#ifdef I_FACE
    if (cfg.xi_face)
#endif
    {
      if (cfg.rbreak || cfg.stop)
        break;
    }
  }

  closedir(dir);
}

void get_urls_to_synchronize(char *dirname, dllist **list)
{
  DIR *dir;
  struct dirent *dent;
  char next_dir[PATH_MAX];
  struct stat estat;
  url *purl;


  if (!(dir = tl_opendir(dirname)))
  {
    xperror1("Cannot read directory content", mk_native(dirname));
    return;
  }

  gui_set_msg(gettext("Searching for documents to synchronize"), 0);

  while ((dent = readdir(dir)))
  {
    _Xt_Serve;

    snprintf(next_dir, sizeof(next_dir), "%s%s%s", dirname, (tl_is_dirname(dirname) ? "" : "/"), dent->d_name);
    next_dir[sizeof(next_dir) - 1] = 0;
    if (!strcmp(dent->d_name, "."))
      continue;
    if (!strcmp(dent->d_name, ".."))
      continue;
    if (tl_lstat(next_dir, &estat))
    {
      xperror(mk_native(next_dir));
      continue;
    }

    if (S_ISDIR(estat.st_mode))
    {
      if (!strcmp(dent->d_name, PAVUK_INFO_DIRNAME) && cfg.enable_info)
        continue;

      strcat(next_dir, "/");

      if ((purl = filename_to_url(next_dir)) && purl->type == URLT_FTP && !cfg.store_index)
      {
        purl->status |= URL_ISSTARTING;
        purl->extension = ftp_url_ext_new(FTP_TYPE_D, -1, -1, NULL, 0);
        url_set_filename(purl, tl_str_concat(DBGvars(NULL), next_dir, priv_cfg.index_name, NULL));
        *list = dllist_prepend(*list, (dllist_t)purl);
      }
      else if (purl)
      {
        free_deep_url(purl);
        _free(purl);
      }

      next_dir[strlen(next_dir) - 1] = '\0';

      get_urls_to_synchronize(next_dir, list);
    }
    else if (cfg.enable_info && !strcmp(dent->d_name, PAVUK_LOCK_FILENAME))
    {
      /* do nothing */
      continue;
    }
    else if (!strncmp(PAVUK_FNAME_PREFIX, dent->d_name, strlen(PAVUK_FNAME_PREFIX)))
    {
      snprintf(next_dir, sizeof(next_dir), "%s%s%s", dirname, (tl_is_dirname(dirname) ? "" : "/"),
               dent->d_name + strlen(PAVUK_FNAME_PREFIX));
      next_dir[sizeof(next_dir) - 1] = 0;
      if ((purl = filename_to_url(next_dir)))
      {
        char *ustr;

        ustr = url_to_urlstr(purl, FALSE);
        if (cfg.mode != MODE_MIRROR)
        {
          xprintf(1, gettext("Adding file %s to sync list as URL %s\n"), next_dir, ustr);
        }
        _free(ustr);
        if (purl->type == URLT_FTP)
        {
          int tp;

          if (purl->p.ftp.dir)
            tp = FTP_TYPE_D;
          else
            tp = FTP_TYPE_F;
          purl->extension = ftp_url_ext_new(tp, -1, -1, NULL, 0);
        }
        purl->status |= URL_ISSTARTING;

        url_set_filename(purl, tl_strdup(next_dir));
        *list = dllist_prepend(*list, (dllist_t)purl);
      }
    }
    else
    {
      if ((purl = filename_to_url(next_dir)))
      {
        char *ustr;

        ustr = url_to_urlstr(purl, FALSE);
        if (cfg.mode != MODE_MIRROR)
        {
          xprintf(1, gettext("Adding file %s to sync list as URL %s\n"), next_dir, ustr);
        }
        _free(ustr);

        if (purl->type == URLT_FTP)
        {
          int tp;

          if (purl->p.ftp.dir)
            tp = FTP_TYPE_D;
#ifdef S_ISLNK
          else if (S_ISLNK(estat.st_mode))
            tp = FTP_TYPE_L;
#endif
          else
            tp = FTP_TYPE_F;
          purl->extension = ftp_url_ext_new(tp, -1, -1, NULL, 0);
        }

        purl->status |= URL_ISSTARTING;
        url_set_filename(purl, tl_strdup(next_dir));
        *list = dllist_prepend(*list, (dllist_t)purl);
      }
    }

#ifdef I_FACE
    if (cfg.xi_face)
#endif
    {
      if (cfg.rbreak || cfg.stop)
        break;
    }
  }

  closedir(dir);
}

