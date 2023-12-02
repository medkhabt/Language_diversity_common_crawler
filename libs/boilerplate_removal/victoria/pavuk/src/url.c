/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "gui.h"
#include "http.h"
#include "ftp.h"
#include "gopher.h"
#include "url.h"
#include "html.h"
#include "tools.h"
#include "authinfo.h"
#include "tr.h"
#include "dinfo.h"
#include "form.h"
#include "gui_api.h"
#include "lfname.h"

static char *url_decode_html(const char *, int);

#if 0                           /* [i_a] moved to config.h */


/*
   here can you specify characters,
   which are unsafe in file paths.

   FIXME: these are unsafe FileSYSTEM characters,
   i.e. we don't take proper care of Win32 path
   specifications, which use '\\' next to '/'
   (latest Windows also seems to support '/' out
   of the box, but doesn't return this path delimiter
   itself though).
 */
#if defined(__CYGWIN__) || defined(WIN32) || defined(__WIN32)
#define FS_UNSAFE_CHARACTERS "\\:*?\"<>|&^"
#else
#define FS_UNSAFE_CHARACTERS "*?\"<>|"
#endif

#endif



const protinfo prottable[] = {
  {URLT_UNKNOWN, NULL, "unknown", NULL, 0, FALSE},
  {URLT_HTTP, "http", "http", "http://", 80, TRUE},
#ifdef USE_SSL
  {URLT_HTTPS, "https", "https", "https://", 443, TRUE},
#else
  {URLT_HTTPS, "https", "https", "https://", 443, FALSE},
#endif
  {URLT_FTP, "ftp", "ftp", "ftp://", 21, TRUE},
#ifdef USE_SSL
  {URLT_FTPS, "ftps", "ftps", "ftps://", 21, TRUE},
#else
  {URLT_FTPS, "ftps", "ftps", "ftps://", 21, FALSE},
#endif
  {URLT_FILE, NULL, "file", "file://", 0, TRUE},
  {URLT_GOPHER, "gopher", "gopher", "gopher://", 70, TRUE},
  {URLT_FROMPARENT, NULL, "//", "//", 80, TRUE}
};

#define _STRCLS_LOWER "abcdefghijklmnopqrstuvwxyz"
#define _STRCLS_UPER  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define _STRCLS_DIGIT "0123456789"

char *url_parse_scheme(char *urlstr)
{
  char *p;
  char *retv = NULL;

  if((p = strchr(urlstr, ':')) && tl_ascii_isalpha(*urlstr))
  {
    int l1 = strspn(urlstr, _STRCLS_LOWER _STRCLS_UPER _STRCLS_DIGIT "+-.");

    if(l1 == (p - urlstr))
    {
      retv = tl_strndup(urlstr, l1);
      lowerstr(retv);
    }
  }
  else
  {
    if(urlstr[0] == '/' && urlstr[1] == '/')
      retv = tl_strdup("//");
  }

  return retv;
}

static char *url_parse_authority(char *urlschpart)
{
  char *retv = NULL;

  if(urlschpart[0] == '/' && urlschpart[1] == '/')
  {
    int l1 = strcspn(urlschpart + 2, "/?#;");

    retv = tl_strndup(urlschpart + 2, l1);
  }

  return retv;
}

static int url_split_authority(char *authority, char **user, char **password, char **host, unsigned short *port)
{
  char *p, *p2;

  if(user)
    *user = NULL;
  if(password)
    *password = NULL;
  *host = NULL;
  *port = 0;

  if(user && (p = strrchr(authority, '@')))
  {
    p2 = strchr(authority, ':');

    if(p2 && p2 < p)
    {
      *user = tl_strndup(authority, p2 - authority);
      *password = tl_strndup(p2 + 1, p - p2 - 1);
    }
    else
    {
      *user = tl_strndup(authority, p - authority);
    }
    p++;
  }
  else
    p = authority;

  if((p2 = strrchr(p, ':')))
  {
    *host = tl_strndup(p, p2 - p);
    *port = _atoi(p2 + 1);
  }
  else
  {
    *host = tl_strdup(p);
  }

  lowerstr(*host);

  return 0;
}

#undef get_abs_file_path
static int url_split_path(DBGdecl(char *urlpath), char **path, char **query, char **anchor)
{
  char *p = NULL, *p1 = NULL, *p2 = NULL;

  *path = NULL;
  if(query)
    *query = NULL;
  if(anchor)
    *anchor = NULL;

  if(anchor)
    p1 = strchr(urlpath, '#');

  if(query)
    p2 = strchr(urlpath, '?');

  if(p1 && p2)
  {
    if(p1 > p2)
    {
      *anchor = tld_strdup(DBGpass(p1 + 1));
      *query = url_decode_html(p2 + 1, p1 - (p2 + 1));
      p = p2;
    }
    else
    {
      *query = url_decode_html(p2 + 1, strlen(p2 + 1));
      *anchor = tl_strndup(p1 + 1, p2 - (p1 + 1));
      p = p1;
    }
  }
  else if(p1)
  {
    *anchor = tld_strdup(DBGpass(p1 + 1));
    p = p1;
  }
  else if(p2)
  {
    *query = url_decode_html(p2 + 1, strlen(p2 + 1));
    p = p2;
  }

  if(p)
  {
    if(p - urlpath)
    {
      *path = tld_strndup(DBGpass(urlpath), p - urlpath);
      if(**path == '/')
      {
        p = *path;
        *path = get_abs_file_path(DBGpass(_strtrchr(p, '\\', '/')));
        _free(p);
      }
    }
  }
  else
  {
    if(*urlpath)
    {
      *path = tld_strdup(DBGpass(urlpath));
      if(**path == '/')
      {
        p = *path;
        *path = get_abs_file_path(DBGpass(_strtrchr(p, '\\', '/')));
        _free(p);
      }
    }
  }

  return 0;
}

protocol url_scheme_to_schemeid(char *scheme)
{
  int i;
  for(i = 0; i < NUM_ELEM(prottable); i++)
  {
    if(prottable[i].urlid && !strcmp(prottable[i].urlid, scheme))
    {
      return prottable[i].id;
    }
  }
  return URLT_UNKNOWN;
}

/*
 * If a path is relative and starts // we need to get the type from
 * the parent, which only the caller can do. This function is called
 * by the caller of url_parse when url_parse has returned  type = URTL_FROMPARENT
 * and the parent can figure out the path. It basically does all the work
 * that url_parse would do once it knew the scheme.
 * however, we start with the urlstr in url->urlstr rather
 * than as an argument
 */
static void url_finishpath(url * url)
{
  char *authority = NULL;
  char *p;

  if(url->type == URLT_FROMPARENT)
    url->type = URLT_UNKNOWN;
  if(url->type == URLT_UNKNOWN)
    return;                     /* can't help here */

  p = url->urlstr;
  authority = url_parse_authority(p);
  if(authority)
    p += strlen(authority) + 2;

  if(authority && *authority)
  {
    switch (url->type)
    {
    case URLT_FROMPARENT:
      break;
    case URLT_HTTP:
    case URLT_HTTPS:
      url_split_authority(authority, &(url->p.http.user), &(url->p.http.password), &(url->p.http.host), &(url->p.http.port));

      if(!url->p.http.port)
        url->p.http.port = prottable[url->type].default_port;

      ASSERT(!url->p.http.document);
      ASSERT(!url->p.http.searchstr);
      ASSERT(!url->p.http.anchor_name);
      url_split_path(DBGvars(p), &(url->p.http.document), &(url->p.http.searchstr), &(url->p.http.anchor_name));

      if(!url->p.http.document)
        url->p.http.document = tl_strdup("/");
      break;
    case URLT_FTP:
    case URLT_FTPS:
      url_split_authority(authority, &(url->p.ftp.user), &(url->p.ftp.password), &(url->p.ftp.host), &(url->p.ftp.port));

      if(!url->p.ftp.port)
        url->p.ftp.port = prottable[url->type].default_port;

      ASSERT(!url->p.ftp.path);
      ASSERT(!url->p.ftp.anchor_name);
      url_split_path(DBGvars(p), &url->p.ftp.path, NULL, &url->p.ftp.anchor_name);

      if(!url->p.ftp.path)
        url->p.ftp.path = tl_strdup("/");

      if(p && p[0] == '/' && p[1] == '/')
      {
        char *pp = tl_str_concat(DBGvars(NULL), "/", url->p.ftp.path, NULL);
        _free(url->p.ftp.path);
        url->p.ftp.path = pp;
      }

      if((p = strrchr(url->p.ftp.path, ';')) && !strncasecmp(p, ";type=", 6))
        *p = '\0';

      url->p.ftp.dir = tl_is_dirname(url->p.ftp.path) != 0;
      break;
    case URLT_GOPHER:
      url_split_authority(authority, NULL, NULL, &(url->p.gopher.host), &(url->p.gopher.port));

      if(!url->p.gopher.port)
        url->p.gopher.port = prottable[url->type].default_port;

      if(*(p + 1))
        url->p.gopher.selector = tl_strdup(p + 1);
      else
        url->p.gopher.selector = tl_strdup("1");
      break;
    case URLT_FILE:
      ASSERT(!url->p.file.filename);
      ASSERT(!url->p.file.searchstr);
      ASSERT(!url->p.file.anchor_name);
      url_split_path(DBGvars(p), &(url->p.file.filename), &(url->p.file.searchstr), &(url->p.file.anchor_name));

      if(!url->p.file.filename)
        url->p.file.filename = tl_strdup("");
      break;
    default:
      return;
    }
  }
  if(!authority || !*authority)
  {
    switch (url->type)
    {
    case URLT_FILE:
    case URLT_FTP:
    case URLT_FTPS:
    case URLT_HTTP:
    case URLT_HTTPS:
      url->type = URLT_FILE;
      ASSERT(!url->p.file.filename);
      ASSERT(!url->p.file.searchstr);
      ASSERT(!url->p.file.anchor_name);
      url_split_path(DBGvars(p), &(url->p.file.filename), &(url->p.file.searchstr), &(url->p.file.anchor_name));

      if(!url->p.file.filename)
        url->p.file.filename = tl_strdup("");
      break;
    default:
      url->type = URLT_UNKNOWN;
      return;
      break;
    }
  }
  _free(authority);
  return;
}

