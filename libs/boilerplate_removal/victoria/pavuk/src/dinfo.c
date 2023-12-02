/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "bufio.h"
#include "dinfo.h"
#include "doc.h"
#include "form.h"
#include "mime.h"
#include "tools.h"
#include "url.h"

static char *dinfo_get_filename_by_filename(char *fname)
{
  char *p;
  char *pom;
  int extra_len = strlen(PAVUK_INFO_DIRNAME) + 3;
  int alloced_len = 0;

  if(priv_cfg.info_dir)
  {
    /*
       pro: if our file does NOT reside inside the cache dir we
       just return a bogus file name;
       The file name could e.g. be inside the current directory
       of the running pavuk process.
       See htmlparser.c for details (search for "'#'")
       Without this fix the program will CRASH in such cases
       (provided that the file names have a certain minimum length).

       The crash is caused by the fact that the routine makes the
       resulting filename start with "info_dir" instead of
       "cache_dir" even if the prefix of the given file name does
       not start with "cache_dir".
       This means that the routine makes the resulting filename
       start with "info_dir" and then takes the string starting
       after offset strlen("cache_dir") of the given filename.
       If strlen("cache_dir") is greater than the length of the
       given filename the system will take an undefined string of
       undefined length out of the heap and the resulting string
       might be much longer than the pre-calculated string len.

       => happy crashing
     */
    if(strncmp(fname, priv_cfg.cache_dir, strlen(priv_cfg.cache_dir)) != 0)
    {
      const char *bogus = "/bogus_dir";
      pom = tl_strdup(bogus);
      DEBUG_HTML("Returning bogus filename for file: '%s'\n", fname);
      return pom;
    }
    alloced_len = strlen(fname) + extra_len + strlen(priv_cfg.info_dir) - strlen(priv_cfg.cache_dir);
    pom = _malloc(alloced_len);
    strcpy(pom, priv_cfg.info_dir);
    p = fname + strlen(priv_cfg.cache_dir);
    if(*p != '/')
      p--;

    if(tl_is_dirname(pom))      /* [i_a] fixup apr/2007  for dual forward dashes inside path */
    {
      pom[strlen(pom) - 1] = 0;
    }
    strcat(pom, p);
    ASSERT(strlen(pom) < alloced_len);
  }
  else
  {
    alloced_len = strlen(fname) + 30;
    pom = _malloc(alloced_len);
    strcpy(pom, fname);
    ASSERT(strlen(pom) < alloced_len);
  }

  p = strrchr(pom, '/');
  if(!p)
    p = pom;
  else
    p++;
  memmove(p + strlen(PAVUK_INFO_DIRNAME) + 1, p, strlen(p) + 1);
  strncpy(p, PAVUK_INFO_DIRNAME "/", strlen(PAVUK_INFO_DIRNAME) + 1);
  ASSERT(strlen(pom) < alloced_len);

  return pom;
}

static char *dinfo_get_filename_by_url(url * urlp)
{
  return dinfo_get_filename_by_filename(url_to_filename(urlp, TRUE));
}

