/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "tools.h"
#include "url.h"
#include "doc.h"
#include "html.h"
#include "gui_api.h"
#include "update_links.h"

static void rewrite_links(char *);

void update_links(char *dirname)
{
  DIR *dir;
  struct dirent *dent;
  char next_dir[PATH_MAX];
  struct stat estat;

  xprintf(1, gettext("Entering directory %s\n"), dirname);
  if (!(dir = tl_opendir(dirname)))
  {
    xperror(mk_native(dirname));
    return;
  }

  while ((dent = readdir(dir)))
  {
    _Xt_Serve;

    snprintf(next_dir, sizeof(next_dir), "%s%s%s",
             dirname,
             (tl_is_dirname(dirname) ? "" : "/"),
             dent->d_name);
    next_dir[sizeof(next_dir) - 1] = 0;
    if (!strcmp(dent->d_name, "."))
      continue;
    if (!strcmp(dent->d_name, ".."))
      continue;
    if (tl_stat(next_dir, &estat))
    {
      xperror(mk_native(next_dir));
      continue;
    }

    if (S_ISDIR(estat.st_mode))
    {
      if (!strcmp(dent->d_name, PAVUK_INFO_DIRNAME) && cfg.enable_info)
        continue;

      update_links(next_dir);
    }
    else if (file_is_html(next_dir))
    {
      xprintf(1, gettext("Relocating %s\n"), next_dir);
      rewrite_links(next_dir);
    }
    else
      xprintf(1, gettext("Omitting %s\n"), next_dir);
#ifdef I_FACE
    if (cfg.xi_face)
#endif
    {
      if (cfg.rbreak || cfg.stop)
      {
        xprintf(1, gettext("Aborting directory %s\n"), dirname);
        closedir(dir);
        return;
      }
    }
  }

  closedir(dir);
  xprintf(1, gettext("Leaving directory %s\n"), dirname);
}

static void rewrite_links(char *fn)
{
  char *savetmp;
  int sock;
  doc pdoc;
  struct stat estat;

#ifdef HAVE_UTIME
  struct utimbuf ut;
#endif
  url dum;

  if (tl_stat(fn, &estat) == 0)
  {
    if (S_ISDIR(estat.st_mode))
    {
      xprintf(1, gettext("Can't open directory %s\n"), fn);
      return;
    }
  }

#ifdef HAVE_UTIME
  ut.actime = estat.st_atime;
  ut.modtime = estat.st_mtime;
#endif

  memset(&dum, 0, sizeof(dum));
  dum.type = URLT_FILE;
  dum.p.file.filename = fn;
  dum.local_name = fn;
  if (!strcasecmp(tl_get_extension(fn), "css"))
    dum.status = URL_STYLE;
  else
    dum.status = 0;

  doc_init(&pdoc, &dum);

  doc_download(&pdoc, TRUE, TRUE);
#if 0 /* do NOT dump content for known file I/O like here */
#if defined (HAVE_DEBUG_FEATURES)
  {
    char dumpbuf[2048];

    DEBUG_PROTOD("********************* Content: Rewrite Links **************\n");
    DEBUG_PROTOD("%s\n", tl_asciidump_content(dumpbuf, NUM_ELEM(dumpbuf), pdoc.contents, pdoc.size));
    DEBUG_PROTOD("***************************************************\n");
  }
#endif
#endif

  _free(pdoc.mime);
  ASSERT(pdoc.contents == NULL);
  _free(pdoc.contents);         /* [i_a] */
  ASSERT(pdoc.contents == NULL);

  html_process_parent_document(&pdoc, NULL, NULL);

  savetmp = tl_mktempfname(fn, PAVUK_TEMPFNAME_PREFIX "lnkupd");
  if (!savetmp || tl_rename(fn, savetmp))
  {
    xperror2("Cannot rename original file before updating links", mk_native(fn), mk_native(savetmp));
    doc_remove_lock(&pdoc);
    _free(savetmp);
    _free(pdoc.contents);
    _free(pdoc.request_header);
    _free(pdoc.request_data);
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
    pdoc.actrec_ref = NULL;
#endif
    return;
  }

  if ((sock = tl_open(fn, O_BINARY | O_TRUNC | O_CREAT | O_WRONLY,
                      S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR))
      < 0)
  {
    xperror(mk_native(fn));
    if (tl_rename(savetmp, fn))
    {
      xperror2("Cannot restore filename after link update open error",
               mk_native(savetmp), mk_native(fn));
    }
    doc_remove_lock(&pdoc);
    _free(savetmp);
    _free(pdoc.contents);
    _free(pdoc.request_header);
    _free(pdoc.request_data);
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
    pdoc.actrec_ref = NULL;
#endif
    return;
  }
  if (write(sock, pdoc.contents, pdoc.size) != pdoc.size)
  {
    xperror(mk_native(fn));
    close(sock);
    if (tl_rename(savetmp, fn))
    {
      xperror2("Cannot restore filename after link update rewrite error",
               mk_native(savetmp), mk_native(fn));
    }
    doc_remove_lock(&pdoc);
    _free(savetmp);
    _free(pdoc.contents);
    _free(pdoc.request_header);
    _free(pdoc.request_data);
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
    pdoc.actrec_ref = NULL;
#endif
    return;
  }
  close(sock);
#ifdef HAVE_UTIME
  utime(fn, &ut);
#endif
  tl_unlink(savetmp);
  _free(savetmp);
  _free(pdoc.contents);
  _free(pdoc.request_header);
  _free(pdoc.request_data);
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  pdoc.actrec_ref = NULL;
#endif
  doc_remove_lock(&pdoc);
}