#undef url_parse
url *url_parse(DBGdecl(char *urlstr))
{
  char *scheme = NULL;
  char *authority = NULL;
  char *p;
  url ret_url = { 0 };

  ret_url.type = URLT_UNKNOWN;
  ret_url.status = 0;
  ret_url.parent_url = NULL;
  ret_url.moved_to = NULL;
  ret_url.ref_cnt = 1;
  ret_url.level = 0;
  ret_url.extension = NULL;
  ret_url.local_name = NULL;

#ifdef WITH_TREE
#ifdef I_FACE
  ret_url.prop = NULL;
  ret_url.tree_nfo = NULL;
#endif
#endif

#ifdef HAVE_MT
  pthread_mutex_init(&ret_url.lock, NULL);
#endif

  p = urlstr;

  if(p)
    scheme = url_parse_scheme(urlstr);

  if(scheme)
  {
    ret_url.type = url_scheme_to_schemeid(scheme);

    /* If the string starts with // then we */
    /* don't know the scheme type so we have */
    /* to wait for the parent to set it. */
    if(ret_url.type == URLT_FROMPARENT)
    {
      ret_url.urlstr = tld_strdup(DBGpass(urlstr));
      authority = url_parse_authority(urlstr);
    }
    else
    {
      /* We do know the scheme type, so move past it */
      /* and get the 'authority' */
      p += strlen(scheme) + 1;
      authority = url_parse_authority(p);
    }

    if(authority)
      p += strlen(authority) + 2;

    if(authority && *authority)
    {
      switch (ret_url.type)
      {
      case URLT_FROMPARENT:
        break;
      case URLT_HTTP:
      case URLT_HTTPS:
        url_split_authority(authority, &ret_url.p.http.user, &ret_url.p.http.password, &ret_url.p.http.host, &ret_url.p.http.port);

        if(!ret_url.p.http.port)
          ret_url.p.http.port = prottable[ret_url.type].default_port;

        ASSERT(!ret_url.p.http.document);
        ASSERT(!ret_url.p.http.searchstr);
        ASSERT(!ret_url.p.http.anchor_name);
        url_split_path(DBGvars(p), &ret_url.p.http.document, &ret_url.p.http.searchstr, &ret_url.p.http.anchor_name);

        if(!ret_url.p.http.document)
          ret_url.p.http.document = tl_strdup("/");
        break;
      case URLT_FTP:
      case URLT_FTPS:
        url_split_authority(authority, &ret_url.p.ftp.user, &ret_url.p.ftp.password, &ret_url.p.ftp.host, &ret_url.p.ftp.port);

        if(!ret_url.p.ftp.port)
          ret_url.p.ftp.port = prottable[ret_url.type].default_port;

        ASSERT(!ret_url.p.ftp.path);
        ASSERT(!ret_url.p.ftp.anchor_name);
        url_split_path(DBGvars(p), &ret_url.p.ftp.path, NULL, &ret_url.p.ftp.anchor_name);


        if(!ret_url.p.ftp.path)
          ret_url.p.ftp.path = tl_strdup("/");

        if(p && p[0] == '/' && p[1] == '/')
        {
          char *pp = tl_str_concat(DBGvars(NULL), "/", ret_url.p.ftp.path, NULL);
          _free(ret_url.p.ftp.path);
          ret_url.p.ftp.path = pp;
        }

        if((p = strrchr(ret_url.p.ftp.path, ';')) && !strncasecmp(p, ";type=", 6))
          *p = '\0';

        ret_url.p.ftp.dir = tl_is_dirname(ret_url.p.ftp.path) != 0;
        break;
      case URLT_GOPHER:
        url_split_authority(authority, NULL, NULL, &ret_url.p.gopher.host, &ret_url.p.gopher.port);

        if(!ret_url.p.gopher.port)
          ret_url.p.gopher.port = prottable[ret_url.type].default_port;

        if(*(p + 1))
          ret_url.p.gopher.selector = tl_strdup(p + 1);
        else
          ret_url.p.gopher.selector = tl_strdup("1");
        break;
      case URLT_FILE:
        ASSERT(!ret_url.p.file.filename);
        ASSERT(!ret_url.p.file.searchstr);
        ASSERT(!ret_url.p.file.anchor_name);
        url_split_path(DBGvars(p), &ret_url.p.file.filename, &ret_url.p.file.searchstr, &ret_url.p.file.anchor_name);

        if(!ret_url.p.file.filename)
          ret_url.p.file.filename = tl_strdup("");
        break;
      default:
        ret_url.urlstr = tld_strdup(DBGpass(urlstr));
        break;
      }
    }
  }

  if(!scheme || !authority || !*authority)
  {
    if(!scheme)
      ret_url.type = URLT_FILE;

    switch (ret_url.type)
    {
    case URLT_FILE:
    case URLT_FTP:
    case URLT_FTPS:
    case URLT_HTTP:
    case URLT_HTTPS:
    case URLT_FROMPARENT:
      ret_url.type = URLT_FILE;
      ASSERT(!ret_url.p.file.filename);
      ASSERT(!ret_url.p.file.searchstr);
      ASSERT(!ret_url.p.file.anchor_name);
      url_split_path(DBGvars(p), &ret_url.p.file.filename, &ret_url.p.file.searchstr, &ret_url.p.file.anchor_name);

      if(!ret_url.p.file.filename)
        ret_url.p.file.filename = tld_strdup(DBGpass(""));
      break;
    default:
      ret_url.type = URLT_UNKNOWN;
      ret_url.urlstr = tld_strdup(DBGpass(urlstr));
      break;
    }
  }

  _free(authority);
  _free(scheme);
  return new_url(DBGpass(&ret_url));
}

#undef url_dup_url
url *url_dup_url(DBGdecl(url * src))
{
  url dst = { 0 };

  dst.type = src->type;
  dst.parent_url = NULL;
  dst.moved_to = NULL;
  dst.level = src->level;
  dst.ref_cnt = 1;
  dst.status = src->status & (URL_INLINE_OBJ | URL_STYLE | URL_ISHTML | URL_NORECURSE | URL_FORM_ACTION | URL_ISSCRIPT | URL_ISSTARTING
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
    | URL_ISHAMMER
#endif
    );
  dst.extension = NULL;
  dst.local_name = NULL;
#ifdef WITH_TREE
#ifdef I_FACE
  dst.prop = NULL;
  dst.tree_nfo = NULL;
#endif
#endif
#ifdef HAVE_MT
  pthread_mutex_init(&dst.lock, NULL);
#endif
  switch (dst.type)
  {
  case URLT_FILE:
    dst.p.file.filename = tl_strdup(src->p.file.filename);
    dst.p.file.searchstr = tl_strdup(src->p.file.searchstr);
    dst.p.file.anchor_name = tl_strdup(src->p.file.anchor_name);
    break;
  case URLT_FTP:
  case URLT_FTPS:
    dst.p.ftp.host = tl_strdup(src->p.ftp.host);
    dst.p.ftp.user = tl_strdup(src->p.ftp.user);
    dst.p.ftp.password = tl_strdup(src->p.ftp.password);
    dst.p.ftp.path = tl_strdup(src->p.ftp.path);
    dst.p.ftp.anchor_name = tl_strdup(src->p.ftp.anchor_name);
    dst.p.ftp.port = src->p.ftp.port;
    dst.p.ftp.dir = src->p.ftp.dir;
    if(src->extension)
      dst.extension = ftp_url_ext_dup(src->extension);
    break;
  case URLT_HTTP:
  case URLT_HTTPS:
    dst.p.http.host = tl_strdup(src->p.http.host);
    dst.p.http.port = src->p.http.port;
    dst.p.http.document = tl_strdup(src->p.http.document);
    dst.p.http.searchstr = tl_strdup(src->p.http.searchstr);
    dst.p.http.anchor_name = tl_strdup(src->p.http.anchor_name);
    dst.p.http.user = tl_strdup(src->p.http.user);
    dst.p.http.password = tl_strdup(src->p.http.password);
    if(src->extension && (src->status & URL_FORM_ACTION))
      dst.extension = form_info_dup(src->extension);
    break;
  case URLT_GOPHER:
    dst.p.gopher.host = tl_strdup(src->p.gopher.host);
    dst.p.gopher.port = src->p.gopher.port;
    dst.p.gopher.selector = tl_strdup(src->p.gopher.selector);
    break;
  case URLT_FROMPARENT:        /* This is a 'can't happen'. */
    ASSERT(0);
  case URLT_UNKNOWN:
    break;
  }
  dst.urlstr = tld_strdup(DBGpass(src->urlstr));

  return new_url(DBGpass(&dst));
}

/* convert any URL string to absolute path */
char *url_to_absolute_url(char *base, char *baset, url * parent, char *act)
{
  char *psp = NULL;
  url *purl;
  char *pom;
  int pomlen;

  if(act[0] == 0)
    return 0;
  if(act[0] == '#')
    return 0;

  pomlen = strlen(url_to_filename(parent, TRUE)) + strlen(priv_cfg.cache_dir) + strlen(baset) + strlen(act) + 1;
  if(pomlen < PATH_MAX)
    pomlen = PATH_MAX;
  pom = _malloc(pomlen);

  if((act[0] == '/' && act[1] == '/') && parent->type != URLT_FILE)
  {
    /* we should handle it like net_path */
    snprintf(pom, pomlen, "%s:%s", prottable[parent->type].urlid, act);
    pom[pomlen - 1] = 0;
    psp = tl_strdup(pom);
    purl = url_parse(DBGvars(act));
  }
  else
  {
    purl = url_parse(DBGvars(act));
  }

  if(purl->type == URLT_FROMPARENT)
  {
    purl->type = parent->type;
    url_finishpath(purl);
  }
  ASSERT(purl->type != URLT_FROMPARENT);

  if(purl->type == URLT_FILE && parent->type == URLT_FILE)
  {
    if(!(*purl->p.file.filename))
    {
      strncpy(pom, baset, pomlen);
        pom[pomlen - 1] = 0;
    }
    else
    {
      if(*(purl->p.file.filename) != '/')
      {
        strncpy(pom, base, pomlen);
        pom[pomlen - 1] = 0;
        strncat(pom, purl->p.file.filename, pomlen - strlen(pom));
        pom[pomlen - 1] = 0;

        _free(purl->p.file.filename);
        purl->p.file.filename = tl_strdup(pom);
      }
      else
      {
        snprintf(pom, pomlen, "%s%s", prottable[purl->type].typestr, purl->p.file.filename);
      pom[pomlen - 1] = 0;
      }
    }
    psp = tl_strdup(pom);
  }
  else if((purl->type == URLT_FILE) &&
    (cfg.base_level == 0 || cfg.enable_info) && (parent->status & URL_REDIRECT || parent->status & URL_ISLOCAL))
  {
    char *p1, *p;
    url *pomurl;

    if(*purl->p.file.filename == '/')
    {
      strncpy(pom, purl->p.file.filename, pomlen);
      pom[pomlen - 1] = 0;
    }
    else
    {
      int l;
      p = url_to_filename(parent, TRUE);
      strncpy(pom, p, pomlen);
        pom[pomlen - 1] = 0;

      if(*purl->p.file.filename)
      {
        p1 = strrchr(pom, '/');
        if(p1)
          *(p1 + 1) = '\0';
        strncat(pom, purl->p.file.filename, pomlen - strlen(pom));
        pom[pomlen - 1] = 0;
      }
      /* remove any dynamic stuff to get base name */
      for(l = strlen(pom); l > 0 && pom[l] != '/' && pom[l] != '?'; --l)
        ;
      if(pom[l] == '?')
        pom[l] = '\0';
      /* now fix for index-name files */
      l = strlen(pom) - strlen(priv_cfg.index_name);
      if(l > 0 && !strcmp(pom + l, priv_cfg.index_name) && pom[l - 1] == '/')
        pom[l] = '\0';
      ASSERT(strlen(pom) < pomlen);
    }
    if(purl->p.file.searchstr)
    {
        snprintf(pom + strlen(pom), pomlen - strlen(pom), "?%s", purl->p.file.searchstr);
        pom[pomlen - 1] = 0;
      ASSERT(strlen(pom) < pomlen);
    }

    if(purl->p.file.anchor_name)
    {
		snprintf(pom + strlen(pom), pomlen - strlen(pom), "#%s", purl->p.file.anchor_name);
        pom[pomlen - 1] = 0;
      ASSERT(strlen(pom) < pomlen);
    }

    p = get_abs_file_path(DBGvars(pom));
    pomurl = filename_to_url(p);
    _free(p);
    if(pomurl)
    {
      psp = url_to_urlstr(pomurl, TRUE);
      free_deep_url(pomurl);
      _free(pomurl);
    }
  }
  if((!psp && purl->type == URLT_FILE) &&
    (parent->type == URLT_HTTP || parent->type == URLT_HTTPS || parent->type == URLT_FTPS || parent->type == URLT_FTP))
  {
    char *ri;
    if(*(purl->p.file.filename) == '/')
    {
      char *idx;

        strncpy(pom, base, pomlen);
        pom[pomlen - 1] = 0;
      idx = (char *) strfindnchr(pom, '/', 3);
      if(idx)
        strncpy(idx, purl->p.file.filename, pomlen - (idx - pom));
      else
        strncat(pom, purl->p.file.filename, pomlen - strlen(pom));
        pom[pomlen - 1] = 0;

      if(purl->p.file.searchstr)
      {
        snprintf(pom + strlen(pom), pomlen - strlen(pom), "?%s", purl->p.file.searchstr);
        pom[pomlen - 1] = 0;
      ASSERT(strlen(pom) < pomlen);
      }

      if(purl->p.file.anchor_name)
      {
		snprintf(pom + strlen(pom), pomlen - strlen(pom), "#%s", purl->p.file.anchor_name);
        pom[pomlen - 1] = 0;
      ASSERT(strlen(pom) < pomlen);
      }
    }
    else if(!(*purl->p.file.filename) && !purl->p.file.searchstr)
    {
      if(purl->p.file.anchor_name)
      {
        /* Problem; we just have "#anchor" and unfortunately
           baset might be the parent directory, not the
           actual parent. (Nor is that found in "parent"
           necessarily).
         */

        if(*baset && baset[strlen(baset) - 1] != '/')
        {
		snprintf(pom, pomlen, "%s#%s", baset, purl->p.file.anchor_name);
        pom[pomlen - 1] = 0;
        }
        else
        {
          /* What to do? Just hope to ignore this altogether */
          pom[0] = 0;
        }
        ASSERT(strlen(pom) < pomlen);
      }
    }
    else
    {
      strncpy(pom, base, pomlen);
        pom[pomlen - 1] = 0;
      if(!*purl->p.file.filename && purl->p.file.searchstr && parent->type == URLT_HTTP)
      {
        ri = strrchr(pom, '/');
        if(ri)
        strncpy(ri, parent->p.http.document, pomlen - (ri - pom));
        else
        strncat(pom, parent->p.http.document, pomlen - strlen(pom));
        pom[pomlen - 1] = 0;
      }
      else
      {
        ri = strrchr(pom, '/');
        if(ri)
        strncat(ri + 1, purl->p.file.filename, pomlen - (ri + 1 - pom));
        else
        strncat(pom, purl->p.file.filename, pomlen - strlen(pom));
        pom[pomlen - 1] = 0;
      }
      ASSERT(strlen(pom) < pomlen);

      if((parent->status & URL_REDIRECT) &&
        (strlen(purl->p.file.filename) >= strlen(priv_cfg.index_name)) &&
        !strcmp(priv_cfg.index_name, purl->p.file.filename + strlen(purl->p.file.filename) - strlen(priv_cfg.index_name)))
      {
        *(pom + strlen(pom) - strlen(priv_cfg.index_name)) = '\0';
      }
      ASSERT(strlen(pom) < pomlen);

      if(purl->p.file.searchstr)
      {
        snprintf(pom + strlen(pom), pomlen - strlen(pom), "?%s", purl->p.file.searchstr);
        pom[pomlen - 1] = 0;
      }

      if(purl->p.file.anchor_name)
      {
		snprintf(pom + strlen(pom), pomlen - strlen(pom), "#%s", purl->p.file.anchor_name);
        pom[pomlen - 1] = 0;
      }
      ASSERT(strlen(pom) < pomlen);
    }
    psp = tl_strdup(pom);
  }
  else if(!psp)
  {
    psp = tl_strdup(act);
  }

  free_deep_url(purl);
  _free(purl);

  if(psp && *psp)
  {
    purl = url_parse(DBGvars(psp));
    if(purl->type == URLT_FROMPARENT)
    {
      purl->type = parent->type;
      url_finishpath(purl);
    }
    url_path_abs(purl);
    if(prottable[purl->type].supported)
    {
      _free(psp);
      psp = url_to_urlstr(purl, TRUE);
    }
    free_deep_url(purl);
    _free(purl);
  }

  _free(pom);

  return psp;
}

