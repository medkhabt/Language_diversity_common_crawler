/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#include "tools.h"
#include "mimetype.h"


/*
 * Produces a collection of MIME types, used by the GUI to allow the user to
 * select a set of acceptable MIME types for pavuk to grab from the
 * remote site(s).
 *
 * Returns a list of MIME type strings, terminated by a NULL string entry.
 */
const char **get_mimetypes_collection(void)
{
  return cfg.mime_type_collection;
}


/*
 * A collection of MIME types, where the preferred filename extension is
 * listed with each MIME type.
 *
 * Used by the -fnrules %M/%X macro logic.
 */
struct mime_type_ext *get_mime_type_ext_collection(void)
{
  return cfg.mime_type_ext_collection;
}


struct mimetype_w_exts
{
  const char  *mimet;
  const char **exts;
};

static dllist_t mimetype_key_func(dllist_t key_data)
{
  return key_data;
}

static unsigned int mimetype_hash_func(unsigned int size, dllist_t key)
{
  struct mimetype_w_exts *mte = (struct mimetype_w_exts *)key;
  unsigned int retv;
  unsigned char *p;
  unsigned int i;

  p = (unsigned char *)mte->mimet;
  ASSERT(p);
  ASSERT(*p);
  for (i = 0, retv = 0; *p; p++, i++)
  {
    retv += i * (unsigned int)*p;
    retv %= size;
  }
  return retv;
}

static int dllist_mimetype_compare(dllist_t key1, dllist_t key2)
{
  return url_compare((url *)key1, (url *)key2);
}

static void free_deep_mimetype(struct mimetype_w_exts *mte)
{
  if (!mte)
    return;

  _free(mte->mimet);
  if (mte->exts)
  {
    const char **p;

    for (p = mte->exts; *p; p++)
    {
      _free(p);
    }
    _free(mte->exts);
  }
  _free(mte);
}

static void mimetype_free_func(dllist_t key_data)
{
  free_deep_mimetype((struct mimetype_w_exts *)key_data);
}