static int dinfo_save_real(char *fnm, url * urlp, char *mime)
{
  bufio *fd;
  char *p;

  if(makealldirs(fnm))
    xperror1("Cannot create directory chain for file", mk_native(fnm));

  fd = bufio_copen(fnm, O_BINARY | O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
  if(!fd)
  {
    xperror(mk_native(fnm));
    return -1;
  }

  bufio_puts(fd, "Original_URL: ");
  p = url_to_urlstr(urlp, FALSE);
  bufio_puts(fd, p);
  _free(p);
  bufio_puts(fd, "\n");

  if(urlp->status & URL_FORM_ACTION)
  {
    form_info *fi;
    dllist *ptr;

    fi = (form_info *) urlp->extension;

    bufio_puts(fd, "XPavuk-FormMethod: ");
    if(fi->method == FORM_M_GET)
      bufio_puts(fd, "GET\n");
    else if(fi->method == FORM_M_POST)
      bufio_puts(fd, "POST\n");
    else
    {
      ASSERT(!"unknown XPavuk-FormMethod");
    }

    bufio_puts(fd, "XPavuk-FormEncoding: ");
    if(fi->encoding == FORM_E_URLENCODED)
      bufio_puts(fd, "application/x-www-form-urlencoded\n");
    else if(fi->encoding == FORM_E_MULTIPART)
      bufio_puts(fd, "multipart/form-data\n");
    else
    {
      DEBUG_HTMLFORM("unknown XPavuk-FormEncoding\n");
      bufio_puts(fd, "unknown\n");
    }
    ptr = fi->infos;
    while(ptr)
    {
      form_field *ff;
      char *n, *v;

      ff = (form_field *) ptr->data;

      n = form_encode_urlencoded_str(ff->name);
      v = form_encode_urlencoded_str(ff->value);

      if(ff->type == FORM_T_FILE)
        bufio_puts(fd, "XPavuk-FormFile: ");
      else
        bufio_puts(fd, "XPavuk-FormField: ");

      bufio_puts(fd, n);
      bufio_puts(fd, "=");
      bufio_puts(fd, v);
      bufio_puts(fd, "\n");

      _free(v);
      _free(n);

      ptr = ptr->next;
    }
  }

  if(mime)
  {
    bufio_puts(fd, mime);
  }

  bufio_close(fd);
  fd = NULL;

  return 0;
}

int dinfo_save(doc * docp)
{
  char *p;
  int rv;

  if(!cfg.enable_info)
    return 0;

  p = dinfo_get_filename_by_url(docp->doc_url);
  rv = dinfo_save_real(p, docp->doc_url, docp->mime);

  _free(p);
  return rv;
}

char *dinfo_load(char *fname)
{
  char *p = NULL;
  char pom[BUFIO_ADVISED_READLN_BUFSIZE];
  int l, tl = 0;
  bufio *fd;

  if(!(fd = bufio_open(fname, O_BINARY | O_RDONLY)))
  {
    /* xperror(mk_native(fname)); */
    return NULL;
  }

  while((l = bufio_readln(fd, pom, sizeof(pom))) > 0)
  {
    p = _realloc(p, tl + l + 1);
    memcpy(p + tl, pom, l);
    tl += l;
  }

  if(p)
    *(p + tl) = '\0';

  bufio_close(fd);
  fd = NULL;

  return p;
}

char *dinfo_get_unique_name(url * urlp, char *pname, int lockfn)
{
  char *dinfos;
  char *p, *us;
  int i;
  char *pom = NULL;
  char *pom2 = NULL;
  char xidx[20];
  char *idir, *ofn, *odir;
  int fd = -1;
  bool_t exist = FALSE;
  int pom2_size;

  pom2 = dinfo_get_filename_by_filename(pname);
  pom2_size = strlen(pom2) + 1;

  p = strrchr(pname, '/');
  if(p)
  {
    ofn = tl_strdup(p + 1);
    odir = tl_strndup(pname, p - pname);
  }
  else
  {
    ofn = tl_strdup("");
    odir = tl_strdup(pname);
  }

  p = strrchr(pom2, '/');
  if(p)
    idir = tl_strndup(pom2, p - pom2);
  else
    idir = tl_strdup(pom2);


  /* !!!!! lock !!!!! */
  if(lockfn)
  {
    p = strrchr(idir, '/');
    ASSERT(p != NULL);
    ASSERT(pom2_size > p + 1 - idir);
    pom2[p + 1 - idir] = '\0';
#ifdef ASSERT_IS_ENABLED
    {
      int ll = strlen(pom2);
      int l2 = strlen(PAVUK_LOCK_FILENAME);
      ASSERT(pom2_size > ll + l2);
      ASSERT(pom2_size > strlen(pom2) + strlen(PAVUK_LOCK_FILENAME));
    }
#endif
    strcat(pom2, PAVUK_LOCK_FILENAME);
    ASSERT(pom2_size > strlen(pom2));
    if(makealldirs(pom2))
    {
      xperror(mk_native(pom2));
    }
    fd = tl_open(pom2, O_BINARY | O_WRONLY | O_CREAT | O_SHORT_LIVED, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(fd < 0)
    {
      xperror(mk_native(pom2));
      _free(ofn);
      _free(idir);
      _free(odir);
      _free(pom);
      _free(pom2);
      return NULL;
    }
    if(_flock(fd, pom2, O_BINARY | O_WRONLY | O_CREAT, TRUE))
    {
      xperror1("Cannot lock file", mk_native(pom2));
      _free(ofn);
      _free(idir);
      _free(odir);
      close(fd);
      _free(pom);
      _free(pom2);
      return NULL;
    }
    strcpy(pom2, idir);
    ASSERT(pom2_size > strlen(pom2));
    strcat(pom2, (tl_is_dirname(idir) ? "" : "/"));
    ASSERT(pom2_size > strlen(pom2));
    if(makealldirs(pom2))
    {
      xperror1("Cannot create directory chain for file", mk_native(pom2));
    }
  }

  us = url_to_urlstr(urlp, FALSE);

  i = 1;
  xidx[0] = '\0';
  while(i > 0)
  {
    /* [i_a] memory/buffer overflow fix: */
    _free(pom);
    pom2[0] = 0;
    pom = tl_str_concat(DBGvars(pom), odir, (tl_is_dirname(odir) ? "" : "/"), xidx, ofn, NULL);
    pom2 = tl_str_concat(DBGvars(pom2), idir, (tl_is_dirname(idir) ? "" : "/"), xidx, ofn, NULL);
    if(tl_access(pom, F_OK) && tl_access(pom2, F_OK))
      break;
    if((dinfos = dinfo_load(pom2)))
    {
      p = get_mime_param_val_str("Original_URL:", dinfos);
      _free(dinfos);
      if(p && !strcmp(p, us))
      {
        _free(p);
        exist = TRUE;
        break;
      }
      _free(p);
    }
    else
    {
      /*
         We don't have info file, but have regular file
         thus we can assume that this file belongs to
         current URL. Usable in case when files were
         before downloaded without -store_info option.
       */
      break;
    }

    sprintf(xidx, "%03d_", i);
    i++;
  }

  /* create info file to know, that this filename is reserved */
  if(lockfn)
  {
    if(!exist)
      dinfo_save_real(pom2, urlp, NULL);

    /* !!!! unlock !!!! */
    _funlock(fd);
    close(fd);
  }

  _free(us);
  _free(idir);
  _free(odir);
  _free(ofn);
  _free(pom2);
  return pom;
}

url *dinfo_get_url_for_filename(char *fn)
{
  char *p = dinfo_get_filename_by_filename(fn);
  char *dinfos;
  url *rv = NULL;

  dinfos = dinfo_load(p);
  _free(p);

  if(dinfos)
  {
    p = get_mime_param_val_str("Original_URL:", dinfos);
    if(p)
    {
      rv = url_parse(p);
      ASSERT(rv->type != URLT_FROMPARENT);
      _free(p);
    }
    if(rv)
    {
      p = get_mime_param_val_str("XPavuk-FormMethod:", dinfos);
      if(p)
      {
        form_info *fi;
        int i;

        fi = _malloc(sizeof(*fi));

        fi->method = FORM_M_GET;
        fi->encoding = FORM_E_URLENCODED;
        fi->action = NULL;
        fi->text = NULL;
        fi->infos = NULL;

        if(!strcasecmp(p, "GET"))
          fi->method = FORM_M_GET;
        else if(!strcasecmp(p, "POST"))
          fi->method = FORM_M_POST;
        else
        {
          DEBUG_HTMLFORM("FORM_M_UNKNOWN: dinfo_get_url_for_filename detected unidentified form method: '%s'\n", p);
          fi->method = FORM_M_UNKNOWN;
        }
        _free(p);

        p = get_mime_param_val_str("XPavuk-FormEncoding:", dinfos);
        if(p)
        {
          if(!strcasecmp(p, "multipart/form-data"))
            fi->encoding = FORM_E_MULTIPART;
          else if(strcasecmp(p, "application/x-www-form-urlencoded"))
            fi->encoding = FORM_E_URLENCODED;
          else
          {
            /* report unrecognized encoding method! */
            DEBUG_HTMLFORM("FORM_E_UNKNOWN: dinfo_get_url_for_filename detected unidentified form encoding: '%s'\n", p);
            fi->encoding = FORM_E_UNKNOWN;
          }
          _free(p);
        }

        for(i = 0; (p = get_mime_n_param_val_str("XPavuk-FormFile:", dinfos, i)); i++)
        {
          char *tp = strchr(p, '=');

          if(tp)
          {
            form_field *ff = _malloc(sizeof(*ff));

            ff->type = FORM_T_FILE;
            ff->name = form_decode_urlencoded_str(p, tp - p);
            ff->value = form_decode_urlencoded_str(tp + 1, strlen(tp));

            fi->infos = dllist_append(fi->infos, (dllist_t) ff);
          }
          else
          {
            xprintf(1, gettext("Error parsing .pavuk_info file field: %s\n"), p);
          }
          _free(p);
        }

        for(i = 0; (p = get_mime_n_param_val_str("XPavuk-FormField:", dinfos, i)); i++)
        {
          char *tp = strchr(p, '=');

          if(tp)
          {
            form_field *ff = _malloc(sizeof(*ff));

            ff->type = FORM_T_TEXT;
            ff->name = form_decode_urlencoded_str(p, tp - p);
            ff->value = form_decode_urlencoded_str(tp + 1, strlen(tp));

            fi->infos = dllist_append(fi->infos, (dllist_t) ff);
          }
          else
          {
            xprintf(1, gettext("Error parsing .pavuk_info file field: %s\n"), p);
          }
          _free(p);
        }

        if(!fi->infos)
          _free(fi);

        if(fi)
        {
          rv->extension = fi;
          rv->status = URL_FORM_ACTION;
        }
      }
    }
    _free(dinfos);
  }

  return rv;
}

void dinfo_remove(char *fn)
{
  char *p, *pinf;

  pinf = dinfo_get_filename_by_filename(fn);

  if(!tl_unlink(pinf))
  {
    char *pom;
    int fd;
    char *inspoint;

    pom = _malloc(strlen(pinf) + TL_MAX(strlen(PAVUK_LOCK_FILENAME), strlen(PAVUK_INFO_DIRNAME)) + 1 + 1);
    strcpy(pom, pinf);
    p = strrchr(pom, '/');
    if(p)
      *(p) = '\0';
    p = strrchr(pom, '/');
    if(p)
      *(p + 1) = '\0';
    inspoint = pom + strlen(pom);
    strcpy(inspoint, PAVUK_LOCK_FILENAME);      /* FIXED - security */

    fd = tl_open(pom, O_BINARY | O_WRONLY | O_CREAT | O_SHORT_LIVED, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(fd < 0)
    {
      xperror(mk_native(pom));
      _free(pom);
      _free(pinf);
      return;
    }
    if(_flock(fd, pom, O_BINARY | O_WRONLY | O_CREAT, TRUE))
    {
      xperror1("Cannot lock file", mk_native(pom));
      close(fd);
      _free(pom);
      _free(pinf);
      return;
    }

#if 0
    p = strrchr(pom, '/');
    if(p)
      *(p + 1) = '\0';
#endif
    strcpy(inspoint, PAVUK_INFO_DIRNAME);       /* FIXED - security */
    if(tl_rmdir(pom) && errno != ENOTEMPTY && errno != EEXIST)
    {
      xperror1("Cannot remove directory", mk_native(pom));
    }

#if 0
    p = strrchr(pom, '/');
    if(p)
      *(p + 1) = '\0';
#endif
    strcpy(inspoint, PAVUK_LOCK_FILENAME);      /* FIXED - security */
    if(tl_unlink(pom) && errno != ENOENT)
    {
      /*
         [i_a] TODO / to be fixed: this will always fail here when using (lock) files on a network share :-(
       */
      xperror1("Cannot unlink/delete file", mk_native(pom));
    }
    _funlock(fd);
    close(fd);
    _free(pom);
  }
  _free(pinf);
}