/**************************************/
/* encode unsafe characters with      */
/* url-encoded encoding               */
/**************************************/
static char *url_encode_str_real(char *urlstr, char *unsafe, int safety)
{
  static const char hexa[] = "0123456789ABCDEF";
  char *res, *p, *r;

  if(urlstr == NULL)
    return NULL;

  if(cfg.noencode)
  {
    return tl_strdup(urlstr);
  }

  res = _malloc(strlen(urlstr) * 3 + 1);

  for(p = urlstr, r = res; *p; p++, r++)
  {
    if(safety && *p == '%' && tl_ascii_isxdigit(p[1]) && tl_ascii_isxdigit(p[2]))
    {
      *r = *p;
    }
    else if(strchr(unsafe, *p) || (p[0] >= 0x7f) || (p[0] < 0x20))
    {
      *r = '%';
      r++;
      *r = hexa[((unsigned char *) p)[0] >> 4];
      r++;
      *r = hexa[((unsigned char *) p)[0] % 16];
    }
    else
    {
      *r = *p;
    }
  }
  *r = '\0';

  return res;
}

char *url_encode_str(char *urlstr, char *unsafe)
{
  return url_encode_str_real(urlstr, unsafe, FALSE);
}

static char *url_encode_str_safe(char *urlstr, char *unsafe)
{
  return url_encode_str_real(urlstr, unsafe, TRUE);
}

/* Convert the HTML entities to direct characters, size is ignored at the
moment, res returns the encoded character, the return value is the number
of encoded bytes. Currently only &amp; is handled! */
static int fix_html_entity(const char *str, int size, char *res)
{
  if(size >= 5 && (!strncmp(str, "&amp;", 5) || !strncmp(str, "&#38;", 5)))
  {
    *res = '&';
    return 5;
  }
  return 0;
}

/*
   Decode any URL entities to the actual characters.
 */
char *url_decode_str(const char *urlstr, int len)
{
  char *res, *r;
  int i;

  if(urlstr == NULL)
    return NULL;

  res = tl_strndup(urlstr, len);

  for(i = 0, r = res; i < len; r++, i++)
  {
    if(urlstr[i] == '%' && urlstr[i + 1] && urlstr[i + 2] && tl_ascii_isxdigit(urlstr[i + 1]) && tl_ascii_isxdigit(urlstr[i + 2]))
    {
      *r = HEX2CHAR(urlstr + i + 1);
      i += 2;
    }
    else if(urlstr[i] == '&')
    {
      int s;
      if((s = fix_html_entity(urlstr + i, len - i, r)))
        i += s - 1;
      else
        *r = urlstr[i];         /* copy the & */
    }
    else
    {
      *r = urlstr[i];
    }
  }
  *r = '\0';

  return res;
}

static char *url_decode_html(const char *urlstr, int len)
{
  char *res, *r;
  int i;

  if(urlstr == NULL)
    return NULL;

  res = tl_strndup(urlstr, len);

  for(i = 0, r = res; i < len; r++, i++)
  {
    if(urlstr[i] == '&')
    {
      int s;
      if((s = fix_html_entity(urlstr + i, len - i, r)))
        i += s - 1;
      else
        *r = urlstr[i];         /* copy the & */
    }
    else
    {
      *r = urlstr[i];
    }
  }
  *r = '\0';

  return res;
}


/*************************************/
/* uvolnenie pamate po strukture URL */
/* FIXME: Translate me!              */
/*************************************/
void free_deep_url(url * urlp)
{
  if(!urlp)
    return;

  if(urlp->local_name)
  {
    url_remove_from_file_hash_tab(urlp);
    _free(urlp->local_name);
  }
  ASSERT(!urlp->local_name);

  switch (urlp->type)
  {
  case URLT_FILE:
    _free(urlp->p.file.filename);
    _free(urlp->p.file.searchstr);
    _free(urlp->p.file.anchor_name);
    break;

  case URLT_HTTP:
  case URLT_HTTPS:
    _free(urlp->p.http.host);
    _free(urlp->p.http.document);
    _free(urlp->p.http.searchstr);
    _free(urlp->p.http.anchor_name);
    _free(urlp->p.http.password);
    _free(urlp->p.http.user);
    if(urlp->status & URL_FORM_ACTION)
    {
      form_info *fi = (form_info *) urlp->extension;
      dllist *ptr;

      _free(fi->text);
      _free(fi->action);
      ptr = fi->infos;
      while(ptr)
      {
        form_field *ff = (form_field *) ptr->data;

        _free(ff->value);
        _free(ff->name);
        _free(ff);

        ptr = dllist_remove_entry(ptr, ptr);
      }
      _free(urlp->extension);
    }
    break;

  case URLT_FTP:
  case URLT_FTPS:
    _free(urlp->p.ftp.host);
    _free(urlp->p.ftp.user);
    _free(urlp->p.ftp.password);
    _free(urlp->p.ftp.anchor_name);
    _free(urlp->p.ftp.path);
    if(urlp->extension)
    {
      ftp_url_ext_free(urlp->extension);
    }
    break;

  case URLT_GOPHER:
    _free(urlp->p.gopher.host);
    _free(urlp->p.gopher.selector);
    break;

  case URLT_FROMPARENT:
  default:
    break;
  }
  _free(urlp->urlstr);

  urlp->moved_to = NULL;

  dllist_free_all(urlp->parent_url);

#ifdef WITH_TREE
#ifdef I_FACE
  _free(urlp->tree_nfo);

  if(urlp->prop)
  {
    _free(urlp->prop->type);
    _free(urlp->prop);
  }

#endif
#endif

#ifdef HAVE_MT
  pthread_mutex_destroy(&urlp->lock);
#endif
}

void cat_links_to_url_list(dllist * l1)
{
  dllist *p = l1;
  url *same;
  dllist *reg = NULL, *inl = NULL;
  int nadd = 0;
  cond_info_t condp;

  condp.level = 1;
  condp.urlnr = 0;
  condp.size = 0;
  condp.time = 0L;
  condp.mimet = NULL;
  condp.full_tag = NULL;
  condp.params = NULL;
  condp.html_doc = NULL;
  condp.html_doc_offset = 0;
  condp.tag = NULL;
  condp.attrib = NULL;

  while(p)
  {
    if(url_append_condition((url *) p->data, &condp))
    {
      url_clear_anchor((url *) p->data);
      if(NULL != (same = url_was_befor((url *) p->data)))
      {
        link_url_in_list(same, (url *) p->data);
        free_deep_url((url *) p->data);
        _free(p->data);
      }
      else
      {
        url *urlp = (url *) p->data;

        /* nadd++; */
        LOCK_TCNT;
        cfg.total_cnt++;
        UNLOCK_TCNT;

        urlp->ref_cnt = 1;

#ifdef WITH_TREE
#ifdef I_FACE
        if(cfg.xi_face)
        {
          urlp->tree_nfo = _malloc(sizeof(GUI_TREE_RTYPE));
          urlp->tree_nfo[0] = gui_tree_make_entry(urlp);
        }
#endif
#endif

        url_add_to_url_hash_tab(urlp);

        switch (cfg.scheduling_strategy)
        {
        case SSTRAT_DO_LEVEL_ORDER:
        case SSTRAT_DO_PRE_ORDER:
          reg = dllist_append(reg, (dllist_t) p->data);
          nadd++;
          break;
        case SSTRAT_DO_LEVEL_ORDER_INLINE_FIRST:
        case SSTRAT_DO_PRE_ORDER_INLINE_FIRST:
          if(urlp->status & URL_INLINE_OBJ)
            inl = dllist_append(inl, (dllist_t) urlp);
          else
            reg = dllist_append(reg, (dllist_t) urlp);
          nadd++;
          break;
        default:
          ASSERT(0);
          break;
        }

        if(cfg.hack_add_index && !url_get_search_str(urlp))
        {
          char *pom;
          char *ustr = url_to_urlstr(urlp, FALSE);

          pom = strrchr(ustr, '/');
          if(pom && pom[1])
          {
            url *nurl;

            pom[1] = '\0';
            nurl = url_parse(DBGvars(ustr));
            ASSERT(nurl->type != URLT_FROMPARENT);
            dllist_append(p, (dllist_t) nurl);
          }
          _free(ustr);
        }
      }
    }
    else
    {
      LOCK_REJCNT;
      cfg.reject_cnt++;
      UNLOCK_REJCNT;

      free_deep_url((url *) p->data);
      _free(p->data);
    }

    p = p->next;
  }
  dllist_free_all(l1);

  LOCK_CFG_URLSTACK;
  switch (cfg.scheduling_strategy)
  {
  case SSTRAT_DO_LEVEL_ORDER:
  case SSTRAT_DO_LEVEL_ORDER_INLINE_FIRST:
    if(reg || inl)
    {
      append_url_list_to_list(dllist_concat(inl, reg), NULL);
    }
    break;
  case SSTRAT_DO_PRE_ORDER:
  case SSTRAT_DO_PRE_ORDER_INLINE_FIRST:
    if(reg || inl)
    {
      append_url_list_to_list(dllist_concat(inl, reg), cfg.urlstack);
    }
    break;
  default:
    ASSERT(0);
    nadd = 0;
    break;
  }
  /* UNLOCK_CFG_URLSTACK; */
#ifdef HAVE_MT
  /* this is here for signaling sleeping downloading processes which */
  /* wait for URL to be queued inside downloading queue              */
  for(; nadd > 0; nadd--)
  {
    mt_semaphore_up(&cfg.urlstack_sem);
  }
#endif
  UNLOCK_CFG_URLSTACK;
}