int cfg_load_mime_types(const char *filename)
{
  const char *fn;
  bufio *fd;
  char lnbuf[BUFIO_ADVISED_READLN_BUFSIZE];
  char *lns;
  int rv = 0;
  char *mimet;
  dlhash *mime_hash_tbl;

  free_mime_types();

#if defined (__CYGWIN__) || defined (WIN32) || defined (__WIN32)
  fn = cvt_win32_to_unix_path(filename, FALSE);
#else
  fn = get_abs_file_path_oss(filename);
#endif
  ASSERT(fn != NULL);

  if (!(fd = bufio_open(fn, O_BINARY | O_RDONLY)))
  {
    xvperror(DBGvars(mk_native(fn)), gettext("ERROR: MIME type definitions file loading failed\n"));
    _free(fn);
    return -1;
  }

  mime_hash_tbl = dlhash_new(HASH_TBL_SIZE, mimetype_key_func, mimetype_hash_func, dllist_mimetype_compare);
  dlhash_set_free_func(mime_hash_tbl, mimetype_free_func, NULL);

  while (bufio_readln(fd, lnbuf, sizeof(lnbuf)) > 0)
  {
    struct mimetype_w_exts *mte;
    struct mimetype_w_exts *e;

    strip_nl(lnbuf);
    for (lns = lnbuf; *lns && tl_ascii_isspace(*lns); lns++)
      ;
    if (*lns == '#' || !*lns)
      continue;

    /* extract mimetype from line */
    mimet = lns;
    for ( ; *lns && !tl_ascii_isspace(*lns); lns++)
      ;
    if (*lns)
      *lns++ = 0;

    mte = _malloc(sizeof(*mte));
    mte->mimet = tl_strdup(mimet);
    mte->exts = NULL;
    /* extract filename extension(s): */
    do
    {
      char *ext;

      for ( ; *lns && tl_ascii_isspace(*lns); lns++)
        ;
      ext = lns;
      for ( ; *lns && !tl_ascii_isspace(*lns); lns++)
        ;
      if (*lns)
        *lns++ = 0;
      if (*ext)
      {
        /* add extension to the list; expand the list to allow room for storage: */
        int i;

        for (i = 0; mte->exts && mte->exts[i]; i++)
          ;
        if (!mte->exts)
        {
          ASSERT(i == 0);
          mte->exts = _malloc((i + 2) * sizeof(mte->exts[0]));
        }
        else
        {
          ASSERT(i >= 1);
          mte->exts = (const char **)_realloc((void *)mte->exts, (i + 2) * sizeof(mte->exts[0]));
        }
        mte->exts[i++] = tl_strdup(ext);
        mte->exts[i] = NULL;
      }
    } while (*lns);

    /*
     * Now we have a mimetype 'mimet' with an OPTIONAL set of filename extensions 'exts'.
     *
     * See if we should add this mime type and/or extension set (duplicates in the
     * input file should be tolerated and skipped!)
     */
    e = (struct mimetype_w_exts *)dlhash_find(mime_hash_tbl, mte);
    if (!e)
    {
      dlhash_insert(mime_hash_tbl, (dllist_t)mte);
      mte = NULL;
    }
    else
    {
      /* see if we should add any of those extensions to the existing entry: */
      if (!e->exts)
      {
        e->exts = mte->exts;
        mte->exts = NULL;
      }
      else if (mte->exts)
      {
        const char **src;

        for (src = mte->exts; *src; src++)
        {
          if (!is_in_list(*src, e->exts))
          {
            int i;

            for (i = 2; e->exts[i]; i++)
              ;
            ASSERT(i > 2);
            e->exts = _realloc((void *)e->exts, i * sizeof(mte->exts[0]));
            e->exts[i - 2] = *src;
            e->exts[i - 1] = NULL;
          }
          else
          {
            _free(*src);
          }
        }
        _free(mte->exts);
        ASSERT(mte->exts == NULL);
      }
      ASSERT(mte->exts == NULL);
    }
    free_deep_mimetype(mte);
  }


  /*
   * now we have collected a unique set of mimetypes and a unique set of
   * filename extensions for each mime type:
   * copy these into the cfg destination arrays.
   */
  if (mime_hash_tbl->size > 0)
  {
    int i;

    /* lazy allocation: allocate array in large chunks at a time */
#define MIME_ARR_ALLOC_STEP     16
    int size_m = MIME_ARR_ALLOC_STEP;
    int size_e = MIME_ARR_ALLOC_STEP;
    int cnt_m = 0;
    int cnt_e = 0;

    cfg.mime_type_collection = _malloc(size_m * sizeof(cfg.mime_type_collection[0]));
    cfg.mime_type_ext_collection = _malloc(size_e * sizeof(cfg.mime_type_ext_collection[0]));

    for (i = 0; i < mime_hash_tbl->size; i++)
    {
      dllist *ptr;

      for (ptr = mime_hash_tbl->nodes[i]; ptr; ptr = ptr->next)
      {
        struct mimetype_w_exts *e = (struct mimetype_w_exts *)ptr->data;
        const char **p;

        if (cnt_m > size_m - 2)
        {
          size_m += MIME_ARR_ALLOC_STEP;
          cfg.mime_type_collection = _realloc((void *)cfg.mime_type_collection, size_m * sizeof(cfg.mime_type_collection[0]));
        }
        cfg.mime_type_collection[cnt_m++] = tl_strdup(e->mimet);

        if (e->exts)
        {
          for (p = e->exts; *p; p++)
          {
            struct mime_type_ext *mte;

            if (cnt_e > size_e - 2)
            {
              size_e += MIME_ARR_ALLOC_STEP;
              cfg.mime_type_ext_collection = _realloc((void *)cfg.mime_type_ext_collection, size_e * sizeof(cfg.mime_type_ext_collection[0]));
            }
            mte = &cfg.mime_type_ext_collection[cnt_e++];

            mte->mimet = tl_strdup(e->mimet);
            mte->ext = *p;
            *p = NULL;
          }
        }
      }
    }

    /* write the sentinels! */
    ASSERT(cnt_m < size_m);
    cfg.mime_type_collection[cnt_m] = NULL;
    ASSERT(cnt_e < size_e);
    cfg.mime_type_ext_collection[cnt_e].ext = NULL;
    cfg.mime_type_ext_collection[cnt_e].mimet = NULL;
  }

  bufio_close(fd);
  fd = NULL;

  dlhash_empty(mime_hash_tbl);
  dlhash_free(mime_hash_tbl);
  _free(fn);

  return rv;
}

