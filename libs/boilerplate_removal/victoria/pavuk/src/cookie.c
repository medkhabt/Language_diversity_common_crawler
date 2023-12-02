/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "tools.h"
#include "url.h"
#include "times.h"
#include "cookie.h"
#include "bufio.h"

static int cookies_num = 0;
static cookie_entry *cookies = NULL;
static cookie_entry *last_cookie = NULL;
static int cookie_changed = 0;
static time_t cookie_updated = 0L;
enum CookieFileType
{
  COOKIEFILETYPE_NS = 0,
  COOKIEFILETYPE_KDE2 = 1
};
enum CookieFileType cookiefiletype = COOKIEFILETYPE_NS;

void cookie_deep_free(cookie_entry ** centryp)
{
  cookie_entry *centry;
  if(centryp && *centryp)
  {
    centry = *centryp;
    _free(centry->domain);
    _free(centry->path);
    _free(centry->name);
    _free(centry->value);
    _free(centry);
    *centryp = NULL;
  }
}

void cookie_free_all(cookie_entry ** centryp)
{
  cookie_entry *pce;
  cookie_entry *centry;

  if(centryp && *centryp)
  {
    centry = *centryp;
    ASSERT(centry ? !centry->prev : TRUE);
    for(pce = centry; pce;)
    {
      cookie_entry *next = pce->next;
      cookie_deep_free(&pce);
      pce = next;
    }
    *centryp = NULL;
  }
}

cookie_entry *dup_cookie(cookie_entry * src)
{
  cookie_entry *dst = _malloc(sizeof(*dst));
  memset(dst, 0, sizeof(*dst));

  if(src->domain)
    dst->domain = tl_strdup(src->domain);
  if(src->host)
    dst->host = tl_strdup(src->host);
  if(src->path)
    dst->path = tl_strdup(src->path);
  dst->expires = src->expires;
  dst->secure = src->secure;
  dst->flag = src->flag;
  if(src->name)
    dst->name = tl_strdup(src->name);
  if(src->value)
    dst->value = tl_strdup(src->value);
  dst->loaded = src->loaded;

  return dst;
}

static cookie_entry *cookie_get_last_entry(cookie_entry * centry, int *num)
{
  if(num)
    *num = 0;

  if(!centry)
    return NULL;

  if(num)
    (*num)++;
  while(centry->next)
  {
    centry = centry->next;
    if(num)
      (*num)++;
  }

  return centry;
}

static void cookie_remove_oldest_entry(cookie_entry ** centry, cookie_entry ** lastentry, int *num)
{
  cookie_entry *last = NULL;

  if(*lastentry)
    last = *lastentry;
  else
    last = cookie_get_last_entry(*centry, NULL);

  if(last && last->prev)
  {
    if(*centry == last)
      (*centry) = last->next;
    if(last->prev)
      last->prev->next = last->next;
    if(last->next)
      last->next->prev = last->prev;
    if(*lastentry)
      (*lastentry) = last->prev;
    cookie_deep_free(&last);
    if(num)
      (*num)--;
  }
}

static int cookie_match(url * urlp, cookie_entry * cookiep)
{
  int retv = TRUE;
  int cl, hl;
  char *c, *h;

  retv &= (urlp->type == URLT_HTTP) || (urlp->type == URLT_HTTPS);

  if(cookiep->secure)
    retv &= (urlp->type == URLT_HTTPS);

  if(cookiep->path)
  {
    retv &= !strncmp(urlp->p.http.document, cookiep->path, strlen(cookiep->path));
    DEBUG_COOKIE("cookie_match: %d <- doc: '%s' =?= '%s' (len: %d)\n", retv,
      urlp->p.http.document, cookiep->path, strlen(cookiep->path));
  }

  c = cookiep->domain;
  if(c)
  {
    h = urlp->p.http.host;
    cl = strlen(c);
    hl = strlen(h);

    if(cl == hl + 1 && cookiep->domain[0] == '.')
      ++c;
    else if(hl > cl)
      h += hl - cl;

    retv &= !strcasecmp(c, h);
    DEBUG_COOKIE("cookie_match: %d <- domain: '%s' =?= '%s'\n", retv, c, h);
  }
  return retv;
}