void append_url_to_list(url * urlp)
{
  if(!prottable[urlp->type].supported)
  {
    xprintf(1, gettext("unsupported URL type \"%s\"\n"),
      prottable[urlp->type].urlid ? prottable[urlp->type].urlid : gettext("unknown"));
    return;
  }

  urlp->ref_cnt = 1;

#ifdef WITH_TREE
#ifdef I_FACE
  if(cfg.xi_face)
  {
    urlp->tree_nfo = _malloc(sizeof(GUI_TREE_RTYPE));
    urlp->tree_nfo[0] = gui_tree_make_entry(urlp);
  }
#endif
#endif

  url_add_to_url_hash_tab(urlp);
  LOCK_CFG_URLSTACK;
  cfg.urlstack = dllist_append(cfg.urlstack, (dllist_t) urlp);
#ifdef HAVE_MT
  mt_semaphore_up(&cfg.urlstack_sem);
#endif
  UNLOCK_CFG_URLSTACK;
  LOCK_TCNT;
  cfg.total_cnt++;
  UNLOCK_TCNT;
}

void append_url_list_to_list(dllist * list, dllist * after)
{
  if(after)
    cfg.urlstack = dllist_insert_list_after(cfg.urlstack, after, list);
  else
    cfg.urlstack = dllist_concat(cfg.urlstack, list);
}


void link_url_in_list(url * orig, url * copy)
{
  url *cpar;

  LOCK_URL(copy);
  if(copy->parent_url)
    cpar = (url *) copy->parent_url->data;
  else
    cpar = NULL;
  UNLOCK_URL(copy);

  if(cpar && (orig != cpar))
  {
    dllist *ptr;
    bool_t found = FALSE;

    if(copy->parent_url)
    {
      LOCK_URL(orig);
      for(ptr = orig->parent_url; ptr; ptr = ptr->next)
        if((url *) ptr->data == cpar)
          found = TRUE;
      UNLOCK_URL(orig);
    }

    if(!found)
    {
      LOCK_URL(orig);
      orig->ref_cnt++;
      if(cpar)
        orig->parent_url = dllist_append(orig->parent_url, (dllist_t) cpar);

#ifdef WITH_TREE
#ifdef I_FACE
      if(cfg.xi_face)
      {
        orig->tree_nfo = _realloc(orig->tree_nfo, orig->ref_cnt * sizeof(GUI_TREE_RTYPE));
        orig->tree_nfo[orig->ref_cnt - 1] = gui_tree_make_entry(orig);
      }
#endif
#endif
      UNLOCK_URL(orig);

      if(cpar && (orig->status & URL_MOVED) && (orig->status & URL_MOVED))
      {
        url *purl = orig;
        char *fn;

        while(purl->moved_to)
          purl = purl->moved_to;

        if(purl->status & URL_DOWNLOADED)
        {
          fn = url_to_filename(purl, TRUE);
          rewrite_one_parent_links(copy, cpar, fn);
        }
      }
    }
  }
}

int url_redirect_to(url * src, url * dst, int is_303)
{
  url *pomurl, *pomurl2;

  src->status |= URL_MOVED;

  url_clear_anchor(dst);
  if((pomurl = url_was_befor(dst)))
  {
    free_deep_url(dst);
    _free(dst);
    pomurl2 = pomurl;
    while(pomurl2)
    {
      if(src == pomurl2)
      {
        src->status &= ~URL_MOVED;
        return -1;
      }
      pomurl2 = pomurl2->moved_to;
    }

    LOCK_URL(pomurl);

    pomurl->parent_url = dllist_append(pomurl->parent_url, (dllist_t) src);
    pomurl->ref_cnt++;

    src->moved_to = pomurl;
    src->status |= URL_MOVED;

#ifdef WITH_TREE
#ifdef I_FACE
    if(cfg.xi_face)
    {
      pomurl->tree_nfo = _realloc(pomurl->tree_nfo, (pomurl->ref_cnt) * sizeof(GUI_TREE_RTYPE));
      pomurl->tree_nfo[pomurl->ref_cnt - 1] = gui_tree_make_entry(pomurl);
    }
#endif
#endif

    UNLOCK_URL(pomurl);

    if((pomurl->status & URL_MOVED) || (pomurl->status & URL_DOWNLOADED))
    {
      url *purl = pomurl;
      char *fn;

      xprintf(1, gettext("Moved to already processed URL.\n"));

      if(pomurl->status & URL_MOVED)
      {
        while(purl->moved_to)
          purl = purl->moved_to;
        fn = url_to_filename(purl, TRUE);
      }
      else
        fn = url_to_filename(pomurl, TRUE);

      if(cfg.rewrite_links && (purl->status & URL_DOWNLOADED))
        rewrite_parents_links(src, fn);
    }
  }
  else
  {
    dst->parent_url = dllist_append(dst->parent_url, (dllist_t) src);
    src->moved_to = dst;
    src->status |= URL_MOVED;

    if(!is_303 && !dst->extension && (src->status & URL_FORM_ACTION))
	{
      dst->extension = form_info_dup(src->extension);
	  dst->status |= URL_FORM_ACTION;
	}

#ifdef WITH_TREE
#ifdef I_FACE
    if(cfg.xi_face)
    {
      dst->tree_nfo = _malloc(sizeof(GUI_TREE_RTYPE));
      dst->tree_nfo[0] = gui_tree_make_entry(dst);
    }
#endif
#endif
    dst->ref_cnt = 1;
    url_add_to_url_hash_tab(dst);
  }
  return 0;
}

void url_add_to_url_hash_tab(url * urlp)
{
  url_clear_anchor(urlp);

  LOCK_CFG_URLHASH;
  dlhash_insert(cfg.url_hash_tbl, (dllist_t) urlp);
  UNLOCK_CFG_URLHASH;
}

void url_remove_from_url_hash_tab(url * urlp)
{
  if(!prottable[urlp->type].supported)
    return;

  LOCK_CFG_URLHASH;
  dlhash_exclude(cfg.url_hash_tbl, (dllist_t) urlp);
  UNLOCK_CFG_URLHASH;
}

void url_add_to_file_hash_tab(url * urlp)
{
  if(!prottable[urlp->type].supported)
    return;

  url_to_filename(urlp, TRUE);
}

void url_remove_from_file_hash_tab(url * urlp)
{
  if(!prottable[urlp->type].supported)
    return;

  if(urlp->local_name)
  {
    LOCK_CFG_FILEHASH;
    dlhash_exclude_exact(cfg.fn_hash_tbl, (dllist_t) urlp);
    UNLOCK_CFG_FILEHASH;
  }
}

/*
        Create a shallow copy of the url structure.
 */
url *new_url(DBGdecl(url * urlo))
{
  url *res = (url *) tld_malloc(DBGpass(sizeof(*res)));

  memcpy(res, urlo, sizeof(*res));

  return res;
}

#define isforbiddenchar(a) (NULL != strchr(FS_UNSAFE_CHARACTERS, (a)))  /* [i_a] /\\ */

static char *encode_forbiddenchars(const char *str)
{
  int size = 1;
  const char *s;
  char *res, *r;

  for(s = str; *s; ++s)
  {
    if(isforbiddenchar(*s))
      size += 2;
  }
  size += (s - str);            /* add string length */
  r = res = (char *) _malloc(size);

  for(s = str; *s; ++s)
  {
    if(isforbiddenchar(*s))
    {
      /* no buffer overflow possible here, sprintf is safe */
      sprintf(r, "%%%02x", *s);
      r += 3;
    }
    else
    {
      *(r++) = *s;
    }
  }
  *r = '\0';

  return res;
}

static char *url_get_default_local_name_real(url * urlp, int add_index)
{
  char *pom2 = NULL;
  char pbuf[50];
  char *p;

  snprintf(pbuf, sizeof(pbuf), "_%d", url_get_port(urlp));
  pbuf[sizeof(pbuf) - 1] = 0;

  switch (urlp->type)
  {
  case URLT_HTTP:
  case URLT_HTTPS:
    p = url_decode_str(urlp->p.http.document, strlen(urlp->p.http.document));
    pom2 = tl_str_concat(DBGvars(pom2), prottable[urlp->type].dirname, "/", urlp->p.http.host, pbuf, p, NULL);
    _free(p);

    if(urlp->p.http.searchstr)
    {
      /* search strings may have a / or \ inside, which must be encoded */
      char *sstr = encode_forbiddenchars(urlp->p.http.searchstr);

      pom2 = tl_str_concat(DBGvars(pom2), "?", sstr, NULL);
      _free(sstr);
    }

    if(urlp->status & URL_FORM_ACTION)
    {
      form_info *fi = (form_info *) urlp->extension;

      p = form_encode_urlencoded(fi->infos);
      if(p)
      {
        pom2 = tl_str_concat(DBGvars(pom2), (fi->method == FORM_M_POST) ? "#" : "?", p, NULL);
      }
      _free(p);
    }

    if(tl_is_dirname(pom2) && add_index)
      pom2 = tl_str_append(pom2, priv_cfg.index_name);
    break;

  case URLT_FILE:
    pom2 = url_decode_str(urlp->p.file.filename, strlen(urlp->p.file.filename));
    if(urlp->p.file.searchstr)
    {
      p = url_decode_str(urlp->p.file.searchstr, strlen(urlp->p.file.searchstr));
      pom2 = tl_str_concat(DBGvars(pom2), "?", p, NULL);
      _free(p);
    }
    break;
  case URLT_FTP:
  case URLT_FTPS:
    pom2 = tl_str_concat(DBGvars(pom2), prottable[urlp->type].dirname, "/",
      urlp->p.ftp.host, pbuf, (tl_is_dirname(pbuf) ? "" : "/"),
      urlp->p.ftp.path,
      (urlp->p.ftp.dir ? (tl_is_dirname(urlp->p.ftp.path) ? "" : "/") : NULL), (add_index ? priv_cfg.index_name : NULL), NULL);
    break;
  case URLT_GOPHER:
    pom2 = tl_str_concat(DBGvars(pom2), prottable[URLT_GOPHER].dirname, "/",
      urlp->p.gopher.host, pbuf, urlp->p.gopher.selector,
      ((urlp->p.gopher.selector[0] == '1' && add_index) ? priv_cfg.index_name : NULL), NULL);
    break;
  case URLT_FROMPARENT:
  default:
    return NULL;
  }
  return pom2;
}

char *url_get_default_local_name(url * urlp)
{
  return url_get_default_local_name_real(urlp, TRUE);
}

static char *url_get_local_name_tr(url * urlp, char *local_name, const char *mime_type, int *isdinfo)
{
  dllist *pl = priv_cfg.lfnames;
  char *ustr = url_to_urlstr(urlp, FALSE);
  char *trs;
  char *lfstr = NULL;
  char *pom2 = local_name;
  char *rv = NULL;

  while(pl)
  {
    if(lfname_match((lfname *) pl->data, ustr))
    {
      lfstr = lfname_get_by_url(urlp, ustr, local_name, mime_type, (lfname *) pl->data);
      pom2 = lfstr;
      *isdinfo = TRUE;
      break;
    }
    pl = pl->next;
  }
  _free(ustr);

  trs = tr(pom2);
  if(tl_is_dirname(trs))
  {
    rv = tl_str_concat(DBGvars(NULL), priv_cfg.cache_dir,
      (*trs == '/' ? "" : (tl_is_dirname(priv_cfg.cache_dir) ? "" : "/")),
      (tl_is_dirname(priv_cfg.cache_dir) && *trs == '/' ? trs + 1 : trs), priv_cfg.index_name, NULL);
  }
  else
  {
    rv = tl_str_concat(DBGvars(NULL), priv_cfg.cache_dir,
      (*trs == '/' ? "" : (tl_is_dirname(priv_cfg.cache_dir) ? "" : "/")),
      (tl_is_dirname(priv_cfg.cache_dir) && *trs == '/' ? trs + 1 : trs), NULL);
  }
  _free(trs);
  _free(lfstr);

  return rv;
}