void init_mime_types(void)
{
  static const char *mime_types[] =
  {
    "application/activemessage",
    "application/andrew-insert",
    "application/applefile",
    "application/atomicmail",
    "application/dca-rft",
    "application/dec-dx",
    "application/mac-binhex40",
    "application/macwriteii",
    "application/msword",
    "application/news-message-id",
    "application/news-transmission",
    "application/octet-stream",
    "application/oda",
    "application/pdf",
    "application/postscript",
    "application/remote-printing",
    "application/rtf",
    "application/slate",
    "application/x-mif",
    "application/wita",
    "application/wordperfect5.1",
    "application/x-csh",
    "application/x-dvi",
    "application/x-hdf",
    "application/x-latex",
    "application/x-netcdf",
    "application/x-sh",
    "application/x-tcl",
    "application/x-tex",
    "application/x-texinfo",
    "application/x-troff",
    "application/x-troff-man",
    "application/x-troff-me",
    "application/x-troff-ms",
    "application/x-wais-source",
    "application/zip",
    "application/x-bcpio",
    "application/x-cpio",
    "application/x-gtar",
    "application/x-shar",
    "application/x-sv4cpio",
    "application/x-sv4crc",
    "application/x-tar",
    "application/x-ustar",
    "audio/basic",
    "audio/x-aiff",
    "audio/x-wav",
    "image/gif",
    "image/ief",
    "image/jpeg",
    "image/pjpeg",
    "image/tiff",
    "image/x-cmu-raster",
    "image/x-portable-anymap",
    "image/x-portable-bitmap",
    "image/x-portable-graymap",
    "image/x-portable-pixmap",
    "image/x-rgb",
    "image/x-xbitmap",
    "image/x-xpixmap",
    "image/x-xwindowdump",
    "message/external-body",
    "message/news",
    "message/partial",
    "message/rfc822",
    "multipart/alternative",
    "multipart/appledouble",
    "multipart/digest",
    "multipart/mixed",
    "multipart/parallel",
    "text/html",
    "text/plain",
    "text/richtext",
    "text/tab-separated-values",
    "text/x-setext",
    "video/mpeg",
    "video/quicktime",
    "video/x-msvideo",
    "video/x-sgi-movie",
    0
  };

  static const struct mime_type_ext mime_type_exts[] =
  {
    { "text/html*", "html" },
    { "text/js", "js" },
    { "text/plain", "txt" },
    { "image/jpeg", "jpg" },
    { "image/pjpeg", "jpg" },
    { "image/gif", "gif" },
    { "image/png", "png" },
    { "image/tiff", "tiff" },
    { "application/pdf", "pdf" },
    { "application/msword", "doc" },
    { "application/postscript", "ps" },
    { "application/rtf", "rtf" },
    { "application/wordperfect5.1", "wps" },
    { "application/zip", "zip" },
    { "video/mpeg", "mpg" },
    { 0, 0 }
  };

  int len;
  int i;

  free_mime_types();

  len = sizeof(mime_types) / sizeof(mime_types[0]);
  cfg.mime_type_collection = _malloc(len * sizeof(cfg.mime_type_collection[0]));

  for (i = 0; i < len; i++)
  {
    if (mime_types[i])
    {
      cfg.mime_type_collection[i] = tl_strdup(mime_types[i]);
    }
    else
    {
      cfg.mime_type_collection[i] = NULL;
    }
  }

  len = sizeof(mime_type_exts) / sizeof(mime_type_exts[0]);
  cfg.mime_type_ext_collection = _malloc(len * sizeof(cfg.mime_type_ext_collection[0]));

  for (i = 0; i < len; i++)
  {
    struct mime_type_ext *mte;

    mte = &cfg.mime_type_ext_collection[i];

    if (mime_type_exts[i].mimet)
    {
      mte->mimet = tl_strdup(mime_type_exts[i].mimet);
      mte->ext = tl_strdup(mime_type_exts[i].ext);
    }
    else
    {
      mte->mimet = NULL;
      mte->ext = NULL;
    }
  }
}

void free_mime_types(void)
{
  int i;

  if (cfg.mime_type_collection)
  {
    for (i = 0; cfg.mime_type_collection[i]; i++)
    {
      _free(cfg.mime_type_collection[i]);
    }
    _free(cfg.mime_type_collection);
  }

  if (cfg.mime_type_ext_collection)
  {
    for (i = 0; cfg.mime_type_ext_collection[i].mimet; i++)
    {
      _free(cfg.mime_type_ext_collection[i].mimet);
      _free(cfg.mime_type_ext_collection[i].ext);
    }
    _free(cfg.mime_type_ext_collection);
  }
}