static int cookie_eq(cookie_entry * c1, cookie_entry * c2)
{
  int retv = !strcmp(c1->name, c2->name);
  if(c1->domain && c2->domain)
  {
    retv &= !strcmp(c1->domain, c2->domain);
  }
  else
  {
    retv &= (c1->domain == c2->domain);
  }
  if(c1->path && c2->path)
  {
    retv &= !strcmp(c1->path, c2->path);
  }
  else
  {
    retv &= (c1->path == c2->path);
  }
  return retv;
}

static cookie_entry *cookie_parse_ns(char *line)
{
  cookie_entry *centry;
  char *p, *strtokbuf;
  int i;
  char *ln;


  if(!line)
    return NULL;

  ln = tl_strdup(line);

  centry = _malloc(sizeof(*centry));

  centry->loaded = TRUE;
  centry->next = NULL;
  centry->prev = NULL;
  centry->path = NULL;
  centry->domain = NULL;
  centry->name = NULL;
  centry->value = NULL;
  centry->host = NULL;

  p = strtokc_r(ln, '\t', &strtokbuf);

  for(i = 0; i < 7; i++)
  {
    if(!p)
    {
      cookie_deep_free(&centry);
      _free(ln);
      return NULL;
    }

    switch (i)
    {
    case 0:
      ASSERT(*p);
      centry->domain = tl_strdup(p);
      break;
    case 1:
      ASSERT(*p);
      centry->flag = (bool_t) strcasecmp(p, "FALSE");
      break;
    case 2:
      ASSERT(*p);
      centry->path = tl_strdup(p);
      break;
    case 3:
      ASSERT(*p);
      centry->secure = (bool_t) strcasecmp(p, "FALSE");
      break;
    case 4:
      ASSERT(*p);
      centry->expires = _atoi(p);
      break;
    case 5:
      ASSERT(*p);
      centry->name = tl_strdup(p);
      break;
    case 6:
      ASSERT(*p);
      centry->value = tl_strdup(p);
      break;
    }
    p = strtokc_r(NULL, '\t', &strtokbuf);
  }

  _free(ln);
  return centry;
}

static cookie_entry *cookie_parse_kde2(char *line)
{
  cookie_entry *centry;
  int i;
  char *p, *ln, *next;

  if(!line)
    return NULL;

  next = ln = tl_strdup(line);

  centry = _malloc(sizeof(*centry));

  centry->loaded = TRUE;
  centry->next = NULL;
  centry->prev = NULL;
  centry->path = NULL;
  centry->domain = NULL;
  centry->name = NULL;
  centry->value = NULL;
  centry->host = NULL;

  for(i = 0; i < 8 && *next; ++i)
  {
    p = next;
    while(*p == ' ' || *p == '\t')
      ++p;                      /* skip white space */
    if(*p == '\"')
    {
      next = ++p;
      while(*next && *next != '\"')
        ++next;
    }
    else
    {
      next = p;
      while(*next && *next != ' ' && *next != '\t')
        ++next;
    }
    if(*next)
    {
      *(next++) = 0;
    }
    if(p == next)
    {
      break;
    }

    switch (i)
    {
    case 0:
      ASSERT(*p);
      centry->host = tl_strdup(p);
      break;
    case 1:
      ASSERT(*p);
      centry->domain = tl_strdup(p);
      break;
    case 2:
      ASSERT(*p);
      centry->path = tl_strdup(p);
      break;
    case 3:
      ASSERT(*p);
      centry->expires = _atoi(p);
      break;
    case 4:
      ASSERT(*p);
      centry->flag = _atoi(p);
      break;
    case 5:
      ASSERT(*p);
      centry->name = tl_strdup(p);
      break;
    case 6:
      ASSERT(*p);
      centry->secure = _atoi(p) ^ 4;
      break;
    case 7:
      ASSERT(*p);
      centry->value = tl_strdup(p);
      break;
    }
  }

  if(i < 7 || *next)
  {
    cookie_deep_free(&centry);
    ASSERT(centry == NULL);
  }
  _free(ln);

  return centry;
}