/**********************************************/
char *url_get_local_name_real(url * urlp, const char *mime_type, int adj)
{
  char *pom = NULL;
  char *p1;
  char *p;
  int isdinfo = FALSE;
  struct stat estat;

  if((urlp->status & URL_ISFIRST) && priv_cfg.store_name /* && cfg.mode == MODE_SINGLE */ )
  {
    return get_abs_file_path_oss(priv_cfg.store_name);
  }

  pom = url_get_default_local_name(urlp);

  if(urlp->type != URLT_FILE)
  {
    p = url_get_local_name_tr(urlp, pom, mime_type, &isdinfo);
    _free(pom);
    pom = p;
  }

  /* In mode MIRROR we want to use exactly the same filenames as the
     remote server. Therefore we have to unquote our filename. */
  if(cfg.mode == MODE_MIRROR)
  {
    /* now we unquote the string */

    char *s = pom;
    char *t = pom;
    int hex;

    while(*s != 0)
    {
      if(s[0] == '%' && isxdigit(s[1]) && isxdigit(s[2]))
      {
        sscanf(s + 1, "%2x", &hex);
        *t++ = hex;
        s += 3;
        continue;
      }
      *t++ = *s++;
    }

    *t = 0;
  }

#ifdef FS_UNSAFE_CHARACTERS
  /* This is for automatic handling of windoze  */
  /* filesystem unsafe characters - \:*?"<>|  */
  if(urlp->type != URLT_FILE && strlen(pom) != strcspn(pom, FS_UNSAFE_CHARACTERS))
  {
    if(strchr(FS_UNSAFE_CHARACTERS, '_'))
      p = tr_del_chr(FS_UNSAFE_CHARACTERS, pom);
    else
      p = tr_chr_chr(FS_UNSAFE_CHARACTERS, "_", pom);
    _free(pom);
    pom = p;
  }
#endif

  /* adjusting of filename size if required  */
  if(urlp->type != URLT_FILE && tl_filename_needs_adjust(pom))
  {
    p = tl_adjust_filename(pom);
    _free(pom);
    pom = p;
  }

  if(!tl_lstat(pom, &estat) && S_ISDIR(estat.st_mode) && adj)
  {
    pom = tl_str_concat(DBGvars(pom), (tl_is_dirname(pom) ? "" : "/"), priv_cfg.index_name, NULL);
  }

  if((urlp->type != URLT_FILE) && cfg.base_level && !isdinfo)
  {
    const char *p2;

    p = get_abs_file_path_oss(pom);
    _free(pom);
    pom = p;
    p1 = pom + strlen(priv_cfg.cache_dir) + (tl_is_dirname(priv_cfg.cache_dir) == 0);

    if(!(p2 = strfindnchr(p1, '/', cfg.base_level)))
    {
      if((p2 = strrchr(pom, '/')))
        p2++;
    }

    if(p2)
      memmove(p1, p2, strlen(p2) + 1);
  }

  /* this is here for ensure, that we don't have directory as filename :-) */
  if(tl_is_dirname(pom))
    pom = tl_str_append(pom, priv_cfg.index_name);

  p = get_abs_file_path_oss(pom);
  _free(pom);

  return p;
}

static char *url_get_local_name(url * urlp, const char *mime_type)
{
  return url_get_local_name_real(urlp, mime_type, TRUE);
}

/******************************************************/
/* k danemu URL vytvori meno suboru v lokalnom strome */
/* FIXME: Translate me!                               */
/******************************************************/
static char *url_to_filename_real(url * urlp, const char *mime_type, int lockfn)
{
  char *p;
  bool_t inserted = FALSE;

  if(!urlp->local_name && prottable[urlp->type].supported)
  {
    p = url_get_local_name(urlp, mime_type);
    if(cfg.enable_info && urlp->type != URLT_FILE && !(urlp->status & URL_REDIRECT))
    {
      char *di;
      LOCK_GETLFNAME;
      di = dinfo_get_unique_name(urlp, p, lockfn);
      UNLOCK_GETLFNAME;
      if(di)
      {
        _free(p);
        p = di;
      }
    }
    else if(!cfg.enable_info && cfg.unique_doc && urlp->type != URLT_FILE && !(urlp->status & URL_REDIRECT))
    {
      /*** such filename have already other URL   ***/
      /*** we need to compute new unique filename ***/
      char *f;
      char *pom;
      int i;
      url *inhash;

      LOCK_CFG_FILEHASH;
      inhash = (url *) dlhash_find_by_key(cfg.fn_hash_tbl, (dllist_t) p);

      if(!inhash && !inserted)
      {
        urlp->local_name = p;
        dlhash_insert(cfg.fn_hash_tbl, (dllist_t) urlp);
        inserted = TRUE;
      }

      if(inhash && url_compare(inhash, urlp))
        inhash = NULL;

      UNLOCK_CFG_FILEHASH;

      if(inhash)
      {
        int pomlen = strlen(p) + 9;
        LOCK_GETLFNAME;
        pom = _malloc(pomlen);

        f = strrchr(p, '/');
        if(!f)
          f = "";
        else
        {
          *f = '\0';
          f++;
        }

        if(cfg.remove_before_store)
        {
          snprintf(pom, pomlen, "%s%s%s", p, (tl_is_dirname(p) ? "" : "/"), f);
          pom[pomlen - 1] = 0;
        }
        else
        {
          i = 0;
          do
          {
            i++;
            snprintf(pom, pomlen, "%s%s%03d%s", p, (tl_is_dirname(p) ? "" : "/"), i, f);
            pom[pomlen - 1] = 0;
            LOCK_CFG_FILEHASH;
            inhash = (url *) dlhash_find_by_key(cfg.fn_hash_tbl, (dllist_t) pom);
            if(!inhash && !inserted)
            {
              urlp->local_name = pom;
              dlhash_insert(cfg.fn_hash_tbl, (dllist_t) urlp);
              inserted = TRUE;
            }
            UNLOCK_CFG_FILEHASH;
          }
          while(inhash);
        }
        UNLOCK_GETLFNAME;

        _free(p);
        p = pom;
      }
    }
    if(!inserted)
    {
      LOCK_CFG_FILEHASH;
      urlp->local_name = p;
      dlhash_insert(cfg.fn_hash_tbl, (dllist_t) urlp);
      inserted = TRUE;
      UNLOCK_CFG_FILEHASH;
    }
  }
  return urlp->local_name;
}

char *url_to_filename(url * urlp, int lockfn)
{
  return url_to_filename_real(urlp, NULL, lockfn);
}

char *url_to_filename_with_type(url * urlp, const char *mime_type, int lockfn)
{
  return url_to_filename_real(urlp, mime_type, lockfn);
}

void url_set_filename(url * urlp, char *local_name)
{
  LOCK_CFG_FILEHASH;
  urlp->local_name = local_name;
  dlhash_insert(cfg.fn_hash_tbl, (dllist_t) urlp);
  UNLOCK_CFG_FILEHASH;
}

/******************************************************/
/* k danemu URL vytvori meno suboru v lokalnom strome */
/* FIXME: Translate me!                               */
/******************************************************/
void url_changed_filename(url * urlp)
{
  url_remove_from_file_hash_tab(urlp);
  _free(urlp->local_name);
  url_add_to_file_hash_tab(urlp);
}

/****************************************************************/
/* k danemu URL vytvori meno docasneho suboru v lokalnom strome */
/* FIXME: Translate me!                                         */
/****************************************************************/
char *url_to_in_filename(url * urlp)
{
  char *pom;
  char *p;

  if(cfg.mode == MODE_NOSTORE || cfg.mode == MODE_FTPDIR
    /*   || (cfg.dump_fd->fd >= 0) */ )
  {
    int pomlen = strlen(priv_cfg.cache_dir) + 50;
    pom = _malloc(pomlen);

#ifdef HAVE_MT
    snprintf(pom, pomlen, "%s%s" PAVUK_FNAME_PREFIX "pavuk_nostore_%d_%ld",
      priv_cfg.cache_dir, (tl_is_dirname(priv_cfg.cache_dir) ? "" : "/"), (int) getpid(), pthread_t2long(pthread_self()));
#else
    snprintf(pom, pomlen, "%s%s" PAVUK_FNAME_PREFIX "pavuk_nostore_%d",
      priv_cfg.cache_dir, (tl_is_dirname(priv_cfg.cache_dir) ? "" : "/"), (int) getpid());
#endif
    pom[pomlen - 1] = 0;
    return pom;
  }

  p = url_to_filename(urlp, TRUE);

  pom = _malloc(strlen(p) + strlen(PAVUK_FNAME_PREFIX) + 2);
  strcpy(pom, p);
  p = strrchr(pom, '/');
  if(!p)
    p = pom;
  else
    p++;
  memmove(p + strlen(PAVUK_FNAME_PREFIX), p, strlen(p) + 1);
  strncpy(p, PAVUK_FNAME_PREFIX, strlen(PAVUK_FNAME_PREFIX));

  return pom;
}

/************************************************/
/* make from URL structure URL string           */
/************************************************/
#undef url_to_urlstr
char *url_to_urlstr(DBGdecl(url * urlp), int with_anchor)
{
  char *p;
  char portstr[10];
  char *retv;

  snprintf(portstr, sizeof(portstr), ":%d", url_get_port(urlp));
  switch (urlp->type)
  {
  case URLT_HTTP:
  case URLT_HTTPS:
    retv = tld_malloc(DBGpass(strlen(prottable[urlp->type].typestr) +
        (urlp->p.http.user ? strlen(urlp->p.http.user) + 1 : 0) +
        (urlp->p.http.password ? strlen(urlp->p.http.password) + 1 : 0) +
        strlen(urlp->p.http.host) +
        (urlp->p.http.port ==
          prottable[urlp->type].default_port ? 0 : strlen(portstr) + 1) +
        (urlp->p.http.document ? strlen(urlp->p.http.document) : 0) +
        (urlp->p.http.searchstr ? strlen(urlp->p.http.searchstr) + 1 : 0) +
        (urlp->p.http.anchor_name ? strlen(urlp->p.http.anchor_name) + 1 : 0) + 1));

    sprintf(retv, "%s%s%s%s%s%s%s%s%s%s%s%s", prottable[urlp->type].typestr,
      (urlp->p.http.user ? urlp->p.http.user : ""),
      (urlp->p.http.password ? ":" : ""),
      (urlp->p.http.password ? urlp->p.http.password : ""),
      ((urlp->p.http.password
          || urlp->p.http.user) ? "@" : ""), urlp->p.http.host,
      (urlp->p.http.port ==
        prottable[urlp->type].default_port ? "" : portstr),
      (urlp->p.http.document ? urlp->p.http.document : ""),
      (urlp->p.http.searchstr ? "?" : ""),
      (urlp->p.http.searchstr ? urlp->p.http.searchstr : ""), ((with_anchor
          && urlp->p.http.anchor_name) ? "#" : ""), ((with_anchor && urlp->p.http.anchor_name) ? urlp->p.http.anchor_name : ""));

    if(!urlp->p.http.searchstr && (urlp->status & URL_FORM_ACTION) && (((form_info *) urlp->extension)->method == FORM_M_GET))
    {
      char *ss;

      ss = form_encode_urlencoded(((form_info *) urlp->extension)->infos);
      if(ss)
        retv = tl_str_concat(DBGvars(retv), "?", ss, NULL);
      _free(ss);
    }
    return retv;

  case URLT_FILE:
    p = get_abs_file_path(DBGpass(urlp->p.file.filename));
    retv = tld_malloc(DBGpass(strlen(prottable[URLT_FILE].typestr) +
        strlen(p) +
        (urlp->p.file.searchstr ? strlen(urlp->p.file.searchstr) + 1 : 0) +
        ((with_anchor && urlp->p.file.anchor_name) ? strlen(urlp->p.file.anchor_name) + 1 : 0) + 1));

    sprintf(retv, "%s%s%s%s%s%s", prottable[URLT_FILE].typestr, p,
      (urlp->p.file.searchstr ? "?" : ""),
      (urlp->p.file.searchstr ? urlp->p.file.searchstr : ""),
      (urlp->p.file.anchor_name ? "#" : ""), (urlp->p.file.anchor_name ? urlp->p.file.anchor_name : ""));

    _free(p);
    return retv;

  case URLT_FTP:
  case URLT_FTPS:
    retv = tld_malloc(DBGpass(strlen(prottable[urlp->type].typestr) +
        (urlp->p.ftp.user ? strlen(urlp->p.ftp.user) + 1 : 0) +
        (urlp->p.ftp.password ? strlen(urlp->p.ftp.password) + 1 : 0) +
        strlen(urlp->p.ftp.host) +
        (urlp->p.ftp.port ==
          prottable[urlp->type].default_port ? 0 : strlen(portstr) + 1) +
        strlen(urlp->p.ftp.path) + (urlp->p.ftp.anchor_name ? strlen(urlp->p.ftp.anchor_name) + 1 : 0) + 1));

    sprintf(retv, "%s%s%s%s%s%s%s%s%s%s", prottable[urlp->type].typestr,
      (urlp->p.ftp.user ? urlp->p.ftp.user : ""),
      (urlp->p.ftp.password ? ":" : ""),
      (urlp->p.ftp.password ? urlp->p.ftp.password : ""),
      ((urlp->p.ftp.password || urlp->p.ftp.user) ? "@" : ""),
      urlp->p.ftp.host,
      (urlp->p.ftp.port == prottable[urlp->type].default_port ? "" : portstr),
      urlp->p.ftp.path,
      (with_anchor && urlp->p.ftp.anchor_name ? "#" : ""), (with_anchor && urlp->p.ftp.anchor_name ? urlp->p.ftp.anchor_name : ""));

    return retv;

  case URLT_GOPHER:
    retv = tld_malloc(DBGpass(strlen(prottable[URLT_GOPHER].typestr) +
        strlen(urlp->p.gopher.host) +
        (urlp->p.gopher.port == prottable[urlp->type].default_port ? 0 : strlen(portstr) + 1) + strlen(urlp->p.gopher.selector) + 2));

    sprintf(retv, "%s%s%s/%s", prottable[URLT_GOPHER].typestr,
      urlp->p.gopher.host, (urlp->p.gopher.port == prottable[urlp->type].default_port ? "" : portstr), urlp->p.gopher.selector);

    return retv;

  case URLT_UNKNOWN:
    return tld_strdup(DBGpass(urlp->urlstr));

  case URLT_FROMPARENT:
  default:
    return NULL;
  }
}

#undef url_to_request_urlstr
char *url_to_request_urlstr(DBGdecl(url * urlp), int absolute)
{
  char *p, *s, *w, *u;
  char portstr[10];
  char *retv = NULL;

  snprintf(portstr, sizeof(portstr), ":%d", url_get_port(urlp));
  portstr[sizeof(portstr) - 1] = 0;

  switch (urlp->type)
  {
  case URLT_HTTP:
  case URLT_HTTPS:
    p = url_encode_str_safe(urlp->p.http.document, URL_PATH_UNSAFE);
    s = urlp->p.http.searchstr ? url_encode_str_safe(urlp->p.http.searchstr, URL_QUERY_UNSAFE) : NULL;

    if(absolute)
    {
      retv = tl_str_concat(DBGpass(NULL), prottable[urlp->type].typestr,
        urlp->p.http.host, (urlp->p.http.port == prottable[urlp->type].default_port ? "" : portstr), NULL);
    }
    retv = tl_str_concat(DBGpass(retv), p ? p : "", s ? "?" : "", s ? s : "", NULL);

    _free(p);
    _free(s);

    if(!urlp->p.http.searchstr && (urlp->status & URL_FORM_ACTION) && (((form_info *) urlp->extension)->method == FORM_M_GET))
    {
      char *ss;

      ss = form_encode_urlencoded(((form_info *) urlp->extension)->infos);
      if(ss)
        retv = tl_str_concat(DBGpass(retv), "?", ss, NULL);
      _free(ss);
    }
    break;
  case URLT_FTP:
  case URLT_FTPS:
    p = url_encode_str_safe(urlp->p.ftp.path, URL_PATH_UNSAFE);
    if(absolute)
    {
      w = urlp->p.ftp.password ? url_encode_str_safe(urlp->p.ftp.password, URL_AUTH_UNSAFE) : NULL;
      u = urlp->p.ftp.user ? url_encode_str_safe(urlp->p.ftp.user, URL_AUTH_UNSAFE) : NULL;

      retv = tl_str_concat(DBGpass(NULL), prottable[urlp->type].typestr,
        u ? u : "", w ? ":" : "", w ? w : "",
        (w || u) ? "@" : "", urlp->p.ftp.host, (urlp->p.ftp.port == prottable[urlp->type].default_port ? "" : portstr), NULL);

      _free(u);
      _free(w);
    }

    retv = tl_str_concat(DBGpass(retv), p, NULL);
    _free(p);
    break;
  case URLT_GOPHER:
    p = url_encode_str_safe(urlp->p.gopher.selector, URL_PATH_UNSAFE);
    if(absolute)
    {
      retv = tl_str_concat(DBGpass(NULL), prottable[urlp->type].typestr,
        urlp->p.gopher.host, (urlp->p.gopher.port == prottable[urlp->type].default_port ? "" : portstr), NULL);
    }
    retv = tl_str_concat(DBGpass(retv), "/", urlp->p.gopher.selector, NULL);
    _free(p);
    break;
  default:
    break;
  }

  return retv;
}

/********************************************************/
/* Return address of the server site for the given URL/document. */
/* Return NULL if the URL type is unsupported. */
/********************************************************/
char *url_get_site(url * urlr)
{
  switch (urlr->type)
  {
  case URLT_HTTP:
  case URLT_HTTPS:
    return urlr->p.http.host;
  case URLT_FTP:
  case URLT_FTPS:
    return urlr->p.ftp.host;
  case URLT_GOPHER:
    return urlr->p.gopher.host;
  default:
    return NULL;
  }
}

int url_get_port(url * urlr)
{
  switch (urlr->type)
  {
  case URLT_HTTP:
  case URLT_HTTPS:
    return (int) urlr->p.http.port;
  case URLT_FTP:
  case URLT_FTPS:
    return (int) urlr->p.ftp.port;
  case URLT_GOPHER:
    return (int) urlr->p.gopher.port;
  default:
    return 0;
  }
}

char *url_get_path(url * urlr)
{
  switch (urlr->type)
  {
  case URLT_HTTP:
  case URLT_HTTPS:
    return urlr->p.http.document;
  case URLT_FTP:
  case URLT_FTPS:
    return urlr->p.ftp.path;
  case URLT_GOPHER:
    return urlr->p.gopher.selector;
  case URLT_FILE:
    return urlr->p.file.filename;
  default:
    return NULL;
  }
}

void url_set_path(url * urlr, char *path)
{
  switch (urlr->type)
  {
  case URLT_HTTP:
  case URLT_HTTPS:
    _free(urlr->p.http.document);
    urlr->p.http.document = tl_strdup(path);
    break;
  case URLT_FTP:
  case URLT_FTPS:
    _free(urlr->p.ftp.path);
    urlr->p.ftp.path = tl_strdup(path);
    break;
  case URLT_GOPHER:
    _free(urlr->p.gopher.selector);
    urlr->p.gopher.selector = tl_strdup(path);
    break;
  case URLT_FILE:
    _free(urlr->p.file.filename);
    urlr->p.file.filename = tl_strdup(path);
    break;
  default:
    return;
  }
  url_changed_filename(urlr);
}

char *url_get_full_path(url * urlr)
{
  char *rv = NULL;

  switch (urlr->type)
  {
  case URLT_HTTP:
  case URLT_HTTPS:
    if(urlr->p.http.searchstr)
    {
      rv = tl_str_concat(DBGvars(NULL), urlr->p.http.document, "?", urlr->p.http.searchstr, NULL);
    }
    else
    {
      rv = tl_strdup(urlr->p.http.document);
    }
    break;
  default:
    rv = tl_strdup(url_get_path(urlr));
    break;
  }

  return rv;
}

char *url_get_pass(url * urlr, char *realm)
{
  char *pass = NULL;
  authinfo *ai;

  switch (urlr->type)
  {
  case URLT_HTTP:
  case URLT_HTTPS:
    pass = urlr->p.http.password;
    break;
  case URLT_FTP:
  case URLT_FTPS:
    pass = urlr->p.ftp.password;
    break;
  default:
    return NULL;
  }

  if(!pass)
  {
    ai = authinfo_match_entry(urlr->type, url_get_site(urlr), url_get_port(urlr), url_get_path(urlr), realm);
    if(ai)
      pass = ai->pass;
  }

  if(!pass)
  {
    pass = priv_cfg.passwd_auth;
  }

  return pass;
}

char *url_get_user(url * urlr, char *realm)
{
  char *user = NULL;
  authinfo *ai;

  switch (urlr->type)
  {
  case URLT_HTTP:
  case URLT_HTTPS:
    user = urlr->p.http.user;
    break;
  case URLT_FTP:
  case URLT_FTPS:
    user = urlr->p.ftp.user;
    break;
  default:
    return NULL;
  }

  if(!user)
  {
    ai = authinfo_match_entry(urlr->type, url_get_site(urlr), url_get_port(urlr), url_get_path(urlr), realm);
    if(ai)
      user = ai->user;
  }

  if(!user)
  {
    user = priv_cfg.name_auth;
  }

  return user;
}

int url_get_auth_scheme(url * urlr, char *realm)
{
  authinfo *ai;
  int scheme = cfg.auth_scheme;

  ai = authinfo_match_entry(urlr->type, url_get_site(urlr), url_get_port(urlr), url_get_path(urlr), realm);
  if(ai)
    scheme = ai->type;

  return scheme;
}

char *url_get_anchor_name(url * urlp)
{
  char *anchor;

  switch (urlp->type)
  {
  case URLT_HTTP:
  case URLT_HTTPS:
    anchor = urlp->p.http.anchor_name;
    break;
  case URLT_FTP:
  case URLT_FTPS:
    anchor = urlp->p.ftp.anchor_name;
    break;
  case URLT_FILE:
    anchor = urlp->p.file.anchor_name;
    break;
  default:
    anchor = NULL;
    break;
  }

  return anchor;
}

void url_clear_anchor(url * urlp)
{
  switch (urlp->type)
  {
  case URLT_HTTP:
  case URLT_HTTPS:
    _free(urlp->p.http.anchor_name);
    break;
  case URLT_FTP:
  case URLT_FTPS:
    _free(urlp->p.ftp.anchor_name);
    break;
  case URLT_FILE:
    _free(urlp->p.file.anchor_name);
    break;
  default:
    break;
  }
}

char *url_get_search_str(url * urlp)
{
  char *sstr;

  switch (urlp->type)
  {
  case URLT_HTTP:
  case URLT_HTTPS:
    sstr = urlp->p.http.searchstr;
    break;
  case URLT_FILE:
    sstr = urlp->p.file.searchstr;
    break;
  default:
    sstr = NULL;
    break;
  }

  return sstr;
}

int url_is_dir_index(url * urlp)
{
  return ((urlp->type == URLT_HTTP || urlp->type == URLT_HTTPS) &&
    tl_is_dirname(urlp->p.http.document)) || ((urlp->type == URLT_FTP || urlp->type == URLT_FTPS) && urlp->p.ftp.dir);
}

/* Check if URL is on same site. Be careful not to disallow
   protocol changes like HTTP to HTTPS. */
int url_is_same_site(url * urla, url * urlb)
{
  ASSERT(urla);
  ASSERT(urlb);
  return
    /* (urla->type == urlb->type) &&
       (url_get_port(urla) == url_get_port(urlb)) && */
    !strcmp(url_get_site(urla), url_get_site(urlb));
}

/**************************************************/
/* FIXME: Translate me                            */
/* absolutna cesta k dokumentu z lokalneho stromu */
/* ktory je referencovany relativne               */
/**************************************************/
char *get_redirect_abs_path(url * rurl, char *fstr)
{
  char *pom, *p, *p1;

  pom = tl_strdup(url_to_filename(rurl, TRUE));
  p = strrchr(pom, '/');

  p1 = _realloc(pom, strlen(fstr) + (p - pom) + 2);
  strcpy(p1 + (p - pom) + 1, fstr);

  p = get_abs_file_path_oss(p1);
  _free(p1);

  return p;
}