static cookie_entry *cookie_read_file_real(bufio * fd)
{
  char line[BUFIO_ADVISED_READLN_BUFSIZE];
  cookie_entry *centry = NULL;
  cookie_entry *retv = NULL;
  cookie_entry *prev = NULL;

  while(bufio_readln(fd, line, sizeof(line)) > 0)
  {
    if(line[0] == '#')
    {
      if(!strncmp("# KDE Cookie File v2", line, 20))
      {
        cookiefiletype = COOKIEFILETYPE_KDE2;
      }
      continue;
    }
    if(cookiefiletype == COOKIEFILETYPE_KDE2 && line[0] == '[')
      continue;

    strip_nl(line);

    if(*line)
    {
      switch (cookiefiletype)
      {
      case COOKIEFILETYPE_NS:
        centry = cookie_parse_ns(line);
        break;
      case COOKIEFILETYPE_KDE2:
        centry = cookie_parse_kde2(line);
        break;
      }

      if(centry)
      {
        if(!retv)
        {
          retv = centry;
        }
        else
        {
          prev->next = centry;
          centry->prev = prev;
        }
        prev = centry;
      }
      else
      {
        xprintf(1, gettext("Unable to parse : %s\n"), line);
      }
    }
  }

  return retv;
}

int cookie_read_file(const char *filename)
{
  bufio *fd;
  cookie_entry *retv = NULL;

  LOCK_COOKIES;
  cookie_free_all(&cookies);

  if(!(fd = bufio_open(filename, O_BINARY | O_RDONLY)))
  {
    xperror(mk_native(filename));
    UNLOCK_COOKIES;
    return -1;
  }

  if(bufio_flock(fd, FALSE))
  {
    xperror(mk_native(filename));
    UNLOCK_COOKIES;
    return -2;
  }

  retv = cookie_read_file_real(fd);

  cookie_updated = time(NULL);

  bufio_funlock(fd);
  bufio_close(fd);
  fd = NULL;

  cookies = retv;
  ASSERT(cookies ? !cookies->prev : TRUE);
  last_cookie = cookie_get_last_entry(cookies, &cookies_num);
  ASSERT(last_cookie ? !last_cookie->next : TRUE);
  UNLOCK_COOKIES;
  return 0;
}

static int cookie_write_file(bufio * fd)
{
  cookie_entry *centry;
  time_t t = time(NULL);

  ASSERT(fd->fd >= 0);
  bufio_truncate(fd, 0);
  bufio_seek(fd, 0, SEEK_SET);
  /* bufio_reset(fd); */

  for(centry = cookies; centry; centry = centry->next)
  {
    if(!centry->expires || centry->expires > t)
    {
      ASSERT(centry->domain);
      ASSERT(centry->path);
      bufio_printf(fd, "%s\t%s\t%s\t%s\t%ld\t%s\t%s\n",
        centry->domain,
        centry->flag ? "TRUE" : "FALSE",
        centry->path,
        centry->secure ? "TRUE" : "FALSE",
        centry->expires, centry->name, centry->value);
    }
  }

  return 0;
}