#undef url_path_abs
void url_path_abs(DBGdecl(url * urlp))
{
  char *p;

  switch (urlp->type)
  {
  case URLT_HTTP:
  case URLT_HTTPS:
    p = get_abs_file_path(DBGpass(urlp->p.http.document));
    _free(urlp->p.http.document);
    urlp->p.http.document = p;
    break;
  case URLT_FTP:
  case URLT_FTPS:
    p = get_abs_file_path(DBGpass(urlp->p.ftp.path));
    if(urlp->p.ftp.path[0] == '/' && urlp->p.ftp.path[1] == '/')
    {
      char *pp = tl_str_concat(DBGvars(NULL), "/", p, NULL);
      _free(p);
      p = pp;
    }
    _free(urlp->p.ftp.path);
    urlp->p.ftp.path = p;
    break;
  case URLT_FILE:
    p = get_abs_file_path(DBGpass(urlp->p.file.filename));
    _free(urlp->p.file.filename);
    urlp->p.file.filename = p;
    break;
  default:
    break;
  }
}

url *filename_to_url(char *ifn)
{
  int cdln = strlen(priv_cfg.cache_dir);
  bool_t isok = FALSE;

  if(*ifn != '/')
    return NULL;

  if(cfg.enable_info)
  {
    url *nurl = dinfo_get_url_for_filename(ifn);

    if(nurl)
      return nurl;
  }

  if(!strncmp(ifn, priv_cfg.cache_dir, cdln))
  {
    char *p;
    int i;
    url *nurl = _malloc(sizeof(*nurl));
    char *fn = tl_strdup(ifn);

    memset(nurl, 0, sizeof(*nurl));

    p = fn + cdln;
    p += (*p == '/');

    if(!strcasecmp(tl_get_extension(fn), "css"))
      nurl->status = URL_STYLE;
    else
      nurl->status = 0;

    nurl->level = 0;
    nurl->parent_url = NULL;
    nurl->moved_to = NULL;
    nurl->extension = NULL;
    nurl->local_name = (tl_is_dirname(ifn) ? tl_str_concat(DBGvars(NULL), ifn, priv_cfg.index_name, NULL) : tl_strdup(ifn));
#ifdef HAVE_MT
    pthread_mutex_init(&nurl->lock, NULL);
#endif

#ifdef WITH_TREE
#ifdef I_FACE
    nurl->prop = NULL;
    nurl->tree_nfo = NULL;
#endif
#endif

    if(cfg.base_level && cfg.default_prefix)
    {
      char *tfn, *pfn;
      url *purl = url_parse(DBGvars(priv_cfg.default_prefix));
      ASSERT(purl->type != URLT_FROMPARENT);

      pfn = url_get_default_local_name_real(purl, FALSE);
      tfn = tl_str_concat(DBGvars(NULL), priv_cfg.cache_dir,
        (tl_is_dirname(priv_cfg.cache_dir) || (*pfn == '/') ? "" : "/"), pfn, (tl_is_dirname(pfn) || (*p == '/') ? "" : "/"), p, NULL);
      _free(pfn);
      _free(fn);
      fn = tfn;

      p = fn + cdln;
      p += (*p == '/');

      free_deep_url(purl);
    }

    for(i = 0; i < NUM_ELEM(prottable); i++)
    {
      if(prottable[i].dirname &&
        !strncmp(p, prottable[i].dirname, strlen(prottable[i].dirname)) && p[strlen(prottable[i].dirname)] == '/')
      {
        isok = TRUE;
        break;
      }
    }

    if(isok)
    {
      char *p2, *p3;

      nurl->type = prottable[i].id;
      nurl->parent_url = NULL;
      p += strlen(prottable[i].dirname) + 1;

      if(!p)
      {
        _free(nurl);
        _free(fn);
        return NULL;
      }

      DEBUG_HTML("filename_to_url: p = '%s'\n", p);
      switch (nurl->type)
      {
      case URLT_HTTP:
      case URLT_HTTPS:
        nurl->p.http.password = NULL;
        nurl->p.http.user = NULL;
        nurl->p.http.anchor_name = NULL;
        nurl->p.http.searchstr = NULL;
        nurl->p.http.port = prottable[i].default_port;
        if((p2 = strchr(p, '/')))
        {
          int p2_len = strlen(p2);
          int idx_len = strlen(priv_cfg.index_name);
          char *query = NULL;

          if(idx_len <= p2_len &&
            !strcmp((p2 + p2_len - idx_len), priv_cfg.index_name) &&
            ((p2_len > idx_len && *(p2 + p2_len - idx_len - 1) == '/') || idx_len == p2_len))
          {
            *(p2 + p2_len - idx_len) = '\0';
          }

          /* for POST #query */
          p3 = strchr(p2, '#');
          if(p3)
          {
            form_info *fi;

            *p3 = '\0';
            query = p3 + 1;

            fi = _malloc(sizeof(*fi));

            fi->method = FORM_M_POST;
            fi->encoding = FORM_E_URLENCODED;
            fi->action = NULL;
            fi->text = NULL;
            fi->infos = form_parse_urlencoded_query(query);
            fi->parent_url = NULL;

            nurl->extension = fi;
            nurl->status |= URL_FORM_ACTION;
          }

          /* for query part of GET request URL */
          p3 = strchr(p2, '?');
          if(p3)
          {
            *p3 = '\0';
            nurl->p.http.searchstr = tl_strdup(p3 + 1);
          }

          nurl->p.http.document = tl_strdup(p2);
          *p2 = '\0';
          p2 = strrchr(p, '_');
          if(p2)
          {
            p2++;
            nurl->p.http.port = _atoi(p2);
            if(errno == ERANGE)
            {
              nurl->p.http.host = tl_strdup(p);
              nurl->p.http.port = prottable[i].default_port;
            }
            else
            {
              nurl->p.http.host = tl_strndup(p, p2 - p - 1);
            }
          }
          else
          {
            nurl->p.http.host = tl_strdup(p);
          }
        }
        else
        {
          _free(nurl);
          _free(fn);
          return NULL;
        }
        DEBUG_HTML("filename_to_url: http.password = '%s'\n"
          "-- http.user = '%s'\n"
          "-- http.anchor_name = '%s'\n"
          "-- http.searchstr = '%s'\n"
          "-- http.host = '%s'\n"
          "-- http.port = %d\n", nurl->p.http.password, nurl->p.http.user,
          nurl->p.http.anchor_name, nurl->p.http.searchstr, nurl->p.http.host, nurl->p.http.port);
        break;

      case URLT_GOPHER:
        nurl->p.gopher.port = prottable[i].default_port;
        if((p2 = strchr(p, '/')))
        {
          int p2_len = strlen(p2);
          int idx_len = strlen(priv_cfg.index_name);

          p2++;

          if(idx_len <= p2_len &&
            !strcmp((p2 + p2_len - idx_len), priv_cfg.index_name) &&
            ((p2_len > idx_len && *(p2 + p2_len - idx_len - 1) == '1') || idx_len == p2_len))
          {
            *(p2 + p2_len - idx_len) = '\0';
          }
          nurl->p.gopher.selector = tl_strdup(p2);
          *p2 = '\0';
          p2 = strrchr(p, '_');
          if(p2)
          {
            p2++;
            nurl->p.gopher.port = _atoi(p2);
            if(errno == ERANGE)
            {
              nurl->p.gopher.host = tl_strdup(p);
              nurl->p.gopher.port = prottable[i].default_port;
            }
            else
            {
              nurl->p.gopher.host = tl_strndup(p, p2 - p - 1);
            }
          }
          else
          {
            nurl->p.gopher.host = tl_strdup(p);
          }
        }
        else
        {
          _free(nurl);
          _free(fn);
          return NULL;
        }
        DEBUG_HTML("filename_to_url: gopher.host = '%s'\n" "-- gopher.port = %d\n", nurl->p.gopher.host, nurl->p.gopher.port);
        break;

      case URLT_FTP:
      case URLT_FTPS:
        nurl->p.ftp.port = prottable[i].default_port;
        nurl->p.ftp.password = NULL;
        nurl->p.ftp.user = NULL;
        nurl->p.ftp.dir = FALSE;
        nurl->p.ftp.anchor_name = NULL;
        if((p2 = strchr(p, '/')))
        {
          int p2_len = strlen(p2);
          int idx_len = strlen(priv_cfg.index_name);

          if(idx_len <= p2_len &&
            !strcmp((p2 + p2_len - idx_len), priv_cfg.index_name) &&
            ((p2_len > idx_len && *(p2 + p2_len - idx_len - 1) == '/') || idx_len == p2_len))
          {
            *(p2 + p2_len - idx_len) = '\0';
            nurl->p.ftp.dir = TRUE;
          }
          nurl->p.ftp.path = tl_strdup(p2);
          *p2 = '\0';
          p2 = strrchr(p, '_');
          if(p2)
          {
            p2++;
            nurl->p.ftp.port = _atoi(p2);
            if(errno == ERANGE)
            {
              nurl->p.ftp.host = tl_strdup(p);
              nurl->p.ftp.port = prottable[i].default_port;
            }
            else
            {
              nurl->p.ftp.host = tl_strndup(p, p2 - p - 1);
            }
          }
          else
          {
            nurl->p.ftp.host = tl_strdup(p);
          }
        }
        else
        {
          _free(nurl);
          _free(fn);
          return NULL;
        }
        DEBUG_HTML("filename_to_url: ftp.password = '%s'\n"
          "-- ftp.user = '%s'\n"
          "-- ftp.anchor_name = '%s'\n"
          "-- ftp.host = '%s'\n"
          "-- ftp.port = %d\n", nurl->p.ftp.password, nurl->p.ftp.user, nurl->p.ftp.anchor_name, nurl->p.ftp.host, nurl->p.ftp.port);
        break;

      default:
        _free(nurl);
        nurl = NULL;
        break;
      }
      _free(fn);
      return nurl;
    }
    _free(nurl);
  }
  return NULL;
}

/*
  Return a pointer to the matching url as it is stored in the
  url hash table (cfg.url_hash_tbl).

  Return NULL if the specified url could not be located in
  the hash table.
 */
url *url_was_befor(url * urlp)
{
  url *ret;

  if(!prottable[urlp->type].supported)
    return NULL;
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  /*
     rescan pages by simply playing dumb: we didn't see this page before.
     --> rescan!
   */
  if(cfg.hammer_mode == 0 && cfg.hammer_endcount > 0)
    return NULL;
#endif

  LOCK_CFG_URLHASH;
  ret = (url *) dlhash_find(cfg.url_hash_tbl, (dllist_t) urlp);
  UNLOCK_CFG_URLHASH;

  return ret;
}

void url_forget_filename(url * urlp)
{
  if(cfg.enable_info && cfg.post_update)
    dinfo_remove(urlp->local_name);
  url_remove_from_file_hash_tab(urlp);
  _free(urlp->local_name);
}

int dllist_url_compare(dllist_t key1, dllist_t key2)
{
  return url_compare((url *) key1, (url *) key2);
}