static void cookie_sync(cookie_entry * centry)
{
  cookie_entry *cmem;
  cookie_entry *cfile;
  cookie_entry **uentry;
  cookie_entry *p;
  bool_t found;
  time_t t = time(NULL);

  ASSERT(cookies ? !cookies->prev : TRUE);
  ASSERT(last_cookie ? !last_cookie->next : TRUE);
  ASSERT(centry ? !centry->prev : TRUE);
  for(cfile = centry; cfile; cfile = p)
  {
    found = FALSE;
    p = cfile->next;

    for(cmem = cookies, uentry = &cookies; cmem; uentry = &cmem->next, cmem = cmem->next)
    {
      ASSERT(cfile);
      if(cookie_eq(cfile, cmem))
      {
        found = TRUE;
        if((cmem->expires && cmem->expires < t && !cmem->loaded)
          || (cmem->loaded && !strcmp(cmem->value, cfile->value)))
        {
          cookie_deep_free(&cfile);
        }
        else
        {
          *uentry = cfile;
          cfile->next = cmem->next;
          cfile->prev = cmem->prev;
          if(cmem->next)
            cmem->next->prev = cfile;
          cookie_deep_free(&cmem);
        }
        break;
      }
    }

    if(!found)
    {
      ASSERT(cfile);
      cfile->next = cookies;
      cfile->prev = NULL;
      if(cookies)
        cookies->prev = cfile;
      cookies = cfile;
      cookies_num++;
      if(cookies_num && cookies_num > cfg.cookies_max)
      {
        cookie_remove_oldest_entry(&cookies, &last_cookie, &cookies_num);
        ASSERT(cookies ? !cookies->prev : TRUE);
        ASSERT(last_cookie ? !last_cookie->next : TRUE);
      }
    }
  }
}

int cookie_update_file(int force)
{
  struct stat estat;
  cookie_entry *centry = NULL;
  bufio *fd;

  if(!cookie_changed)
    return 0;

  if(!force && cookie_changed < 10)
    return 0;

  if(!cfg.cookie_file)
    return -1;

  if(cookiefiletype != COOKIEFILETYPE_NS)
  {
    xprintf(1, gettext("Cookie file format not supported for writing.\n"));
    return -2;
  }

  LOCK_COOKIES;
  xprintf(1, gettext("Updating cookie file\n"));

  fd = bufio_copen(cfg.cookie_file, O_BINARY | O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
  if(!fd)
  {
    xperror(mk_native(cfg.cookie_file));
    UNLOCK_COOKIES;
    return -1;
  }

  if(bufio_flock(fd, FALSE))
  {
    xperror(mk_native(cfg.cookie_file));
    bufio_close(fd);
    fd = NULL;
    UNLOCK_COOKIES;
    return -1;
  }

  if(fstat(bufio_getfd(fd), &estat))
  {
    xperror(mk_native(cfg.cookie_file));
    bufio_funlock(fd);
    bufio_close(fd);
    fd = NULL;
    UNLOCK_COOKIES;
    return -1;
  }

  if(estat.st_mtime > cookie_updated)
  {
    xprintf(1, gettext("Cookie file has changed -> synchronizing\n"));
    centry = cookie_read_file_real(fd);
    cookie_sync(centry);
  }

  bufio_reset(fd);              /* MANDATORY to make sure read buffer doesn't flow into write buffer! */
  cookie_write_file(fd);

  cookie_changed = 0;
  cookie_updated = time(NULL);

  bufio_funlock(fd);
  bufio_close(fd);
  fd = NULL;

  UNLOCK_COOKIES;
  return 0;
}

char *cookie_get_field(url * urlp)
{
  int sel = 0;
  char *retv;
  cookie_entry *centry;
  time_t t = time(NULL);

  if(urlp->type != URLT_HTTP && urlp->type != URLT_HTTPS)
    return NULL;

  retv = tl_strdup("Cookie:");

  LOCK_COOKIES;
  ASSERT(cookies ? !cookies->prev : TRUE);
  ASSERT(last_cookie ? !last_cookie->next : TRUE);
  for(centry = cookies; centry; centry = centry->next)
  {
    DEBUG_COOKIE("Check cookie '%s' for GET/POST to URL, value = '%s'\n", centry->name, centry->value);

    if((!centry->expires || centry->expires > t) && cookie_match(urlp, centry))
    {
      retv = _realloc(retv, strlen(retv) + strlen(centry->name) + strlen(centry->value) + 6);

      if(!sel)
        strcat(retv, " ");
      else
        strcat(retv, "; ");

      strcat(retv, centry->name);
      strcat(retv, "=");
      strcat(retv, centry->value);

      sel++;
    }
  }
  UNLOCK_COOKIES;

  if(sel)
    strcat(retv, "\r\n");
  else
    _free(retv);

  return retv;
}

cookie_entry *cookie_parse(const char *field, url * urlp)
{
  char *p;
  char *p1;
  char *strtokbuf;
  char *pom = tl_strdup(field);
  cookie_entry *centry;

  centry = _malloc(sizeof(*centry));
  memset(centry, 0, sizeof(*centry));

  centry->next = NULL;
  centry->prev = NULL;
  centry->loaded = FALSE;
  centry->domain = NULL;
  centry->flag = TRUE;
  centry->path = NULL;
  centry->expires = 0;
  centry->secure = FALSE;
  centry->name = NULL;
  centry->value = NULL;

  p = strtokc_r(pom, ';', &strtokbuf);
  while(p)
  {
    while(tl_ascii_isspace(*p))
      p++;

    if(!strncasecmp(p, "expires=", 8))
    {
      centry->expires = scntime(p + 8, FALSE);
#if defined(HAVE_DEBUG_FEATURES)
	  if(cfg.debug && (DEBUG_COOKIE_BITMASK & cfg.debug_level))
	  {
        LOCK_TIME;
	    DEBUG_COOKIE("cookie expiry date decoded: '%s' (original timestamp text: '%s')\n", 
		    ctime(&centry->expires), p + 8);
        UNLOCK_TIME;
	  }
#endif
	  if (centry->expires == (time_t)-1)
	  {
        xprintf(1, "ERROR: cannot decode cookie date/time stamp '%s'\n", p + 8);
		centry->expires = scntime(p + 8, TRUE);
	  }
    }
    else if(!strncasecmp(p, "path=", 5))
    {
      centry->path = tl_strdup(p + 5);
    }
    else if(!strncasecmp(p, "domain=", 7))
    {
      centry->domain = tl_strdup(p + 7);
    }
    else if(!strcasecmp(p, "secure"))
    {
      centry->secure = TRUE;
    }
    else
    {
      p1 = strchr(p, '=');
      if(p1)
      {
        _free(centry->name);    /* FIXED: multiple assignment -- memory leak */
        _free(centry->value);   /* FIXED: multiple assignment -- memory leak */
        centry->name = tl_strndup(p, (p1 - p));
        centry->value = tl_strdup(p1 + 1);
      }
    }

    p = strtokc_r(NULL, ';', &strtokbuf);
  }

  _free(pom);

  if(!centry->value || !centry->name)
  {
    cookie_deep_free(&centry);
    ASSERT(centry == NULL);
  }

  if(centry)
  {
    const char **pp;

    if(urlp && fixup_cookie(centry, urlp))
    {
      cookie_deep_free(&centry);
      return NULL;
    }

    for(pp = priv_cfg.cookies_disabled_domains; pp && *pp; pp++)
    {
      if(!strcasecmp(*pp, centry->domain))
      {
        xprintf(1, gettext("Removing cookie from disabled domain %s\n"), centry->domain);
        cookie_deep_free(&centry);
        return NULL;
      }
    }
  }
  return centry;
}



int fixup_cookie(cookie_entry * centry, url * urlp)
{
  ASSERT(centry);
  ASSERT(urlp);

  if(!centry->path)
    centry->path = tl_strdup(url_get_path(urlp));

  if(!centry->domain)
    centry->domain = tl_strdup(url_get_site(urlp));
  lowerstr(centry->domain);

    /*** check if cookie is set for source domain ***/
  if(cfg.cookie_check_domain)
  {
    char *p = url_get_site(urlp);
    int plen = strlen(p);
    int slen = strlen(centry->domain);
    if(plen < slen || strcmp(centry->domain, p + (plen - slen)))
    {
      xprintf(1, gettext("Server %s is trying to set cookie for %s domain\n"), p, centry->domain);
      return -1;
    }
  }
  return 0;
}

void store_cookie_in_global_store(cookie_entry * centrysrc)
{
  cookie_entry *centry;
  cookie_entry *pcentry;
  cookie_entry *uentry;
  bool_t found = FALSE;

  ASSERT(centrysrc);
  centry = dup_cookie(centrysrc);
  LOCK_COOKIES;
  ASSERT(cookies ? !cookies->prev : TRUE);
  ASSERT(last_cookie ? !last_cookie->next : TRUE);
  for(pcentry = cookies, uentry = NULL; pcentry; uentry = pcentry, pcentry = pcentry->next)
  {
    if(cookie_eq(centry, pcentry))
    {
      DEBUG_COOKIE("cookie_parse: found cookie '%s'\n", centry->name);
      found = TRUE;
      if(centry->expires && centry->expires < time(NULL))
      {
        DEBUG_COOKIE("cookie_parse: update expiry date for cookie '%s'\n", centry->name);
        pcentry->expires = centry->expires;
        cookie_deep_free(&centry);
        cookie_changed++;
        ASSERT(centry == NULL);
        break;
      }
      else
      {
        if(centry->expires != pcentry->expires || strcmp(centry->value, pcentry->value))
        {
          DEBUG_COOKIE("cookie_parse: overwrite cookie '%s'\n", centry->name);

          if(uentry)
            uentry->next = centry;
          else
            cookies = centry;
          ASSERT(cookies ? !cookies->prev : TRUE);
          centry->next = pcentry->next;
          centry->prev = uentry;
          if(centry->next)
            centry->next->prev = centry;
          if(last_cookie == pcentry)
            last_cookie = centry;
          ASSERT(last_cookie ? !last_cookie->next : TRUE);
          cookie_deep_free(&pcentry);
          cookie_changed++;
          break;
        }
      }
    }
  }
  if(!found)
  {
    if(centry->expires && centry->expires < time(NULL))
    {
      DEBUG_COOKIE("cookie_parse: cookie '%s' is already expired!\n", centry->name);

      cookie_deep_free(&centry);
    }
    else
    {
      DEBUG_COOKIE("cookie_parse: add cookie '%s'\n", centry->name);

      centry->next = cookies;
      if(cookies)
        cookies->prev = centry;
      if(!last_cookie)
        last_cookie = centry;
      cookies = centry;
      ASSERT(cookies ? !cookies->prev : TRUE);
      ASSERT(last_cookie ? !last_cookie->next : TRUE);
      cookie_changed++;
      cookies_num++;
      if(cfg.cookies_max && cookies_num > cfg.cookies_max)
      {
        DEBUG_COOKIE("cookie_parse: erase oldest cookie\n");
        cookie_remove_oldest_entry(&cookies, &last_cookie, &cookies_num);
        ASSERT(cookies ? !cookies->prev : TRUE);
        ASSERT(last_cookie ? !last_cookie->next : TRUE);
      }
    }
  }
  UNLOCK_COOKIES;
}



char *cvt_cookie_to_setstr(cookie_entry * centry)
{
  char *retv = NULL;

  ASSERT(centry);
  if(centry->expires)
  {
    char *p;
    LOCK_TIME;
    p = ctime(&centry->expires);
    retv = tl_str_concat(DBGvars(retv), "expires=", p, ";", NULL);
    UNLOCK_TIME;                /* [i_a] */
    ASSERT(retv);
  }
  if(centry->path)
  {
    retv = tl_str_concat(DBGvars(retv), (retv ? " " : ""), "path=", centry->path, ";", NULL);
    ASSERT(retv);
  }
  if(centry->domain)
  {
    retv = tl_str_concat(DBGvars(retv), (retv ? " " : ""), "domain=", centry->domain, ";", NULL);
    ASSERT(retv);
  }
  if(centry->secure)
  {
    retv = tl_str_concat(DBGvars(retv), (retv ? " " : ""), "secure", ";", NULL);
    ASSERT(retv);
  }
  if(centry->name)
  {
    ASSERT(centry->value);
    retv = tl_str_concat(DBGvars(retv), (retv ? " " : ""), centry->name, "=", (centry->value ? centry->value : ""), ";", NULL);
    ASSERT(retv);
  }
  ASSERT(retv);
  return retv;
}


void cleanup_cookies(void)
{
  cookie_free_all(&cookies);
}