int url_compare(url * u1, url * u2)
{
  int rv;

  if(u1->type != u2->type)
    return 0;

  switch (u1->type)
  {
  case URLT_HTTP:
  case URLT_HTTPS:
    if((rv = strcmp(u1->p.http.document, u2->p.http.document)))
      return !rv;

    if(u1->p.http.searchstr && u2->p.http.searchstr)
      rv = strcmp(u1->p.http.searchstr, u2->p.http.searchstr);
    else
      rv = u1->p.http.searchstr - u2->p.http.searchstr;

    if(rv)
      return !rv;

    if(u1->p.http.user && u2->p.http.user)
      rv = strcmp(u1->p.http.user, u2->p.http.user);
    else
      rv = u1->p.http.user - u2->p.http.user;

    if(rv)
      return !rv;

    if(u1->p.http.password && u2->p.http.password)
      rv = strcmp(u1->p.http.password, u2->p.http.password);
    else
      rv = u1->p.http.password - u2->p.http.password;

    if(rv)
      return !rv;

    if((rv = strcmp(u1->p.http.host, u2->p.http.host)))
      return !rv;

    if(u1->p.http.port != u2->p.http.port)
      return FALSE;

    if((u1->status & URL_FORM_ACTION) != (u2->status & URL_FORM_ACTION))
      return FALSE;

    if((u1->status & URL_FORM_ACTION) && (u2->status & URL_FORM_ACTION))
    {
      dllist *ptr;
      form_info *fi1 = (form_info *) u1->extension;
      form_info *fi2 = (form_info *) u2->extension;

      if(fi1->method != fi2->method)
        return FALSE;
      if(fi1->encoding != fi2->encoding)
        return FALSE;

      ptr = fi1->infos;
      while(ptr)
      {
        if(!dllist_find2(fi2->infos, ptr->data, form_field_compare))
          return FALSE;
        ptr = ptr->next;
      }
    }

    return TRUE;

  case URLT_FTP:
  case URLT_FTPS:
    if((rv = strcmp(u1->p.ftp.path, u2->p.ftp.path)))
      return !rv;

    if(u1->p.ftp.user && u2->p.ftp.user)
      rv = strcmp(u1->p.ftp.user, u2->p.ftp.user);
    else
      rv = u1->p.ftp.user - u2->p.ftp.user;

    if(rv)
      return !rv;

    if(u1->p.ftp.password && u2->p.ftp.password)
      rv = strcmp(u1->p.ftp.password, u2->p.ftp.password);
    else
      rv = u1->p.ftp.password - u2->p.ftp.password;

    if(rv)
      return !rv;

    if((rv = strcmp(u1->p.ftp.host, u2->p.ftp.host)))
      return !rv;

    return u1->p.ftp.port == u2->p.ftp.port;

  case URLT_GOPHER:
    if((rv = strcmp(u1->p.gopher.selector, u2->p.gopher.selector)))
      return !rv;

    if((rv = strcmp(u1->p.gopher.host, u2->p.gopher.host)))
      return !rv;

    return u1->p.gopher.port == u2->p.gopher.port;

  case URLT_FILE:
    if((rv = strcmp(u1->p.file.filename, u2->p.file.filename)))
      return !rv;

    if(u1->p.file.searchstr && u2->p.file.searchstr)
      rv = strcmp(u1->p.file.searchstr, u2->p.file.searchstr);
    else
      rv = u1->p.file.searchstr - u2->p.file.searchstr;

    return !rv;

  default:
    break;
  }
  return 0;
}

url_info *url_info_new(char *urlstr)
{
  url_info *ui;

  ui = _malloc(sizeof(*ui));
  memset(ui, 0, sizeof(*ui));
  ui->urlstr = tl_strdup(urlstr);
  ui->type = URLI_NORMAL;
  ui->fields = NULL;
  ui->cookies = NULL;
  ui->encoding = FORM_E_UNKNOWN;
  ui->method = FORM_M_GET;
  ui->localname = NULL;

  return ui;
}

void url_info_free(url_info * ui)
{
  dllist *ptr;

  if(!ui)
    return;

  _free(ui->urlstr);

#if 0
  if(ui->type == URLI_FORM)
#endif
  {
    for(ptr = ui->fields; ptr; ptr = dllist_remove_entry(ptr, ptr))
    {
      form_field *fi = (form_field *) ptr->data;

      _free(fi->name);
      _free(fi->value);
      _free(fi);
    }
  }
  cookie_free_all(&ui->cookies);
  _free(ui->localname);
  _free(ui);
}

static const struct
{
  enum
  {
    _RQF_URL,
    _RQF_METHOD,
    _RQF_ENCODING,
    _RQF_FIELD,
    _RQF_FILE,
    _RQF_COOKIE,
    _RQF_LOCALNAME
  } type;
  char *str;
} _request_fields[] =
{
  {
  _RQF_URL, "URL:"},
  {
  _RQF_METHOD, "METHOD:"},
  {
  _RQF_ENCODING, "ENCODING:"},
  {
  _RQF_FIELD, "FIELD:"},
  {
  _RQF_FILE, "FILE:"},
  {
  _RQF_COOKIE, "COOKIE:"},
  {
  _RQF_LOCALNAME, "LNAME:"}
};

url_info *url_info_parse(const char *str, bool_t try_me)
{
  url_info *ui;
  const char *p;
  const char *tp;
  int l = 0;
  bool_t err = FALSE;
  bool_t found = FALSE;
  int i;

  ui = url_info_new(NULL);
  ui->type = URLI_FORM;

  p = str;
  while(!err && *p)
  {
    char quotechr = 0;

    p += strspn(p, " \t");
    if(!*p)
      break;                    /* [i_a] tolerate trailing whitespace */

    found = FALSE;
    for(i = 0; i < NUM_ELEM(_request_fields); i++)
    {
      if(!strncasecmp(p, _request_fields[i].str, strlen(_request_fields[i].str)))
      {
        found = TRUE;
        p += strlen(_request_fields[i].str);
        if(*p == '\"')
        {
          quotechr = '\"';
          p++;
          l = strcspn(p, "\"");
        }
        else if(*p == '\'')
        {
          quotechr = '\'';
          p++;
          l = strcspn(p, "\'");
        }
        else
          l = strcspn(p, " \t");
        if(!l)
          err = TRUE;

        break;
      }
    }
    if(err || !found)
    {
      err = TRUE;
      break;
    }
    switch (_request_fields[i].type)
    {
    case _RQF_URL:
      {
        url *urlp;

        ui->urlstr = tl_strndup(p, l);
        urlp = url_parse(DBGvars(ui->urlstr));
        ASSERT(urlp->type != URLT_FROMPARENT);
        _free(ui->urlstr);
        ui->urlstr = url_to_urlstr(DBGvars(urlp), FALSE);
        free_deep_url(urlp);
        _free(urlp);
      }
      break;

    case _RQF_LOCALNAME:
      {
        char *tmp = tl_strndup(p, l);

        ui->localname = get_abs_file_path_oss(tmp);
        _free(tmp);
      }
      break;

    case _RQF_METHOD:
      if(!strncasecmp(p, "GET", l))
      {
        ui->method = FORM_M_GET;
      }
      else if(!strncasecmp(p, "POST", l))
      {
        ui->method = FORM_M_POST;
      }
      else
      {
        err = TRUE;
      }
      break;

    case _RQF_ENCODING:
      if(!strncasecmp(p, "m", l))
      {
        ui->encoding = FORM_E_MULTIPART;
      }
      else if(!strncasecmp(p, "u", l))
      {
        ui->encoding = FORM_E_URLENCODED;
      }
      else
      {
        err = TRUE;
      }
      break;

    case _RQF_COOKIE:
      {
        cookie_entry *cookie;

        cookie = cookie_parse(p, NULL);
        if(!cookie)
        {
          xprintf(1, gettext("Cannot parse COOKIE: '%s'\n"), p);
        }
        else
        {
          cookie->next = ui->cookies;
          if(ui->cookies)
            ui->cookies->prev = cookie;
          ui->cookies = cookie;
        }
      }
      break;

    case _RQF_FIELD:
    case _RQF_FILE:
      {
        form_field *fi;

        fi = _malloc(sizeof(*fi));

        fi->name = NULL;
        fi->value = NULL;

        switch (_request_fields[i].type)
        {
        case _RQF_FILE:
          fi->type = FORM_T_FILE;
          break;

        default:
        case _RQF_FIELD:
          fi->type = FORM_T_TEXT;
          break;
        }

        tp = strchr(p, '=');

        if(!tp || (tp - p) > l)
        {
          err = TRUE;
        }
        else
        {
          fi->name = form_decode_urlencoded_str(p, tp - p);
          fi->value = form_decode_urlencoded_str(tp + 1, l - (tp - p + 1));
          if(fi->type == FORM_T_TEXT && strchr(fi->value, '\n'))
            fi->type = FORM_T_TEXTAREA;
        }
        if(err || !fi->name || !fi->value)
        {
          _free(fi->value);
          _free(fi->name);
          _free(fi);
        }
        else
        {
          ui->fields = dllist_append(ui->fields, (dllist_t) fi);
        }
      }
      break;
    }
    p += l;
    if(quotechr)
      p += (*p == quotechr);
  }

  if(!err)
  {
    if(!ui->urlstr)
    {
      if(!try_me)
      {
        xprintf(1, gettext("Missing specification of URL in request\n"));
      }
      err = TRUE;
    }

#if 0                           /* sometimes we need also empty forms */
    if(!ui->fields && ui->method == FORM_M_GET)
    {
      ui->type = URLI_NORMAL;
    }
    else if(!ui->fields)
    {
      if(!try_me)
      {
        xprintf(1, gettext("Missing request fields specification for POST request\n"));
      }
      err = TRUE;
    }
#endif

    if(ui->method == FORM_M_GET && ui->encoding == FORM_E_MULTIPART)
    {
      /* [i_a] do NOT include 'if (!try_me)' condition around this one */
      xprintf(1, gettext("Multipart encoding not supported with GET requests\n"));
      err = TRUE;
    }
  }

  /* fixup cookies */
  if(!err)
  {
    url *urlp;

    ASSERT(ui->urlstr);
    urlp = url_parse(DBGvars(ui->urlstr));
    ASSERT(urlp);
    if(urlp->type != URLT_HTTP && urlp->type != URLT_HTTPS)
    {
      if(ui->cookies)
      {
        xprintf(1, gettext("COOKIEs are only supported for HTTP/HTTPS type URLs\n"));
        err = TRUE;
      }
    }
    else
    {
      cookie_entry *centry;

      for(centry = ui->cookies; centry; centry = centry->next)
      {
        VERIFY(!fixup_cookie(centry, urlp));
      }
    }

    free_deep_url(urlp);
    _free(urlp);
  }

  if(err)
  {
    url_info_free(ui);
    ui = NULL;
  }

  return ui;
}

char *url_info_dump(url_info * ui)
{
  char *retv = NULL;
  dllist *ptr;
  cookie_entry *cptr;

  retv = tl_str_concat(DBGvars(retv), "URL:\"", ui->urlstr, "\" ", NULL);

  if(ui->localname)
    retv = tl_str_concat(DBGvars(retv), "LNAME:\"", ui->localname, "\" ", NULL);

  if(ui->type == URLI_FORM)
  {
    if(ui->method == FORM_M_GET)
      retv = tl_str_append(retv, "METHOD:GET ");
    else if(ui->method == FORM_M_POST)
      retv = tl_str_append(retv, "METHOD:POST ");

    if(ui->encoding == FORM_E_URLENCODED)
      retv = tl_str_append(retv, "ENCODING:u ");
    if(ui->encoding == FORM_E_MULTIPART)
      retv = tl_str_append(retv, "ENCODING:m ");
  }

  ptr = ui->fields;
  while(ptr)
  {
    char *n, *v;
    form_field *ff = (form_field *) ptr->data;

    n = form_encode_urlencoded_str(ff->name);
    v = form_encode_urlencoded_str(ff->value);

    if(ff->type == FORM_T_FILE)
      retv = tl_str_concat(DBGvars(retv), "FILE:\"", n, "=", v, "\" ", NULL);
    else
      retv = tl_str_concat(DBGvars(retv), "FIELD:\"", n, "=", v, "\" ", NULL);

    _free(n);
    _free(v);
    ptr = ptr->next;
  }

  cptr = ui->cookies;
  while(cptr)
  {
    char *v = cvt_cookie_to_setstr(cptr);
    ASSERT(cptr);
    ASSERT(cptr->name);
    ASSERT(cptr->value);
    ASSERT(v);
    retv = tl_str_concat(DBGvars(retv), "COOKIE:\"", v, "\" ", NULL);

    _free(v);
    cptr = cptr->next;
  }

  return retv;
}

url_info *url_info_duplicate(url_info * ui)
{
  url_info *cui;
  dllist *ptr;
  cookie_entry *cptr;

  cui = url_info_new(ui->urlstr);
  if(ui->localname)
    cui->localname = tl_strdup(ui->localname);
  cui->method = ui->method;
  cui->encoding = ui->encoding;
  cui->type = ui->type;

  ptr = ui->fields;
  while(ptr)
  {
    form_field *ff = (form_field *) ptr->data;
    form_field *cff = (form_field *) _malloc(sizeof(*cff));

    cff->type = ff->type;
    cff->name = tl_strdup(ff->name);
    cff->value = tl_strdup(ff->value);

    cui->fields = dllist_append(cui->fields, (dllist_t) cff);

    ptr = ptr->next;
  }
  cptr = ui->cookies;
  while(cptr)
  {
    cookie_entry *c = dup_cookie(cptr);

    c->next = cui->cookies;
    if(cui->cookies)
      cui->cookies->prev = c;
    cui->cookies = c;

    cptr = cptr->next;
  }
  return cui;
}
