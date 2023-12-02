/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "config.h"
#include "tools.h"
#include "ftp.h"
#include "http.h"
#include "gopher.h"
#include "authinfo.h"
#include "times.h"
#include "html.h"
#include "lfname.h"
#include "re.h"
#include "mopt.h"
#include "jstrans.h"
#include "tag_pattern.h"


#ifdef HAVE_REGEX
#define __CONDITIONS "F or R"
#else
#define __CONDITIONS "F"
#endif


static void cfg_version_info(void);
static int cfg_load_scenario(const char *);
static char *cfg_get_option_string(cfg_param_t *param, int type);

#include "options.h"

struct strategy_mapt
{
  strategy id;
  char    *name;
  char    *label;
};

static struct strategy_mapt strategy_map[] =
{
  { SSTRAT_DO_LEVEL_ORDER, "level", gettext_nop("Level order") },
  { SSTRAT_DO_LEVEL_ORDER_INLINE_FIRST, "leveli",
    gettext_nop("Level order, inline first") },
  { SSTRAT_DO_PRE_ORDER, "pre", gettext_nop("Pre order") },
  { SSTRAT_DO_PRE_ORDER_INLINE_FIRST, "prei",
    gettext_nop("Pre order, inline first") },
};

static strategy get_strategy_by_str(char *str)
{
  int i;

  for (i = 0; i < SSTRAT_LAST; i++)
  {
    if (!strcasecmp(str, strategy_map[i].name))
      return strategy_map[i].id;
  }

  return SSTRAT_LAST;
}

static char *get_strategy_str(strategy id)
{
  return strategy_map[id].name;
}

char *get_strategy_label(strategy id)
{
  return gettext(strategy_map[id].label);
}

static const struct
{
  char *name;
  int   id;
} _ssl_versions[] =
{
  {
    "ssl23", 1
  },
  {
    "ssl2", 2
  },
  {
    "ssl3", 3
  },
  {
    "tls1", 4
  }
};

static strategy get_ssl_version_by_str(char *str)
{
  int i;

  for (i = 0; i < NUM_ELEM(_ssl_versions); i++)
  {
    if (!strcasecmp(str, _ssl_versions[i].name))
      return _ssl_versions[i].id;
  }

  return -1;
}

static char *get_ssl_version_str(strategy id)
{
  return _ssl_versions[id - 1].name;
}

/*
 * Print the available HELP info for the option specified (when a
 * valid 'param' is passed to this function)
 *
 * OR
 *
 * Print the available HELP info for ALL options (when a NULL
 * pointer is passed instead).
 *
 * This is useful for 'targeted help', i.e. providing help descriptions
 * which are targeted at specific command line options.
 *
 * NOTE:
 * When a command line option is specified, which does NOT come
 * with its own help text, this routine scans the option list to
 * find a matching option which DOES have a help text.
 * This feature enables pavuk to print some suitable help text for
 * both -noXXX and -XXX command line options.
 *
 * NOTE:
 * When 'param' is valid but NO help can be found for it, no
 * help will be printed whatsoever.
 */
void show_targeted_usage(cfg_param_t *param)
{
  int i;
  int j;
  int param_with_help = -1;

  if (param)
  {
    if (param->help)
    {
      param_with_help = param - params;
      if (param_with_help < 0
          || param_with_help >= NUM_ELEM(params))
      {
        param_with_help = -1;
      }
    }

    if (param_with_help < 0)
    {
      /*
       * Find matching param which comes WITH a help text!
       */
      for (i = 0; i < NUM_ELEM(params); i++)
      {
        if (params[i].help)
        {
          if (param
              && param->val_adr == params[i].val_adr
              && param->mval_adr == params[i].mval_adr)
          {
            param_with_help = i;
            break;
          }
        }
      }
    }

    /*
     * If no help could be found, make sure no help whatsoever is printed!
     */
    if (param_with_help < 0)
    {
      param_with_help = NUM_ELEM(params);             /* place 'marker' at and out of range position */
    }
  }

  for (i = 0; i < NUM_ELEM(params); i++)
  {
    if (param_with_help >= 0
        && param_with_help != i)
      continue;/* skip non-targeted options/help */

    if (params[i].help)
    {
      xprintf(0, gettext(params[i].help));

      if (params[i].type & PARAM_UNSUPPORTED)
      {
        xprintf(0, gettext("!! This option is NOT supported by this pavuk build/version. !!"));
        continue;
      }

      if (params[i].type & PARAM_FOREIGN)
      {
        continue;
      }

      switch (params[i].type)
      {
      default:
        break;

      case PARAM_DEBUGL:
        xprintf(0, gettext("\n                            Supported levels:\n"));

        for (j = 0; cfg_debug_levels[j].id; j++)
        {
          xprintf(0, "                              %-10s %s\n",
                  cfg_debug_levels[j].option,
                  (cfg_debug_levels[j].label ? cfg_debug_levels[j].label : ""));
        }
        break;
      }
    }
  }
}

/**********************************/
/* show program usage information */
/**********************************/
void usage(void)
{
	char *pname;

  cfg.bgmode = FALSE;

  pname = pavuk_get_program_name();
  xprintf(0,
          gettext("Usage:  %s  [options]  [any number of URLS]\npavuk-%s %s\n"),
          pname, VERSION, HOSTTYPE);
  _free(pname);

  show_targeted_usage(NULL);

  exit(PAVUK_EXIT_OK);
}

void usage_short(cfg_param_t *param)
{
  char *pname;

  xprintf(0, "%s %s %s %s\n", PACKAGE, VERSION, REVISION, HOSTTYPE);
  pname = pavuk_get_program_name();
  xprintf(0, gettext("Type \"%s --help\" for long help\n\n"), pname);
  _free(pname);

  if (param)
  {
    /* do NOT show extra help if it cannot be targeted! */
    show_targeted_usage(param);
  }

  exit(PAVUK_EXIT_CFG_ERR);
}

static void cfg_version_info(void)
{
  xprintf(0, "%s %s %s %s\n", PACKAGE, VERSION, REVISION, HOSTTYPE);
  xprintf(0, gettext("Optional features available :\n"));
#ifdef DEBUG
  xprintf(0, gettext(" - Developer Debug Build (with extra run-time checks)\n"));
#endif

#ifdef HAVE_DEBUG_FEATURES
  xprintf(0, gettext(" - Debug features\n"));
#endif

#ifdef GETTEXT_NLS
  xprintf(0, gettext(" - GNU gettext internationalization of messages\n"));
#endif

#ifdef HAVE_FLOCK
  xprintf(0, gettext(" - flock() document locking\n"));
#elif (defined (WIN32) || defined (_WIN32))
  xprintf(0, gettext(" - Win32 flock() document locking\n"));
#endif

#ifdef HAVE_FCNTL_LOCK
  xprintf(0, gettext(" - fcntl() document locking\n"));
#endif

#ifdef I_FACE
#ifdef GTK_FACE
  xprintf(0, gettext(" - Gtk GUI interface\n"));
#endif
#ifdef WITH_TREE
  xprintf(0, gettext(" - URL tree preview\n"));
#endif
#endif

#ifdef USE_SSL
  xprintf(0, gettext(" - HTTP and FTP over SSL\n"));

#if defined (USE_SSL_IMPL_OPENSSL) && defined (OPENSSL)
#define __SSLIMP "OpenSSL"
#elif defined (USE_SSL_IMPL_OPENSSL)
#define __SSLIMP "SSLeay"
#elif defined (USE_SSL_IMPL_NSS)
#define __SSLIMP "NSS3"
#else
#define __SSLIMP "unknown"
#endif
  xprintf(0, gettext(" - SSL layer implemented with %s library\n"), __SSLIMP);
#endif

#ifdef SOCKS
  xprintf(0, gettext(" - Socks proxy support\n"));
#endif
#ifdef HAVE_FSTATFS
  xprintf(0, gettext(" - filesystem free space checking\n"));
#endif

#ifdef HAVE_REGEX
  xprintf(0, gettext(" - optional regex patterns in -fnrules and -*rpattern options\n"));
#endif

#ifdef HAVE_POSIX_REGEX
  xprintf(0, gettext(" - POSIX regexp\n"));
#endif

#ifdef HAVE_V8_REGEX
  xprintf(0, gettext(" - Bell V8 regexp\n"));
#endif

#ifdef HAVE_BSD_REGEX
  xprintf(0, gettext(" - BSD regexp\n"));
#endif

#ifdef HAVE_GNU_REGEX
  xprintf(0, gettext(" - GNU regexp\n"));
#endif

#ifdef HAVE_PCRE_REGEX
  xprintf(0, gettext(" - PCRE regexp\n"));
#endif

#ifdef HAVE_TRE_REGEX
  xprintf(0, gettext(" - TRE regexp\n"));
#endif

#ifdef HAVE_BDB_18x
  xprintf(0, gettext(" - support for loading files from Netscape browser cache\n"));
#endif

#ifdef HAVE_TERMIOS
  xprintf(0, gettext(" - support for detecting whether pavuk is running as background job\n"));
#endif

#ifdef HAVE_MT
  xprintf(0, gettext(" - multithreading support\n"));
#endif

#ifdef ENABLE_NTLM
  xprintf(0, gettext(" - NTLM authorization support\n"));
#endif

#ifdef HAVE_MOZJS
  xprintf(0, gettext(" - JavaScript bindings\n"));
#endif

#ifdef HAVE_INET6
  xprintf(0, gettext(" - IPv6 support\n"));
#endif

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  xprintf(0, gettext(" - DoS support (a.k.a. 'chunky' a.k.a. 'hammer modes')\n"));
#endif

  exit(PAVUK_EXIT_OK);
}

static int htmltag_set_disabled(char *tagstr, int disable)
{
  int i, j;
  bool_t tfound, afound;
  char *tag;
  char *attrib;
  char *pom, *strtokbuf;

  if (!strcasecmp(tagstr, "all"))
  {
    for (i = 0; i < html_link_tags_num(); i++)
    {
      for (j = 0; html_link_tags[i].attribs[j].attrib; j++)
      {
        if (disable)
          html_link_tags[i].attribs[j].stat |= LINK_DISABLED;
        else
          html_link_tags[i].attribs[j].stat &= ~LINK_DISABLED;
      }
    }

    return -1;
  }

  pom = tl_strdup(tagstr);
  tag = strtokc_r(pom, ',', &strtokbuf);
  attrib = strtokc_r(NULL, ';', &strtokbuf);

  while (tag)
  {
    tfound = FALSE;
    afound = FALSE;
    for (i = 0; i < html_link_tags_num(); i++)
    {
      if (!strcasecmp(html_link_tags[i].tag, tag))
      {
        tfound = TRUE;
        for (j = 0; html_link_tags[i].attribs[j].attrib; j++)
        {
          if (attrib && *attrib)
          {
            if (!strcasecmp(html_link_tags[i].attribs[j].attrib, attrib))
            {
              afound = TRUE;
              if (disable)
                html_link_tags[i].attribs[j].stat |= LINK_DISABLED;
              else
                html_link_tags[i].attribs[j].stat &= ~LINK_DISABLED;
              break;
            }
          }
          else
          {
            afound = TRUE;
            if (disable)
              html_link_tags[i].attribs[j].stat |= LINK_DISABLED;
            else
              html_link_tags[i].attribs[j].stat &= ~LINK_DISABLED;
          }
        }
        break;
      }
    }
    if (!(tfound && afound))
    {
      xprintf(0, gettext("HTML tag not supported : %s.%s\n"), tag, attrib ? attrib : "(null)");
    }

    tag = strtokc_r(NULL, ',', &strtokbuf);
    attrib = strtokc_r(NULL, ';', &strtokbuf);
  }

  _free(pom);
  return -1;
}

static void cfg_set_to_default(cfg_param_t *cpar)
{
  char **p;
  int x, j;

  if (cpar->type & PARAM_UNSUPPORTED)
    return;

  if (cpar->type & PARAM_FOREIGN)
    return;

  switch (cpar->type)
  {
  default:
    ASSERT(!"Unsupported param type");
    break;

  case PARAM_FNAME:
    {
      char *p = (char *)cpar->default_val.p;
      bufio *fdp = *((bufio **)cpar->val_adr);
      if (fdp)
      {
        fdp = bufio_new(!!fdp->mutex, fdp->buf_size);
        bufio_close(*((bufio **)cpar->val_adr));
        *((bufio **)cpar->val_adr) = NULL;
      }
      else
      {
        fdp = bufio_new(FALSE, 0);
      }
      *((bufio **)cpar->val_adr) = fdp;

      if (p)
      {
        fdp->flags = ((*p == '@') ? BUFIO_F_APPEND : 0);
        ASSERT(!fdp->filename);
        fdp->filename = tl_strdup(p + (*p == '@'));
      }
      else
      {
        fdp->flags = 0;
        fdp->filename = NULL;
        ASSERT(!p);
      }
      fdp->fd = -1;
    }
    break;

  case PARAM_FD:
    {
      bufio *fdp = *((bufio **)cpar->val_adr);
      if (fdp)
      {
        /*
         * Trick: note that the new bufio will use a buffer
         * with a size IDENTICAL to the buffer size of the previously
         * active bufio.
         */
        fdp = bufio_new(!!fdp->mutex, fdp->buf_size);
        bufio_close(*((bufio **)cpar->val_adr));
        *((bufio **)cpar->val_adr) = NULL;
      }
      else
      {
        fdp = bufio_new(FALSE, 0);
      }
      *((bufio **)cpar->val_adr) = fdp;

      ASSERT(!fdp->filename);
      fdp->filename = NULL;
      fdp->flags = BUFIO_F_APPEND;
      fdp->fd = cpar->default_val.l;
    }
    break;

  case PARAM_NUM:
  case PARAM_BYTE_NUM:
    *((long *)cpar->val_adr) = cpar->default_val.l;
    break;

  case PARAM_PBOOL:
  case PARAM_NBOOL:
    *((bool_t *)cpar->val_adr) = (bool_t)cpar->default_val.l;
    break;

  case PARAM_PORT_RANGE:
    *((long *)cpar->val_adr) = cpar->default_val.l;
    *((long *)cpar->mval_adr) = cpar->mdefault_val.l;
    break;

  case PARAM_PATH:
    _free(*((char **)cpar->val_adr));
#if defined (__CYGWIN__) || defined (WIN32) || defined (__WIN32)
    *((char **)cpar->val_adr) = cvt_win32_to_unix_path((char *)cpar->default_val.p, FALSE);
#else
    *((char **)cpar->val_adr) = get_abs_file_path_oss((char *)cpar->default_val.p);
#endif
    break;

  case PARAM_PATH_NC:
  case PARAM_STR:
  case PARAM_PASS:
    _free(*((char **)cpar->val_adr));
    ASSERT(! * ((char **)cpar->val_adr));
    if (cpar->default_val.p)
    {
      *((char **)cpar->val_adr) = tl_strdup(cpar->default_val.p);
    }
    break;

  case PARAM_STRLIST:
  case PARAM_STRLIST_EX:
    for (p = *((char ***)cpar->val_adr); p && *p; p++)
      _free(*p);
    _free(*(char ***)cpar->val_adr);

    *((char ***)cpar->val_adr) = (char **)cpar->default_val.p;
    if (cpar->mval_adr)
      *((bool_t *)cpar->mval_adr) = (bool_t)cpar->mdefault_val.l;
    break;

  case PARAM_CONN:
    _free(*((char **)cpar->val_adr));
    if (cpar->default_val.p)
    {
      *((char **)cpar->val_adr) = tl_strdup(cpar->default_val.p);
    }
    if (cpar->mval_adr)
    {
      *((long *)cpar->mval_adr) = cpar->mdefault_val.l;
    }
    break;

  case PARAM_AUTHSCH:
    *((long *)cpar->val_adr) = cpar->default_val.l;
    break;

  case PARAM_MODE:
    *((long *)cpar->val_adr) = cpar->default_val.l;
    break;

  case PARAM_TIME:
    *((time_t *)cpar->val_adr) = (time_t)0;
    break;

  case PARAM_HTMLTAG:
    for (x = 0; x < html_link_tags_num(); x++)
      for (j = 0; html_link_tags[x].attribs[j].attrib; j++)
        html_link_tags[x].attribs[j].stat &= ~LINK_DISABLED;
    break;

  case PARAM_TWO_QSTR:
    _free(*((char **)cpar->val_adr));
    _free(*((char **)cpar->mval_adr));
    ASSERT(! * ((char **)cpar->val_adr));
    ASSERT(! * ((char **)cpar->mval_adr));
    if (cpar->default_val.p)
    {
      *((char **)cpar->val_adr) = tl_strdup(cpar->default_val.p);
    }
    if (cpar->mdefault_val.p)
    {
      *((char **)cpar->mval_adr) = tl_strdup(cpar->mdefault_val.p);
    }
    break;

  case PARAM_DOUBLE:
    *((double *)cpar->val_adr) = ((double)cpar->default_val.l) / 1000.0;
    break;

  case PARAM_LFNAME:
    while (cfg.lfnames)
    {
      lfname_free((lfname *)cfg.lfnames->data);
      cfg.lfnames = dllist_remove_entry(cfg.lfnames, cfg.lfnames);
    }
    break;

  case PARAM_RE:
#ifdef HAVE_REGEX
    {
      dllist *ptr = *((dllist **)cpar->val_adr);

      *((dllist **)cpar->val_adr) = NULL;

      while (ptr)
      {
        re_free((re_entry *)ptr->data);
        ptr = dllist_remove_entry(ptr, ptr);
      }
    }
#endif
    break;

  case PARAM_USTRAT:
    *((strategy *)cpar->val_adr) = (strategy)cpar->default_val.l;
    break;

  case PARAM_SSLVER:
    *((long *)cpar->val_adr) = cpar->default_val.l;
    break;

  case PARAM_HTTPHDR:
    {
      dllist *ptr = *(dllist **)cpar->val_adr;
      *(dllist **)cpar->val_adr = NULL;

      while (ptr)
      {
        httphdr_free((httphdr *)ptr->data);
        ptr = dllist_remove_entry(ptr, ptr);
      }
    }
    break;

  case PARAM_DEBUGL:
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  case PARAM_HAMMERFLAG:
#endif
    *((unsigned long *)cpar->val_adr) = (unsigned long)cpar->default_val.l;
    break;

  case PARAM_REQUEST:
    {
      dllist *ptr = *(dllist **)cpar->val_adr;
      *(dllist **)cpar->val_adr = NULL;
      while (ptr)
      {
        url_info_free((url_info *)ptr->data);
        ptr->data = NULL;
        ptr = dllist_remove_entry(ptr, ptr);
      }
    }
    break;

  case PARAM_TRANSPARENT:
    {
      if (cpar->val_adr)
      {
        http_proxy *pr = *((http_proxy **)cpar->val_adr);
        if (pr)
          http_proxy_free(pr);
      }
    }
    break;

  case PARAM_PROXY:
    {
      dllist *ptr = *((dllist **)cpar->val_adr);
      *((dllist **)cpar->val_adr) = NULL;

      while (ptr)
      {
        http_proxy *pr = (http_proxy *)ptr->data;
        http_proxy_unref(pr);
        ptr = dllist_remove_entry(ptr, ptr);
      }
    }
    break;

  case PARAM_FUNC:
    break;

  case PARAM_JSTRANS:
#ifdef HAVE_REGEX
    while (cfg.js_transform)
    {
      js_transform_free((js_transform_t *)cfg.js_transform->data);
      cfg.js_transform = dllist_remove_entry(cfg.js_transform, cfg.js_transform);
    }
#endif
    break;

  case PARAM_NUMLIST:
    {
      dllist *ptr = *((dllist **)cpar->val_adr);
      *((dllist **)cpar->val_adr) = NULL;
      while (ptr)
        ptr = dllist_remove_entry(ptr, ptr);
      if (cpar->mval_adr)
        *((bool_t *)cpar->mval_adr) = (bool_t)cpar->mdefault_val.l;
    }
    break;

  case PARAM_FTPHS:
    {
      dllist *ptr = *((dllist **)cpar->val_adr);
      *((dllist **)cpar->val_adr) = NULL;
      for ( ; ptr; ptr = dllist_remove_entry(ptr, ptr))
        ftp_handshake_info_free((ftp_handshake_info *)ptr->data);
    }
    break;

  case PARAM_TAGPAT:
    {
      dllist *ptr = *((dllist **)cpar->val_adr);
      *((dllist **)cpar->val_adr) = NULL;
      for ( ; ptr; ptr = dllist_remove_entry(ptr, ptr))
        tag_pattern_free((tag_pattern_t *)ptr->data);
    }
    break;
  }
}

void cfg_set_all_to_default(void)
{
  int i;

  init_mime_types();

  for (i = 0; i < NUM_ELEM(params); i++)
    cfg_set_to_default(&params[i]);
}

void cfg_setup_default(void)
{
  int i, x, j;

  init_mime_types();

  for (i = 0; i < NUM_ELEM(params); i++)
  {
    if (params[i].type & PARAM_UNSUPPORTED)
      continue;

    if (params[i].type & PARAM_FOREIGN)
      continue;

    switch (params[i].type)
    {
    default:
      ASSERT(!"Unhandled param type");
      break;

    case PARAM_FNAME:
      {
        char *p = (char *)params[i].default_val.p;
        bufio *fdp = *((bufio **)params[i].val_adr);
        ASSERT(fdp);
        if (p)
        {
          fdp->flags = ((*p == '@') ? BUFIO_F_APPEND : 0);
          ASSERT(!fdp->filename);
          fdp->filename = tl_strdup(p + (*p == '@'));
        }
        else
        {
          ASSERT(!fdp->filename);
          fdp->filename = NULL;
          fdp->flags = 0;
        }
        fdp->fd = -1;
      }
      break;

    case PARAM_FD:
      {
        bufio *fdp = *((bufio **)params[i].val_adr);
        ASSERT(fdp);
        ASSERT(!fdp->filename);
        fdp->filename = NULL;
        fdp->flags = BUFIO_F_APPEND;
        fdp->fd = params[i].default_val.l;
      }
      break;

    case PARAM_NUM:
    case PARAM_BYTE_NUM:
      *((long *)params[i].val_adr) = params[i].default_val.l;
      break;

    case PARAM_PBOOL:
    case PARAM_NBOOL:
      *((bool_t *)params[i].val_adr) = (bool_t)params[i].default_val.l;
      break;

    case PARAM_PORT_RANGE:
      *((long *)params[i].val_adr) = params[i].default_val.l;
      *((long *)params[i].mval_adr) = params[i].mdefault_val.l;
      break;

    case PARAM_PATH:
#if defined (__CYGWIN__) || defined (WIN32) || defined (__WIN32)
      *((char **) params[i].val_adr) = cvt_win32_to_unix_path((char *) params[i].default_val.p, FALSE);
#else
      *((char **)params[i].val_adr) = get_abs_file_path_oss((char *)params[i].default_val.p);
#endif
      break;

    case PARAM_PATH_NC:
    case PARAM_STR:
    case PARAM_PASS:
      *((char **)params[i].val_adr) = NULL;
      if (params[i].default_val.p)
      {
        *((char **)params[i].val_adr) = tl_strdup(params[i].default_val.p);
      }
      break;

    case PARAM_STRLIST:
    case PARAM_STRLIST_EX:
      *((char ***)params[i].val_adr) = (char **)params[i].default_val.p;
      if (params[i].mval_adr)
        *((bool_t *)params[i].mval_adr) = (bool_t)params[i].mdefault_val.l;
      break;

    case PARAM_CONN:
      *((char **)params[i].val_adr) = (char *)params[i].default_val.p;
      if (params[i].mval_adr)
        *((long *)params[i].mval_adr) = params[i].mdefault_val.l;
      break;

    case PARAM_AUTHSCH:
      *((long *)params[i].val_adr) = params[i].default_val.l;
      break;

    case PARAM_MODE:
      *((long *)params[i].val_adr) = params[i].default_val.l;
      break;

    case PARAM_TIME:
      *((time_t *)params[i].val_adr) = (time_t)0;
      break;

    case PARAM_HTMLTAG:
      for (x = 0; x < html_link_tags_num(); x++)
        for (j = 0; html_link_tags[x].attribs[j].attrib; j++)
          html_link_tags[x].attribs[j].stat &= ~LINK_DISABLED;
      break;

    case PARAM_TWO_QSTR:
      *((char **)params[i].val_adr) = NULL;
      *((char **)params[i].mval_adr) = NULL;
      if (params[i].default_val.p)
      {
        *((char **)params[i].val_adr) = tl_strdup(params[i].default_val.p);
      }
      if (params[i].mdefault_val.p)
      {
        *((char **)params[i].mval_adr) = tl_strdup(params[i].mdefault_val.p);
      }
      break;

    case PARAM_DOUBLE:
      *((double *)params[i].val_adr) = ((double)params[i].default_val.l) / 1000.0;
      break;

    case PARAM_LFNAME:
      cfg.lfnames = NULL;
      break;

    case PARAM_RE:
#ifdef HAVE_REGEX
      *((dllist **)params[i].val_adr) = NULL;
#endif
      break;

    case PARAM_USTRAT:
      *((strategy *)params[i].val_adr) = (strategy)params[i].default_val.l;
      break;

    case PARAM_SSLVER:
      *((long *)params[i].val_adr) = params[i].default_val.l;
      break;

    case PARAM_HTTPHDR:
      *((dllist **)params[i].val_adr) = NULL;
      break;

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
    case PARAM_HAMMERFLAG:
#endif
    case PARAM_DEBUGL:
      *((unsigned long *)params[i].val_adr) = (unsigned long)params[i].default_val.l;
      break;

    case PARAM_REQUEST:
      *((dllist **)params[i].val_adr) = NULL;
      break;

    case PARAM_TRANSPARENT:
      *((http_proxy **)params[i].val_adr) = NULL;
      break;

    case PARAM_PROXY:
      *((dllist **)params[i].val_adr) = NULL;
      break;

    case PARAM_FUNC:
      break;

    case PARAM_JSTRANS:
#ifdef HAVE_REGEX
      cfg.js_transform = NULL;
#endif
      break;

    case PARAM_NUMLIST:
      *((dllist **)params[i].val_adr) = (dllist *)params[i].default_val.p;
      if (params[i].mval_adr)
        *((bool_t *)params[i].mval_adr) = (bool_t)params[i].mdefault_val.l;
      break;

    case PARAM_FTPHS:
      *((dllist **)params[i].val_adr) = (dllist *)params[i].default_val.p;
      break;

    case PARAM_TAGPAT:
      *((dllist **)params[i].val_adr) = (dllist *)params[i].default_val.p;
      break;
    }
  }
}

int cfg_get_num_params(cfg_param_t *cpar, int opt_type)
{
  long num;
  par_type_t typ;

  static const struct
  {
    par_type_t type;
    int        num_params;
  } tab[] =
  {
    {
      PARAM_FNAME, 1
    },                          /* [i_a] 1 arg: filename, prefixed with '@' for append */
    {
      PARAM_FD, 1
    },                          /* [i_a] 1 arg: file handle (number) or filename, prefixed with '@' or '@@' */
    {
      PARAM_NUM, 1
    },
    {
      PARAM_BYTE_NUM, 1
    },
    {
      PARAM_PBOOL, 0
    },
    {
      PARAM_NBOOL, 0
    },
    {
      PARAM_STR, 1
    },
    {
      PARAM_PASS, 1
    },
    {
      PARAM_STRLIST, 1
    },
    {
      PARAM_STRLIST_EX, 1
    },
    {
      PARAM_CONN, 1
    },
    {
      PARAM_AUTHSCH, 1
    },
    {
      PARAM_MODE, 1
    },
    {
      PARAM_PATH_NC, 1
    },
    {
      PARAM_PATH, 1
    },
    {
      PARAM_TIME, 1
    },
    {
      PARAM_HTMLTAG, 1
    },
    {
      PARAM_TWO_QSTR, 2
    },
    {
      PARAM_DOUBLE, 1
    },
    {
      PARAM_LFNAME, 3
    },
    {
      PARAM_RE, 1
    },
    {
      PARAM_USTRAT, 1
    },
    {
      PARAM_SSLVER, 1
    },
    {
      PARAM_HTTPHDR, 1
    },
    {
      PARAM_DEBUGL, 1
    },
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
    {
      PARAM_HAMMERFLAG, 1
    },
#endif
    {
      PARAM_REQUEST, 1
    },
    {
      PARAM_PROXY, 1
    },
    {
      PARAM_TRANSPARENT, 1
    },
    {
      PARAM_FUNC, 0
    },                          /* default_val == # of expected arguments */
    {
      PARAM_JSTRANS, 4
    },
    {
      PARAM_NUMLIST, 1
    },
    {
      PARAM_FTPHS, 2
    },
    {
      PARAM_TAGPAT, 3
    },
    {
      PARAM_PORT_RANGE, 1
    }
  };

  ASSERT(cpar);
  typ = (cpar->type & ~(PARAM_UNSUPPORTED | PARAM_FOREIGN));
  if (typ < 0 || typ >= NUM_ELEM(tab))
  {
    xprintf(1, gettext("ERROR: internal failure while processing option \"-%s\" - contact the pavuk mailing list for support\n"),
            cfg_get_option_string(cpar, opt_type));
    abort();
  }

  num = tab[typ].num_params;

  if (typ == PARAM_FUNC)
    num = cpar->default_val.l;

  return num;
}

static char *cfg_get_option_prefix_string(cfg_param_t *param, int type)
{
  switch (type)
  {
  case MOPT_OPT_SHORT:
    return "-";

  case MOPT_OPT_LONG:
  case MOPT_OPT_COMPAT:
  default:
    return "";
  }
}

static char *cfg_get_option_string(cfg_param_t *param, int type)
{
  switch (type)
  {
  case MOPT_OPT_SHORT:
    return param->short_cmd;

  case MOPT_OPT_LONG:
    return param->long_cmd;

  case MOPT_OPT_COMPAT:
    return param->long_cmd;

  default:
    return "";
  }
}

static int cfg_postprocess_param_fd(bufio * fdp, const char *basedir, const char *param_str, bool_t open_file_handle)
{
  int rv = 0;

  if (!fdp->filename)
  {
    /* check filedescriptor: must be valid */
    if (fdp->fd >= 0)
    {
#ifdef F_GETFD
      errno = 0;
      if ((fcntl(fdp->fd, F_GETFD) < 0) && (errno == EBADF))
#else
      if (fdp->fd != 1 && fdp->fd != 2)  /* stdout / stderr */
#endif
      {
        xprintf(0,
                gettext("Error: Supplied bad file descriptor %d in option[s]: %s\n"),
                fdp->fd, param_str);
        rv = 1;
      }
      else
      {
        bufio_set_flags(fdp, BUFIO_F_DELAY_WRITE, 0);
      }
    }
  }
  else
  {
    if (fdp->fd < 0)
    {
      if (!strcmp(fdp->filename, "-"))
      {
        fdp->fd = fileno(stdout);
        bufio_set_flags(fdp, BUFIO_F_DELAY_WRITE, 0);
      }
      else
      {
        const char *p;
        const char *fn = NULL;
        /* [i_a] same behaviour as -scenario: if scenario contains path designator, forget about path */
#if defined (__CYGWIN__) || defined (WIN32) || defined (__WIN32)
        p = cvt_win32_to_unix_path(fdp->filename, TRUE);
#else
        p = tl_strdup(fdp->filename);
#endif
        if ((*p == '/') || !basedir)
        {
          fn = tl_strdup(p);
        }
        else
        {
          fn = tl_str_concat(DBGvars(NULL), basedir, (tl_is_dirname(basedir) ? "" : "/"), p, NULL);
        }
        _free(p);
#if defined (__CYGWIN__) || defined (WIN32) || defined (__WIN32)
        p = cvt_win32_to_unix_path(fn, FALSE);
#else
        p = get_abs_file_path_oss(fn);
#endif
        _free(fn);
        ASSERT(p != NULL);
        if (makealldirs(p))
        {
          if (errno != ENOENT)
          {
            xperror(mk_native(p));
            xprintf(0,
                    gettext
                    ("ERROR: Could not construct specified path.\n"
                     "       Invalid filename/path '%s' (%s) specified for parameter[s] '%s'\n"),
                    mk_native(p), mk_native(fdp->filename), param_str);
            rv = 1;
          }
        }
        if (!rv)
        {
          _free(fdp->filename);
          fdp->filename = tl_strdup(p);
          if (open_file_handle
              && bufio_open2spec(fdp, O_BINARY | O_CREAT | O_WRONLY,
                                 S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR))
          {
            xperror(mk_native(p));
            xprintf(0,
                    gettext("Invalid filename/path '%s' (%s) specified for parameter[s] '%s'\n"),
                    mk_native(p), mk_native(fdp->filename), param_str);
            rv = 1;
          }
        }
        _free(p);
      }
    }
  }
  if (!rv && bufio_is_open(fdp) && (fdp->open_flags & O_WRONLY))
  {
    bufio_set_flags(fdp, BUFIO_F_DELAY_WRITE, 0);
    bufio_set_minbuf(fdp, fdp->buf_size);
  }
  if (fdp && fdp->filename)
  {
    DEBUG_BUFIO("open file '%s': %d\n", mk_native(fdp->filename), bufio_is_open(fdp));
  }

  return rv;
}

static char *cfg_portproc_get_matching_params(void *val_adr, int cmd_type)
{
  char *p = NULL;
  int i;

  for (i = 0; i < NUM_ELEM(params); i++)
  {
    if (params[i].val_adr == val_adr)
    {
      const char *cmd_tag = cfg_get_option_string(&params[i], cmd_type);
      const char *prefix = cfg_get_option_prefix_string(&params[i], cmd_type);
      if (cmd_tag)
      {
        if (p && *p)
          p = tl_str_concat(DBGvars(p), ", ", NULL);
        p = tl_str_concat(DBGvars(p), prefix, cmd_tag, NULL);
      }
    }
  }
  return p;
}

/*
 * Create proper filepaths for the various logfiles, etc.
 * Also truncate existing files when 'append' mode is disabled.
 */
static int cfg_postprocessing(int cmd_type)
{
  int rv = 0;
  char *param_str;

  param_str = cfg_portproc_get_matching_params(&cfg.dump_fd, cmd_type);
  rv |= cfg_postprocess_param_fd(cfg.dump_fd, cfg.dump_cmd_dir, param_str, TRUE);
  _free(param_str);
  param_str = cfg_portproc_get_matching_params(&cfg.dump_urlfd, cmd_type);
  rv |= cfg_postprocess_param_fd(cfg.dump_urlfd, cfg.dump_cmd_dir, param_str, TRUE);
  _free(param_str);
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  param_str = cfg_portproc_get_matching_params(&cfg.rec_act_dump_fd, cmd_type);
  rv |= cfg_postprocess_param_fd(cfg.rec_act_dump_fd, cfg.dump_cmd_dir, param_str, TRUE);
  _free(param_str);
#endif

  param_str = cfg_portproc_get_matching_params(&cfg.logfile, cmd_type);
  rv |= cfg_postprocess_param_fd(cfg.logfile, cfg.dump_cmd_dir, param_str, TRUE);
  _free(param_str);
  param_str = cfg_portproc_get_matching_params(&cfg.short_logfile, cmd_type);
  rv |= cfg_postprocess_param_fd(cfg.short_logfile, cfg.dump_cmd_dir, param_str, TRUE);
  _free(param_str);
  param_str = cfg_portproc_get_matching_params(&cfg.time_logfile, cmd_type);
  rv |= cfg_postprocess_param_fd(cfg.time_logfile, cfg.dump_cmd_dir, param_str, TRUE);
  _free(param_str);
  param_str = cfg_portproc_get_matching_params(&cfg.time_logfile4sum, cmd_type);
  rv |= cfg_postprocess_param_fd(cfg.time_logfile4sum, cfg.dump_cmd_dir, param_str, TRUE);
  _free(param_str);
  /* [i_a] added - cfg_dump_cmd() was unused but useful */
  param_str = cfg_portproc_get_matching_params(&cfg.dump_cmd_fname, cmd_type);
  rv |= cfg_postprocess_param_fd(cfg.dump_cmd_fname, cfg.dump_cmd_dir, param_str, TRUE);
  _free(param_str);
  param_str = cfg_portproc_get_matching_params(&cfg.save_scn, cmd_type);
  rv |= cfg_postprocess_param_fd(cfg.save_scn, cfg.scndir, param_str, TRUE);
  _free(param_str);
#if 0
  param_str = cfg_portproc_get_matching_params(&cfg.scenario, cmd_type);
  rv |= cfg_postprocess_param_fd(cfg.scenario, cfg.scndir, param_str, FALSE);
  _free(param_str);
#endif

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  if (!cfg.hammer_endcount)
  {
    /* default 0 means infinity. Let's have that */
    cfg.hammer_endcount = INT_MAX;
  }
#endif

  /* sanity checks */
  if (cfg.no_disc_io && cfg.condition.follow_cmd)
  {
    xprintf(0, gettext("WARNING: cannot execute '-follow_cmd' when '-no_disc_io' has been specified\n"));
  }

#ifdef HAVE_MT
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  if (cfg.nthr > FD_SETSIZE || cfg.hammer_threadcount > FD_SETSIZE)
  {
    xprintf(1, gettext("WARNING: you specified thread counts (-nthread:%d,\n"
                       "         -hammer_thread_count:%d) which are larger than\n"
                       "         this kernel FD_SETSIZE (%d) value. This\n"
                       "         will result in gross TCP/IP stack usage failure.\n"
                       "         Please specify lower thread counts or rebuild\n"
                       "         your kernel with a higher FD_SETSIZE value.\n"), cfg.nthr,
            cfg.hammer_threadcount, FD_SETSIZE);
  }
#endif

  set_mk_native_pool(cfg.nthr);
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  set_mk_native_pool(cfg.hammer_threadcount);
#endif
#endif
  set_mk_native_pool(1);

  if (!cfg.test_id)
  {
    cfg.test_id = tl_strdup("---");
  }
  return rv;
}


static mopt_t mopt = {0};

void cfg_cmdln_cleanup(void)
{
	mopt_destroy(&mopt);
}

void cfg_setup_cmdln(int argc, char **argv)
{
  long nr = 0;
  double dnr = 0.0;
  char *p = NULL;
  char **pl = NULL;
  cfg_param_t *cpar;
  int moptrv;

  mopt_init(&mopt, NUM_ELEM(params), params, argc, argv);

  for (;;)
  {
    moptrv = mopt_get_next_param(&mopt, &cpar);

    if (moptrv == MOPT_END)
      break;

    if (moptrv == MOPT_ERR)
    {
      xprintf(0, gettext("Error parsing commandline\n"));
      usage_short(cpar);
      return;
    }

    if (moptrv == MOPT_MISSINGP)
    {
      xprintf(0,
              gettext("Not enough number of parameters for \"-%s\" option\n"),
              cfg_get_option_string(cpar, mopt.option_type));
      usage_short(cpar);
      return;
    }

    if (moptrv == MOPT_UNKNOWN)
    {
      xprintf(0, gettext("Unknown option %s\n"), argv[mopt.current]);
      usage_short(cpar);
      return;
    }

    if (moptrv == MOPT_BAD)
    {
      xprintf(0, gettext("Wrong format of option %s\n"), argv[mopt.current]);
      usage_short(cpar);
      return;
    }

    if (moptrv == MOPT_OK)
    {
      if (cpar->type & PARAM_UNSUPPORTED)
      {
        xprintf(0, gettext("WARNING: option \"-%s\" not supported in current configuration!\n"),
                cfg_get_option_string(cpar, mopt.option_type));
        continue;
      }

      if (cpar->type & PARAM_FOREIGN)
        continue;

      switch (cpar->type)
      {
      default:
        ASSERT(!"Unsupported parameter type");
        break;

      case PARAM_FNAME:
        cfg_set_to_default(cpar);
        {
          bufio *fdp = *((bufio **)cpar->val_adr);
          ASSERT(fdp);
          ASSERT(fdp->fd < 0);
          if (mopt.args[0])
          {
            fdp->flags = ((mopt.args[0][0] == '@') ? BUFIO_F_APPEND : 0);
            _free(fdp->filename);
            fdp->filename = tl_strdup(&mopt.args[0][!!(fdp->flags & BUFIO_F_APPEND)]);
          }
          else
          {
            xprintf(0,
                    gettext("Please specify filename with parameter \"-%s\"\n"),
                    cfg_get_option_string(cpar, mopt.option_type));
            usage_short(cpar);
			return;
          }
        }
        break;

      case PARAM_FD:
        cfg_set_to_default(cpar);
        {
          bufio *fdp = *((bufio **)cpar->val_adr);
          ASSERT(fdp);
          if (mopt.args[0] && mopt.args[0][0] == '@')
          {
            bool_t append = (mopt.args[0][1] == '@');
            _free(fdp->filename);
            fdp->filename = tl_strdup(&mopt.args[0][1 + append]);
            fdp->fd = -1;
            fdp->flags &= ~BUFIO_F_APPEND;
            fdp->flags |= (append ? BUFIO_F_APPEND : 0);
          }
          else
          {
            int fd;
            _free(fdp->filename);
            fdp->filename = NULL;
            fd = _atoi(mopt.args[0]);
            if (errno == ERANGE)
            {
              xprintf(0,
                      gettext
                      ("Please specify number or '@'-prefixed filename with parameter \"-%s\"\n"),
                      cfg_get_option_string(cpar, mopt.option_type));
              usage_short(cpar);
			  return;
            }
            else
            {
              fdp->fd = fd;
            }
          }
        }
        break;

      case PARAM_BYTE_NUM:
      case PARAM_NUM:
        nr = _atoi_with_size(mopt.args[0], (cpar->type != PARAM_BYTE_NUM));
        if (errno == ERANGE)
        {
          xprintf(0,
                  gettext("Please specify number with parameter \"-%s\"\n"),
                  cfg_get_option_string(cpar, mopt.option_type));
          usage_short(cpar);
          return;
        }
        *((long *)cpar->val_adr) = nr;
        break;

      case PARAM_PBOOL:
        *((bool_t *)cpar->val_adr) = TRUE;
        break;

      case PARAM_NBOOL:
        *((bool_t *)cpar->val_adr) = FALSE;
        break;

      case PARAM_PORT_RANGE:
        if (sscanf(mopt.args[0], "%ld:%ld",
                   (long *)cpar->val_adr,
                   (long *)cpar->mval_adr) != 2
            || *((long *)cpar->val_adr) <= 1023
            || *((long *)cpar->mval_adr) > 65535
            || *((long *)cpar->val_adr) >= *((long *)cpar->mval_adr))
        {
          xprintf(0, gettext("Invalid port range \"%s\"\n"), mopt.args[0]);
          usage_short(cpar);
		  return;
        }
        break;

      case PARAM_PASS:
      case PARAM_STR:
      case PARAM_PATH_NC:
        cfg_set_to_default(cpar);
        if (mopt.args[0][0])
          p = tl_strdup(mopt.args[0]);
        else
          p = NULL;
        *((char **)cpar->val_adr) = p;
        if (cpar->type == PARAM_PASS)
        {
          if (mopt.args[0][0])
          {
            memset(mopt.args[0], ' ', strlen(mopt.args[0]));
            strcpy(mopt.args[0], "*");
          }
        }
        break;

      case PARAM_PATH:
        cfg_set_to_default(cpar);
#if defined (__CYGWIN__) || defined (WIN32) || defined (__WIN32)
        p = cvt_win32_to_unix_path(mopt.args[0], FALSE);
#else
        p = get_abs_file_path_oss(mopt.args[0]);
#endif
        *((char **)cpar->val_adr) = p;
        break;

      case PARAM_STRLIST:
        cfg_set_to_default(cpar);
        pl = tl_str_split(mopt.args[0], ",");
        if (cpar->mval_adr)
          *((bool_t *)cpar->mval_adr) = (bool_t)cpar->mdefault_val.l;
        *((char ***)cpar->val_adr) = pl;
        break;

      case PARAM_STRLIST_EX:
        /* support multiple occurrences of this argument: append to existing */
        if (!cpar->val_adr)
        {
          cfg_set_to_default(cpar);
        }
        pl = tl_str_split(mopt.args[0], ",");
        if (cpar->mval_adr)
          *((bool_t *)cpar->mval_adr) = (bool_t)cpar->mdefault_val.l;
        if (!cpar->val_adr || ! * ((char ***)cpar->val_adr))
        {
          *((char ***)cpar->val_adr) = pl;
        }
        else
        {
          char **sl = *((char ***)cpar->val_adr);
          int n;
          char **p;
          char **s;
          for (n = 0, p = sl; *p; p++, n++) ;
          for (p = pl; *p; p++, n++) ;
          sl = _realloc(sl, sizeof(*sl) * (n + 1));
          for (p = sl; *p; p++) ;
          for (s = pl; *s;)
            *p++ = *s++;
          *p = NULL;
          *((char ***)cpar->val_adr) = sl;
          ASSERT(p - sl <= n);
        }
        break;

      case PARAM_CONN:
        cfg_set_to_default(cpar);
        if (mopt.args[0][0])
        {
          p = strchr(mopt.args[0], ':');
          if (p)
          {
            nr = _atoi(p + 1);
            if (errno == ERANGE)
            {
              struct servent *se;

              if ((se = getservbyname(p + 1, "tcp")))
              {
                nr = ntohs(se->s_port);
              }
              else
              {
                xprintf(0, gettext("Unknown port \"%s\"\n"), p + 1);
              }
            }
            if (cpar->mval_adr)
              *((int *)cpar->mval_adr) = (int)nr;
          }
        }
        else
        {
          p = NULL;
        }
        *((char **)cpar->val_adr) = (p
                                     ? tl_strndup(mopt.args[0], p - mopt.args[0])
                                     : tl_strdup(mopt.args[0]));
        break;

      case PARAM_AUTHSCH:
        nr = authinfo_get_type(mopt.args[0]);
        if (nr == HTTP_AUTH_NONE)
        {
          xprintf(0, gettext("Bad auth scheme \"%s\"\n"), mopt.args[0]);
          usage_short(cpar);
		  return;
        }
        else
        {
          *((int *)cpar->val_adr) = nr;
        }
        break;

      case PARAM_MODE:
        cfg.mode = mode_get_by_str(mopt.args[0]);
        if (cfg.mode == MODE_UNKNOWN)
        {
          xprintf(0, gettext("Unknow operation mode \"%s\"\n"), mopt.args[0]);
          usage_short(cpar);
		  return;
        }
        break;

      case PARAM_TIME:
        *((time_t *)cpar->val_adr) = time_scn_cmd(mopt.args[0]);
        if (*((time_t *)cpar->val_adr) == (time_t)-1)
        {
          xprintf(0,
                  gettext("Bad time parameter \"%s\". Expected format: YYYY.MM.DD.HH:MM\n"),
                  mopt.args[0]);
          usage_short(cpar);
		  return;
        }
        break;

      case PARAM_HTMLTAG:
        htmltag_set_disabled(mopt.args[0], cpar->default_val.l);
        break;

      case PARAM_TWO_QSTR:
        cfg_set_to_default(cpar);
        p = tl_strdup(mopt.args[0]);
        *((char **)cpar->val_adr) = p;

        p = tl_strdup(mopt.args[1]);
        *((char **)cpar->mval_adr) = p;
        break;

      case PARAM_DOUBLE:
        dnr = _atof(mopt.args[0]);
        if (errno == ERANGE)
        {
          xprintf(0,
                  gettext("Please specify floating number with parameter \"-%s\"\n"),
                  cfg_get_option_string(cpar, mopt.option_type));
          usage_short(cpar);
		  return;
        }
        *((double *)cpar->val_adr) = dnr;
        break;

      case PARAM_LFNAME:
        {
          lfname_type t;
          lfname *lfnm;

          if (!strcasecmp(mopt.args[0], "F"))
          {
            t = LFNAME_FNMATCH;
          }
#ifdef HAVE_REGEX
          else if (!strcasecmp(mopt.args[0], "R"))
          {
            t = LFNAME_REGEX;
          }
#endif
          else
          {
            t = LFNAME_UNKNOWN;
            xprintf(0,
                    gettext
                    ("Please specify proper condition type for -%s (%s)\n"),
                    cfg_get_option_string(cpar, mopt.option_type), __CONDITIONS);
            usage_short(cpar);
			return;
          }
          lfnm = lfname_new(t, mopt.args[1], mopt.args[2]);
          if (!lfnm)
		  {
            usage_short(cpar);
			return;
		  }
          cfg.lfnames = dllist_append(cfg.lfnames, (dllist_t)lfnm);
        }
        break;

      case PARAM_RE:
#ifdef HAVE_REGEX
        {
          re_entry *ree = NULL;

          if (!(ree = re_make(mopt.args[0])))
          {
            xprintf(0, gettext("Please specify valid RE with parameter \"-%s\"\n"), cfg_get_option_string(cpar, mopt.option_type));
            usage_short(cpar);
			return;
          }
          *((dllist **)cpar->val_adr) = dllist_append(*((dllist **)cpar->val_adr), (dllist_t)ree);
        }
#endif
        break;

      case PARAM_USTRAT:
        *(strategy *)cpar->val_adr = get_strategy_by_str(mopt.args[0]);
        if (*(strategy *)cpar->val_adr == SSTRAT_LAST)
        {
          xprintf(0, gettext("Unknown URL scheduling strategy - \"%s\"\n"), mopt.args[0]);
          usage_short(cpar);
		  return;
        }
        break;

      case PARAM_SSLVER:
        *(long *)cpar->val_adr = get_ssl_version_by_str(mopt.args[0]);
        if (*(long *)cpar->val_adr == -1)
        {
          xprintf(0, gettext("Unknown SSL version - \"%s\"\n"), mopt.args[0]);
          usage_short(cpar);
		  return;
        }
        break;

      case PARAM_HTTPHDR:
        {
          httphdr *hr = httphdr_parse(mopt.args[0]);
          if (!hr)
          {
            xprintf(0, gettext("Invalid additional HTTP header - \"%s\"\n"), mopt.args[0]);
            usage_short(cpar);
			return;
          }
          *((dllist **)cpar->val_adr) = dllist_append(*((dllist **)cpar->val_adr), (dllist_t)hr);
        }
        break;

      case PARAM_DEBUGL:
#ifdef HAVE_DEBUG_FEATURES
        {
          unsigned long dl;
          if (debug_level_parse(mopt.args[0], &dl))
          {
            usage_short(cpar);
			return;
          }
          *((unsigned long *)cpar->val_adr) = dl;
        }
#endif
        break;

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
      case PARAM_HAMMERFLAG:
        {
          unsigned long dl;
          if (hammerflag_level_parse(mopt.args[0], &dl))
          {
            usage_short(cpar);
			return;
          }
          *((unsigned long *)cpar->val_adr) = dl;
        }
        break;

#endif
      case PARAM_REQUEST:
        {
          /*
           * heuristics: see if we can decode the line in '-request' style,
           * if not, assume it is an old-style url with no frills.
           */
          url_info *ui = url_info_parse(mopt.args[0],
                                        TRUE /* do not yak when this goes wrong */);
          if (!ui)
          {
            ui = url_info_new(mopt.args[0]);
          }
          if (!ui)
          {
            xprintf(0, gettext("Invalid request specification - \"%s\"\n"), mopt.args[0]);
            usage_short(cpar);
			return;
          }
          *((dllist **)cpar->val_adr) = dllist_append(*((dllist **)cpar->val_adr), (dllist_t)ui);
        }
        break;

      case PARAM_TRANSPARENT:
        if (mopt.args[0][0])
        {
          http_proxy *pr = http_proxy_parse(mopt.args[0]);
          if (!pr)
          {
            usage_short(cpar);
			return;
          }
          else
          {
            *((http_proxy **)cpar->val_adr) = pr;
          }
        }
        else
        {
          cfg_set_to_default(cpar);
        }
        break;

      case PARAM_PROXY:
        if (mopt.args[0][0])
        {
          http_proxy *pr = http_proxy_parse(mopt.args[0]);

          if (!pr)
          {
            usage_short(cpar);
			return;
          }
          else
          {
            *((dllist **)cpar->val_adr) = dllist_append(*((dllist **)cpar->val_adr), (dllist_t)pr);
          }
        }
        else
        {
          cfg_set_to_default(cpar);
        }
        break;

      case PARAM_FUNC:
        {
          typedef int (*cfg_func_t)(char *, char *, char *, char *);
          cfg_func_t _cfg_func;

          _cfg_func = (cfg_func_t)cpar->val_adr;
          if (_cfg_func)
          {
            if (_cfg_func(mopt.args[0], mopt.args[1], mopt.args[2], mopt.args[3]))
            {
              usage_short(cpar);
			  return;
            }
          }
        }
        break;

      case PARAM_JSTRANS:
#ifdef HAVE_REGEX
        {
          js_transform_t *jt;
          jt = js_transform_new(mopt.args[0],
                                mopt.args[1],
                                mopt.args[2],
                                mopt.args[3],
                                cpar->mdefault_val.l);
          if (!jt)
          {
            xprintf(0, gettext("Invalid parameters for \"-%s\" option\n"),
                    cfg_get_option_string(cpar, mopt.option_type));
            usage_short(cpar);
			return;
          }
          else
          {
            cfg.js_transform = dllist_append(cfg.js_transform, (dllist_t)jt);
          }
        }
#endif
        break;

      case PARAM_NUMLIST:
        {
          dllist *ptr = tl_numlist_split(mopt.args[0], ",");

          if (!ptr && mopt.args[0][0])
          {
            xprintf(0,
                    gettext("Invalid number list \"%s\" for option \"-%s\"\n"),
                    mopt.args[0], cfg_get_option_string(cpar, mopt.option_type));
            usage_short(cpar);
			return;
          }
          cfg_set_to_default(cpar);
          if (cpar->mval_adr)
            *((bool_t *)cpar->mval_adr) = (bool_t)cpar->mdefault_val.l;
          *((dllist **)cpar->val_adr) = ptr;
        }
        break;

      case PARAM_FTPHS:
        {
          ftp_handshake_info *fhi;
          fhi = ftp_handshake_info_parse(mopt.args[0], mopt.args[1]);
          if (!fhi)
          {
            xprintf(0,
                    gettext("Invalid FTP login handshake string \"%s\" for option \"-%s\"\n"),
                    mopt.args[1], cfg_get_option_string(cpar, mopt.option_type));
            usage_short(cpar);
			return;
          }
          *((dllist **)cpar->val_adr) = dllist_append(*((dllist **)cpar->val_adr), (dllist_t)fhi);
        }
        break;

      case PARAM_TAGPAT:
        {
          tag_pattern_t *tp;
          tp = tag_pattern_new(cpar->mdefault_val.l, mopt.args[0], mopt.args[1], mopt.args[2]);
          if (!tp)
          {
            usage_short(cpar);
			return;
          }
          *((dllist **)cpar->val_adr) = dllist_append(*((dllist **)cpar->val_adr), (dllist_t)tp);
        }
        break;
      }
    }

    if (moptrv == MOPT_PARAM)
    {
      url_info *ui;
      ui = url_info_new(mopt.args[0]);
      cfg.request = dllist_append(cfg.request, (dllist_t)ui);
      LOCK_TCNT;
      cfg.total_cnt++;
      UNLOCK_TCNT;
    }
  }

  if (cfg_postprocessing(mopt.option_type))
  {
    usage_short(NULL);
	return;
  }

  mopt_destroy(&mopt);
}

static int cfg_load_fd(bufio *fd)
{
  int i;
  bool_t found;
  long nr;
  double dnr;
  char *p;
  char lnbuf[BUFIO_ADVISED_READLN_BUFSIZE];
  char *lns;
  pavuk_mode temp_mode;
  int rv = 0;

  while (bufio_readln(fd, lnbuf, sizeof(lnbuf)) > 0)
  {
    strip_nl(lnbuf);
    for (lns = lnbuf; *lns && tl_ascii_isspace(*lns); lns++) ;
    if (*lns == '#' || ! * lns)
      continue;

    found = FALSE;

    for (i = 0; i < NUM_ELEM(params); i++)
    {
      if (!params[i].par_entry)
        continue;
      if (!strncasecmp(lns, params[i].par_entry, strlen(params[i].par_entry)))
      {
        if (params[i].type & PARAM_UNSUPPORTED)
        {
          xprintf(0, gettext("WARNING: option \"-%s\" not supported in current configuration!\n"), params[i].par_entry);
          continue;
        }

        if (params[i].type & PARAM_FOREIGN)
          continue;

        lns += strlen(params[i].par_entry);
        for ( ; *lns && tl_ascii_isspace(*lns); lns++) ;
        for (p = lns + strlen(lns) - 1; p >= lns && tl_ascii_isspace(*p); p--)
          *p = '\0';

        if (! * lns)
        {
          cfg_set_to_default(&(params[i]));
          continue;
        }

        found = TRUE;
        switch (params[i].type)
        {
        default:
          ASSERT(!"Unsupported param type");
          break;

        case PARAM_FNAME:
          cfg_set_to_default(&params[i]);
          {
            bufio *fdp = *((bufio **)params[i].val_adr);
            ASSERT(fdp);
            ASSERT(fdp->fd < 0);
            ASSERT(!fdp->filename);
            if (lns)             /* [i_a] */
            {
              fdp->flags = ((lns[0] == '@') ? BUFIO_F_APPEND : 0);
              ASSERT(!fdp->filename);
              fdp->filename = tl_strdup(&lns[!!(fdp->flags & BUFIO_F_APPEND)]);
              fdp->fd = -1;
            }
            else
            {
              xprintf(0, gettext("Please specify filename with parameter \"-%s\"\n"), params[i].par_entry);
              rv = 1;
            }
          }
          break;

        case PARAM_FD:
          cfg_set_to_default(&params[i]);
          {
            bufio *fdp = *((bufio **)params[i].val_adr);
            ASSERT(fdp);
            ASSERT(fdp->fd < 0);
            ASSERT(!fdp->filename);
            if (lns && lns[0] == '@')
            {
              bool_t append = (lns[1] == '@');
              ASSERT(!fdp->filename);
              fdp->filename = tl_strdup(&lns[1 + append]);
              fdp->fd = -1;
              fdp->flags &= ~BUFIO_F_APPEND;
              fdp->flags |= (append ? BUFIO_F_APPEND : 0);
            }
            else
            {
              nr = _atoi(lns);
              if (errno == ERANGE)
              {
                xprintf(0, gettext("Please specify a number or '@'-prefixed filename for option \"%s\"\n"), params[i].par_entry);
                rv = 1;
              }
              else
              {
                fdp->fd = nr;
              }
            }
          }
          break;

        case PARAM_BYTE_NUM:
        case PARAM_NUM:
          nr = _atoi_with_size(lns, (params[i].type != PARAM_BYTE_NUM));
          if (errno == ERANGE)
          {
            xprintf(0, gettext("Please specify number \"%s\"\n"), params[i].par_entry);
            rv = 1;
          }
          else
          {
            *((long *)params[i].val_adr) = nr;
          }
          break;

        case PARAM_PBOOL:
        case PARAM_NBOOL:
          if (!strcasecmp(lns, "false"))
          {
            *((bool_t *)params[i].val_adr) = FALSE;
          }
          else if (!strcasecmp(lns, "true"))
          {
            *((bool_t *)params[i].val_adr) = TRUE;
          }
          else
          {
            xprintf(0, gettext("Only \"true\" & \"false\" is allowed : \"%s\"\n"), lns);
            rv = 1;
          }
          break;

        case PARAM_PORT_RANGE:
          if (sscanf(lns, "%ld:%ld",
                     (long *)params[i].val_adr,
                     (long *)params[i].mval_adr) != 2
              || *((long *)params[i].val_adr) <= 1023
              || *((long *)params[i].mval_adr) > 65535
              || *((long *)params[i].val_adr) >= *((long *)params[i].mval_adr))
          {
            xprintf(0, gettext("Invalid port range \"%s\"\n"), lns);
            rv = 1;
          }
          break;

        case PARAM_PATH_NC:
        case PARAM_STR:
        case PARAM_PASS:
          cfg_set_to_default(&params[i]);
          *((char **)params[i].val_adr) = *lns ? tl_strdup(lns) : NULL;
          break;

        case PARAM_PATH:
          cfg_set_to_default(&params[i]);
#if defined (__CYGWIN__) || defined (WIN32) || defined (__WIN32)
          *((char **)params[i].val_adr) = *lns ? cvt_win32_to_unix_path(lns, FALSE) : NULL;
#else
          *((char **)params[i].val_adr) = *lns ? get_abs_file_path_oss(lns) : NULL;
#endif
          break;

        case PARAM_STRLIST:
          cfg_set_to_default(&params[i]);
          if (params[i].mval_adr)
          {
            *((bool_t *)params[i].mval_adr) = (bool_t)params[i].mdefault_val.l;
          }
          if (*lns)
          {
            *((char ***)params[i].val_adr) = tl_str_split(lns, ",");
          }
          break;

        case PARAM_STRLIST_EX:
          cfg_set_to_default(&params[i]);
          if (params[i].mval_adr)
          {
            *((bool_t *)params[i].mval_adr) = (bool_t)params[i].mdefault_val.l;
          }
          if (*lns)
          {
            *((char ***)params[i].val_adr) = tl_str_split(lns, ",");
          }
          break;

        case PARAM_CONN:
          cfg_set_to_default(&params[i]);
          p = strchr(lns, ':');
          if (p)
          {
            nr = _atoi(p + 1);
            if (errno == ERANGE)
            {
              struct servent *se;

              if ((se = getservbyname(p + 1, "tcp")))
              {
                nr = ntohs(se->s_port);
              }
              else
              {
                xprintf(0, gettext("Unknown port \"%s\"\n"), p + 1);
                rv = 1;
              }
            }
            if (params[i].mval_adr)
            {
              *((int *)params[i].mval_adr) = (int)nr;
            }
          }
          *((char **)params[i].val_adr) = p ? tl_strndup(lns, p - lns) : tl_strdup(lns);
          break;

        case PARAM_AUTHSCH:
          nr = authinfo_get_type(lns);
          if (nr == HTTP_AUTH_NONE)
          {
            xprintf(0, gettext("Bad auth scheme \"%s\"\n"), lns);
            rv = 1;
          }
          else
          {
            *((int *)params[i].val_adr) = nr;
          }
          break;

        case PARAM_MODE:
          temp_mode = mode_get_by_str(lns);
          if (temp_mode == MODE_UNKNOWN)
          {
            xprintf(0, gettext("Unknow operation mode \"%s\"\n"), lns);
            rv = 1;
          }
          else
          {
            *((pavuk_mode *)params[i].val_adr) = temp_mode;
          }
          break;

        case PARAM_TIME:
          {
            time_t ttm = time_scn_cmd(lns);
            if (ttm == (time_t)-1)
            {
			  xprintf(0, gettext("Bad time parameter \"%s\". Expected format: YYYY.MM.DD.HH:MM\n"), lns);
              rv = 1;
            }
            else
            {
              *(time_t *)params[i].val_adr = ttm;
            }
          }
          break;

        case PARAM_HTMLTAG:
          htmltag_set_disabled(lns, params[i].default_val.l);
          break;

        case PARAM_TWO_QSTR:
          cfg_set_to_default(&params[i]);
          if (lns && *lns)
          {
            char *xp = tl_strdup(lns);

            *(char **)params[i].val_adr = tl_strdup(get_1qstr(xp));
            *(char **)params[i].mval_adr = tl_strdup(get_1qstr(NULL));

            _free(xp);
          }
          else
          {
            *(char **)params[i].val_adr = NULL;
            *(char **)params[i].mval_adr = NULL;
          }
          break;

        case PARAM_DOUBLE:
          dnr = _atof(lns);
          if (errno == ERANGE)
          {
            xprintf(0, gettext("Please specify floating number \"%s\"\n"), params[i].par_entry);
            rv = 1;
          }
          else
          {
            *(double *)params[i].val_adr = dnr;
          }
          break;

        case PARAM_LFNAME:
          {
            char *ps1 = tl_strdup(get_1qstr(lns));
            char *ps2 = tl_strdup(get_1qstr(NULL));
            char *ps3 = tl_strdup(get_1qstr(NULL));
            lfname_type t;
            lfname *lfnm;

            if (!ps1 || !ps2 || !ps3)
            {
              t = LFNAME_UNKNOWN;
              xprintf(0, gettext("Please specify proper arguments for %s\n"), params[i].par_entry);
              rv = 1;
            }
            else if (!strcasecmp(ps1, "F"))
            {
              t = LFNAME_FNMATCH;
            }
#ifdef HAVE_REGEX
            else if (!strcasecmp(ps1, "R"))
            {
              t = LFNAME_REGEX;
            }
#endif
            else
            {
              t = LFNAME_UNKNOWN;
              xprintf(0, gettext("Please specify proper condition type for %s (%s)\n"), params[i].par_entry, __CONDITIONS);
              rv = 1;
            }
            if (t != LFNAME_UNKNOWN)
            {
              lfnm = lfname_new(t, ps2, ps3);
              if (lfnm)
              {
                cfg.lfnames = dllist_append(cfg.lfnames, (dllist_t)lfnm);
              }
              else
              {
                rv = 1;
              }
            }
            _free(ps1);
            _free(ps2);
            _free(ps3);
          }
          break;

        case PARAM_RE:
#ifdef HAVE_REGEX
          {
            re_entry *ree;
            if (!(ree = re_make(lns)))
            {
              xprintf(0, gettext("Please specify valid RE \"%s\"\n"), params[i].par_entry);
              rv = 1;
            }
            else
            {
              *(dllist **) params[i].val_adr = dllist_append(*((dllist **) params[i].val_adr), (dllist_t) ree);
            }
          }
#endif
          break;

        case PARAM_USTRAT:
          {
            strategy strtg = get_strategy_by_str(lns);
            if (strtg == SSTRAT_LAST)
            {
              xprintf(0, gettext("Unknown URL scheduling strategy - \"%s\"\n"), lns);
              rv = 1;
            }
            else
            {
              *(strategy *)params[i].val_adr = strtg;
            }
          }
          break;

        case PARAM_SSLVER:
          {
            int sslv = get_ssl_version_by_str(lns);
            if (sslv == -1)
            {
              xprintf(0, gettext("Unknown SSL version - \"%s\"\n"), lns);
              rv = 1;
            }
            else
            {
              *(int *)params[i].val_adr = sslv;
            }
          }
          break;

        case PARAM_HTTPHDR:
          {
            httphdr *hr = httphdr_parse(lns);
            if (!hr)
            {
              xprintf(0, gettext("Invalid additional HTTP header - \"%s\"\n"), lns);
              rv = 1;
            }
            else
            {
              *((dllist **) params[i].val_adr) = dllist_append(*((dllist **) params[i].val_adr), (dllist_t) hr);
            }
          }
          break;

        case PARAM_DEBUGL:
#ifdef HAVE_DEBUG_FEATURES
          {
            unsigned long dl;
            if (!debug_level_parse(lns, &dl))
            {
              *((unsigned long *)params[i].val_adr) = dl;
            }
            else
            {
              rv = 1;
            }
          }
#endif
          break;

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
        case PARAM_HAMMERFLAG:
          {
            unsigned long dl;
            if (!hammerflag_level_parse(lns, &dl))
            {
              *((unsigned long *)params[i].val_adr) = dl;
            }
            else
            {
              rv = 1;
            }
          }
          break;

#endif
        case PARAM_REQUEST:
          {
            /*
             * heuristics: see if we can decode the line in '-request' style,
             * if not, assume it is an old-style url with no frills.
             */
            url_info *ui = url_info_parse(lns,
                                          TRUE /* do not yak when this goes wrong */);
            if (!ui)
            {
              ui = url_info_new(lns);
            }
            if (!ui)
            {
              xprintf(0, gettext("Invalid request specification - \"%s\"\n"), lns);
              rv = 1;
            }
            else
            {
              *((dllist **) params[i].val_adr) = dllist_append(*((dllist **) params[i].val_adr), (dllist_t) ui);
            }
          }
          break;

        case PARAM_TRANSPARENT:
          if (lns)
          {
            http_proxy *pr = http_proxy_parse(lns);
            if (pr)
            {
              *((http_proxy **)params[i].val_adr) = pr;
            }
            else
            {
              rv = 1;
            }
          }
          break;

        case PARAM_PROXY:
          if (lns)
          {
            http_proxy *pr = http_proxy_parse(lns);

            if (pr)
            {
              *((dllist **) params[i].val_adr) = dllist_append(*((dllist **) params[i].val_adr), (dllist_t) pr);
            }
            else
            {
              rv = 1;
            }
          }
          break;

        case PARAM_FUNC:
          break;

        case PARAM_JSTRANS:
#ifdef HAVE_REGEX
          {
            char *ps1 = tl_strdup(get_1qstr(lns));
            char *ps2 = tl_strdup(get_1qstr(NULL));
            char *ps3 = tl_strdup(get_1qstr(NULL));
            char *ps4 = tl_strdup(get_1qstr(NULL));

            js_transform_t *jt;

            jt = js_transform_new(ps1, ps2, ps3, ps4, params[i].mdefault_val.l);
            if (!jt)
            {
              xprintf(0, gettext("Invalid js_transform specification - \"%s\"\n"), lns);
              rv = 1;
            }
            else
              cfg.js_transform = dllist_append(cfg.js_transform, (dllist_t)jt);
            _free(ps1);
            _free(ps2);
            _free(ps3);
            _free(ps4);
          }
#endif
          break;

        case PARAM_NUMLIST:
          cfg_set_to_default(&params[i]);
          if (params[i].mval_adr)
          {
            *((bool_t *)params[i].mval_adr) = (bool_t)params[i].mdefault_val.l;
          }
          if (*lns)
          {
            *((dllist **)params[i].val_adr) = tl_numlist_split(lns, ",");
          }
          if (*lns && ! * ((dllist **)params[i].val_adr))
          {
            xprintf(0, gettext("Invalid number list specification - \"%s\"\n"), lns);
            rv = 1;
          }
          break;

        case PARAM_FTPHS:
          {
            char *ps1 = tl_strdup(get_1qstr(lns));
            char *ps2 = tl_strdup(get_1qstr(NULL));
            ftp_handshake_info *fhi = NULL;

            if (ps1 && ps2)
            {
              fhi = ftp_handshake_info_parse(ps1, ps2);
            }
            if (!fhi)
            {
              xprintf(0, gettext("Invalid FTP login handshake string \"%s\".\n"), lns);
              rv = 1;
            }
            *((dllist **) params[i].val_adr) = dllist_append(*((dllist **) params[i].val_adr), (dllist_t) fhi);
            _free(ps1);
            _free(ps2);
          }
          break;

        case PARAM_TAGPAT:
          {
            char *ps1 = tl_strdup(get_1qstr(lns));
            char *ps2 = tl_strdup(get_1qstr(NULL));
            char *ps3 = tl_strdup(get_1qstr(NULL));
            tag_pattern_t *tp = NULL;

            if (ps1 && ps2 && ps3)
            {
              tp = tag_pattern_new(params[i].mdefault_val.l, ps1, ps2, ps3);
            }
            if (!tp)
            {
              rv = 1;
            }
            *((dllist **) params[i].val_adr) = dllist_append(*((dllist **) params[i].val_adr), (dllist_t) tp);
            _free(ps1);
            _free(ps2);
            _free(ps3);
          }
          break;
        }
      }
    }
    if (!found && !strncasecmp(lns, "URL:", 4))
    {
      url_info *ui;

      lns += 4;
      for ( ; *lns && tl_ascii_isspace(*lns); lns++) ;
      for (p = lns + strlen(lns); *p && tl_ascii_isspace(*p); p--)
        *p = '\0';
      if (! * lns)
        continue;

      ui = url_info_new(lns);
      cfg.request = dllist_append(cfg.request, (dllist_t)ui);
      LOCK_TCNT;
      cfg.total_cnt++;
      UNLOCK_TCNT;
    }
    else if (!found)
    {
      xprintf(0, gettext("Unable to parse \"%s\"\n"), lns);
      rv = 1;
    }
  }

  rv |= cfg_postprocessing(MOPT_OPT_LONG);
  return rv;
}

int cfg_load(const char *filename)
{
  int rv;
  bufio *fd;

  if (!(fd = bufio_open(filename, O_BINARY | O_RDONLY)))
  {
    xperror(mk_native(filename));
    return -1;
  }

  rv = cfg_load_fd(fd);

  bufio_close(fd);
  fd = NULL;
  return rv;
}

static int cfg_load_scenario(const char *filename)
{
  const char *p;
  const char *fn = NULL;
  int rv;

  _free(cfg.scenario);
#if defined (__CYGWIN__) || defined (WIN32) || defined (__WIN32)
  p = cvt_win32_to_unix_path(filename, TRUE);
#else
  p = tl_strdup(filename);
#endif

  if ((*p == '/') || !cfg.scndir)
  {
    fn = tl_strdup(p);
  }
  else
  {
    fn = tl_str_concat(DBGvars(NULL), cfg.scndir, (tl_is_dirname(cfg.scndir) ? "" : "/"), p, NULL);
  }
  _free(p);
#if defined (__CYGWIN__) || defined (WIN32) || defined (__WIN32)
  p = cvt_win32_to_unix_path(fn, FALSE);
#else
  p = get_abs_file_path_oss(fn);
#endif
  ASSERT(p != NULL);
  if ((rv = cfg_load(p)))
  {
    xprintf(0, gettext("ERROR: Scenario loading failed (%s)\n"), fn);
    _free(fn);
    exit(PAVUK_EXIT_CFG_ERR);
  }
  _free(fn);

  cfg.scenario = tl_strdup(p);
  _free(p);
  return rv;
}

/*
   Attempt to load the pavuk configuration settings from, in order of appearance:

   env:PAVUKRC_FILE
   ~/.pavukrc
   SYSCONFDIR/pavukrc

   (see also man page)
 */
void cfg_load_setup(void)
{
  char pom[PATH_MAX];
  char *p;

#ifdef DEFAULTRC
  if (!tl_access(DEFAULTRC, R_OK))
  {
    if (cfg_load(DEFAULTRC))
    {
      xprintf(0, gettext("ERROR: Loading default configuration from resource file '%s' failed\n"), DEFAULTRC);
      exit(PAVUK_EXIT_CFG_ERR);
    }
  }
#endif
  p = getenv("PAVUKRC_FILE");
  if (!p || tl_access(p, R_OK))
  {
    /*
	   no file specified or file does not exist: try the next level: ~/.pavukrc
	 */
    snprintf(pom, sizeof(pom), "%s%s%s%s", cfg.path_to_home,
             (tl_is_dirname(cfg.path_to_home) ? "" : "/"), 
			 PAVUK_CFGFNAME_PREFIX, PAVUK_CONFIG_FILENAME);
    pom[sizeof(pom) - 1] = 0;
    p = pom;
  }
#if defined(SYS_CONF_DIR)
  if (tl_access(p, R_OK))
  {
    /*
	   no file specified or file does not exist: try the last level: /bin/local/etc/pavukrc

	   WARNING: this pavukrc does NOT have the '.' prefix in the filename!
	 */
    snprintf(pom, sizeof(pom), "%s%s%s", SYS_CONF_DIR,
             (tl_is_dirname(SYS_CONF_DIR) ? "" : "/"), 
			 PAVUK_CONFIG_FILENAME);
    pom[sizeof(pom) - 1] = 0;
    p = pom;
  }
#endif
  if (!tl_access(p, R_OK))
  {
    if (cfg_load(p))
    {
      xprintf(0, gettext("ERROR: Loading configuration from $PAVUKRC_FILE '%s' failed\n"), p);
      exit(PAVUK_EXIT_CFG_ERR);
    }
  }
}

static int cfg_dump_fd(bufio *fd_wl)
{
  int i, j;
  char pom[8192];
  char pom2[20];
  char **pl;
  bufio *fd;

  VERIFY(!bufio_lock(fd_wl, FALSE, &fd));
  if (cfg.request)
  {
    dllist *dptr;

    for (dptr = cfg.request; dptr; dptr = dptr->next)
    {
      url_info *ui = (url_info *)dptr->data;

      if (ui->type == URLI_NORMAL && !ui->localname)
      {
        bufio_puts_m(fd, "URL: ", ui->urlstr, "\n", NULL);
      }
    }
  }

  for (i = 0; i < NUM_ELEM(params); i++)
  {
    if (params[i].type & PARAM_UNSUPPORTED)
      continue;
    if (params[i].type & PARAM_FOREIGN)
      continue;
    if (!params[i].par_entry)
      continue;

    switch (params[i].type)
    {
    default:
      ASSERT(!"Unsupported param type");
      break;

    case PARAM_FNAME:
      {
        bufio *fdp = *((bufio **)params[i].val_adr);
        if (fdp && fdp->filename)
        {
          bufio_printf(fd, "%s %s%s\n", params[i].par_entry,
                       ((fdp->flags & BUFIO_F_APPEND) ? "@" : ""), mk_native(fdp->filename));
        }
      }
      break;

    case PARAM_FD:
      {
        bufio *fdp = *((bufio **)params[i].val_adr);
        if (fdp && fdp->filename)
        {
          bufio_printf(fd, "%s @%s%s\n", params[i].par_entry,
                       ((fdp->flags & BUFIO_F_APPEND) ? "@" : ""), mk_native(fdp->filename));
        }
        else if (fdp && fdp->fd >= 0)
        {
          bufio_printf(fd, "%s %d\n", params[i].par_entry, fdp->fd);
        }
      }
      break;

    case PARAM_BYTE_NUM:
    case PARAM_NUM:
      bufio_printf(fd, "%s %ld\n", params[i].par_entry, *((long *)params[i].val_adr));
      break;

    case PARAM_NBOOL:
    case PARAM_PBOOL:
      if (*((bool_t *)params[i].val_adr))
        bufio_printf(fd, "%s true\n", params[i].par_entry);
      else
        bufio_printf(fd, "%s false\n", params[i].par_entry);
      break;

    case PARAM_PORT_RANGE:
      if (*((long *)params[i].val_adr) >= 0)
      {
        bufio_printf(fd, "%s %ld:%ld\n", params[i].par_entry, *((long *)params[i].val_adr),
                     *((long *)params[i].mval_adr));
      }
      break;

    case PARAM_PATH:
      if (*((char **)params[i].val_adr))
      {
        bufio_printf(fd, "%s %s\n", params[i].par_entry, mk_native(*((char **)params[i].val_adr)));
      }
      break;

    case PARAM_PATH_NC:
    case PARAM_STR:
    case PARAM_PASS:
      if (*((char **)params[i].val_adr))
      {
        bufio_printf(fd, "%s %s\n", params[i].par_entry, *((char **)params[i].val_adr));
      }
      break;

    case PARAM_STRLIST:
    case PARAM_STRLIST_EX:
      if (!params[i].mval_adr
          || (params[i].mval_adr
              && (*((bool_t *)params[i].mval_adr) == (bool_t)params[i].mdefault_val.l)))
      {
        pl = *((char ***)params[i].val_adr);
        if (pl && pl[0])
        {
          bufio_printf(fd, "%s %s", params[i].par_entry, pl[0]);
          j = 1;
          while (pl[j])
          {
            bufio_puts(fd, ",");
            bufio_puts(fd, pl[j]);
            j++;
          }
          bufio_puts(fd, "\n");
        }
      }
      break;

    case PARAM_CONN:
      if (*((char **)params[i].val_adr))
      {
        bufio_printf(fd, "%s %s:%d\n", params[i].par_entry, *((char **)params[i].val_adr),
                     *((int *)params[i].mval_adr));
      }
      break;

    case PARAM_AUTHSCH:
      bufio_printf(fd,
                   "%s %s\n",
                   params[i].par_entry,
                   http_auths[*((long *)params[i].val_adr)].name);
      break;

    case PARAM_MODE:
      bufio_printf(fd, "%s %s\n", params[i].par_entry, mode_get_str(cfg.mode));
      break;

    case PARAM_TIME:
      if (*((time_t *)params[i].val_adr))
      {
        LOCK_TIME;
        strftime(pom2, sizeof(pom2), "%Y.%m.%d.%H:%M", localtime((time_t *)params[i].val_adr));
        UNLOCK_TIME;
        bufio_printf(fd, "%s %s\n", params[i].par_entry, pom2);
      }
      break;

    case PARAM_HTMLTAG:
      {
        int x, y;
        bool_t first = TRUE;

        snprintf(pom, sizeof(pom), "%s ", params[i].par_entry);
        pom[sizeof(pom) - 1] = 0;
        for (x = 0; x < html_link_tags_num(); x++)
        {
          for (y = 0; html_link_tags[x].attribs[y].attrib; y++)
          {
            if (!(html_link_tags[x].attribs[y].stat & LINK_DISABLED) == !params[i].default_val.l)
            {
              if (!first)
              {
                strncat(pom, ";", sizeof(pom) - strlen(pom));
                pom[sizeof(pom) - 1] = '\0';
              }
              strncat(pom, html_link_tags[x].tag, sizeof(pom) - strlen(pom));
              pom[sizeof(pom) - 1] = '\0';
              strncat(pom, ",", sizeof(pom) - strlen(pom));
              pom[sizeof(pom) - 1] = '\0';
              strncat(pom, html_link_tags[x].attribs[y].attrib, sizeof(pom) - strlen(pom));
              pom[sizeof(pom) - 1] = '\0';
              first = FALSE;
            }
          }
        }
        strncat(pom, "\n", sizeof(pom) - strlen(pom));
        pom[sizeof(pom) - 1] = '\0';
        if (!first)
        {
          bufio_puts(fd, pom);
        }
      }
      break;

    case PARAM_TWO_QSTR:
      if (*((char **)params[i].val_adr))
      {
        char *p1, *p2;
        p1 = escape_str(*((char **)params[i].val_adr), "\"");
        p2 = escape_str(*((char **)params[i].mval_adr), "\"");
        bufio_printf(fd, "%s \"%s\" \"%s\"\n", params[i].par_entry, p1, p2);
        _free(p1);
        _free(p2);
      }
      break;

    case PARAM_DOUBLE:
      bufio_printf(fd, "%s %.3lf\n", params[i].par_entry, *((double *)params[i].val_adr));
      break;

    case PARAM_LFNAME:
      {
        dllist *pdl = cfg.lfnames;
        while (pdl)
        {
          lfname *lfnm = (lfname *)pdl->data;
          char *p1, *p2;

          p1 = escape_str(lfnm->matchstr, "\"");
          p2 = escape_str(lfnm->transstr, "\"");

          bufio_printf(fd, "%s \"%s\" \"%s\" \"%s\"\n", params[i].par_entry,
                       (lfnm->type == LFNAME_FNMATCH) ? "F" : "R", p1, p2);
          _free(p1);
          _free(p2);

          pdl = pdl->next;
        }
      }
      break;

    case PARAM_RE:
#ifdef HAVE_REGEX
      {
        dllist *ptr = *((dllist **)params[i].val_adr);
        while (ptr)
        {
          re_entry *ree = (re_entry *)ptr->data;

          bufio_printf(fd, "%s %s\n", params[i].par_entry, ree->pattern);

          ptr = ptr->next;
        }
      }
#endif
      break;

    case PARAM_USTRAT:
      bufio_printf(fd, "%s %s\n", params[i].par_entry,
                   get_strategy_str(*(strategy *)params[i].val_adr));
      break;

    case PARAM_SSLVER:
      bufio_printf(fd, "%s %s\n", params[i].par_entry,
                   get_ssl_version_str(*(long *)params[i].val_adr));
      break;

    case PARAM_HTTPHDR:
      {
        dllist *ptr = *((dllist **)params[i].val_adr);
        while (ptr)
        {
          httphdr *hr = (httphdr *)ptr->data;

          bufio_printf(fd,
                       "%s %s%s %s\n",
                       params[i].par_entry,
                       hr->all ? "+" : "",
                       hr->name,
                       hr->val);

          ptr = ptr->next;
        }
      }
      break;

    case PARAM_DEBUGL:
#ifdef HAVE_DEBUG_FEATURES
      {
        char *strbuf = debug_level_construct(*((unsigned long *)params[i].val_adr));
        bufio_printf(fd, "%s %s\n", params[i].par_entry, strbuf);
        _free(strbuf);
      }
#endif
      break;

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
    case PARAM_HAMMERFLAG:
      {
        char *strbuf = hammerflag_level_construct(*((unsigned long *)params[i].val_adr));
        bufio_printf(fd, "%s %s\n", params[i].par_entry, strbuf);
        _free(strbuf);
      }
      break;

#endif
    case PARAM_REQUEST:
      {
        dllist *ptr = *((dllist **)params[i].val_adr);
        while (ptr)
        {
          url_info *ui = (url_info *)ptr->data;

          if (ui->type != URLI_NORMAL || ui->localname)
          {
            char *p = url_info_dump(ui);
            bufio_printf(fd, "%s %s\n", params[i].par_entry, p);
            _free(p);
          }

          ptr = ptr->next;
        }
      }
      break;

    case PARAM_TRANSPARENT:
      {
        http_proxy *pr = *((http_proxy **)params[i].val_adr);
        if (pr)
        {
          bufio_printf(fd, "%s %s:%d\n", params[i].par_entry, pr->addr, pr->port);
        }
      }
      break;

    case PARAM_PROXY:
      {
        dllist *ptr = *((dllist **)params[i].val_adr);
        while (ptr)
        {
          http_proxy *pr = (http_proxy *)ptr->data;
          bufio_printf(fd, "%s %s:%hu\n", params[i].par_entry, pr->addr, pr->port);

          ptr = ptr->next;
        }
      }
      break;

    case PARAM_FUNC:
      break;

    case PARAM_JSTRANS:
#ifdef HAVE_REGEX
      {
        dllist *ptr;
        for (ptr = cfg.js_transform; ptr; ptr = ptr->next)
        {
          js_transform_t *jt = (js_transform_t *)ptr->data;
          char *p[4];

          if (jt->type != params[i].mdefault_val.l)
            continue;

          p[0] = escape_str(jt->re->pattern, "\"");
          p[1] = escape_str(jt->transform, "\"");
          p[2] = escape_str(jt->tag, "\"");
          p[3] = escape_str(jt->attrib, "\"");

          bufio_printf(fd, "%s \"%s\" \"%s\" \"%s\" \"%s\"\n", params[i].par_entry, p[0], p[1],
                       p[2], p[3]);
          _free(p[0]);
          _free(p[1]);
          _free(p[2]);
          _free(p[3]);
        }
      }
#endif
      break;

    case PARAM_NUMLIST:
      if (!params[i].mval_adr
          || (params[i].mval_adr
              && (*((bool_t *)params[i].mval_adr) == (bool_t)params[i].mdefault_val.l)))
      {
        dllist *ptr = *((dllist **)params[i].val_adr);
        if (ptr)
        {
          bufio_printf(fd, "%s %ld", params[i].par_entry, (long)ptr->data);

          j = 1;
          for ( ; ptr; ptr = ptr->next)
          {
            bufio_printf(fd, ",%ld", (long)ptr->data);
          }
          bufio_puts(fd, "\n");
        }
      }
      break;

    case PARAM_FTPHS:
      {
        dllist *ptr;
        for (ptr = *((dllist **)params[i].val_adr); ptr; ptr = ptr->next)
        {
          char *p, *p2;
          ftp_handshake_info *fhi = (ftp_handshake_info *)ptr->data;

          p = ftp_handshake_info_data_dump(fhi);
          p2 = escape_str(p, "\"");
          _free(p);
          p = p2;
          if (*fhi->host)
            bufio_printf(fd,
                         "%s \"%s:%hu\" \"%s\"\n",
                         params[i].par_entry,
                         fhi->host,
                         fhi->port,
                         p);
          else
            bufio_printf(fd, "%s \"\" \"%s\"\n", params[i].par_entry, p);
          _free(p);
        }
      }
      break;

    case PARAM_TAGPAT:
      {
        dllist *ptr;
        for (ptr = *((dllist **)params[i].val_adr); ptr; ptr = ptr->next)
        {
          char *t, *a, *u;
          tag_pattern_t *tp = (tag_pattern_t *)ptr->data;

          t = escape_str(tp->tag, "\"");
          a = escape_str(tp->attrib, "\"");
          u = escape_str(tp->urlp, "\"");

          bufio_printf(fd, "%s \"%s\" \"%s\" \"%s\"\n", params[i].par_entry, t, a, u);
          _free(t);
          _free(a);
          _free(u);
        }
      }
      break;
    }
  }
  VERIFY(!bufio_release(fd_wl, &fd));
  return 0;
}

int cfg_dump(bufio **file)
{
  if (file && *file && bufio_is_open(*file))
  {
    cfg_dump_fd(*file);
    bufio_close(*file);
    *file = NULL;
#if 0
    /* [i_a] I don't want to quit, just because I saved the scenario! */
    pavuk_do_at_exit();
    exit(PAVUK_EXIT_OK);
#endif
  }
  return 0;
}

int cfg_load_pref(void)
{
  bufio *fd;
  char filename[PATH_MAX];
  int rv = 0;

  snprintf(filename, sizeof(filename), "%s%s%s", cfg.path_to_home,
           (tl_is_dirname(cfg.path_to_home) ? "" : "/"), PAVUK_PREFERENCES_FILENAME);
  if (!(fd = bufio_open(filename, O_BINARY | O_RDONLY)))
  {
    xperror(mk_native(filename));
    return -1;
  }

  cfg_set_all_to_default();
  rv = cfg_load_fd(fd);
  bufio_close(fd);
  fd = NULL;
  return rv;
}

int cfg_dump_pref(void)
{
  bufio *fd;
  char filename[PATH_MAX];

  snprintf(filename, sizeof(filename), "%s%s%s", cfg.path_to_home,
           (tl_is_dirname(cfg.path_to_home) ? "" : "/"), PAVUK_PREFERENCES_FILENAME);
  fd = bufio_copen(filename,
                   O_BINARY | O_CREAT | O_WRONLY,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  if (!fd)
  {
    xperror(mk_native(filename));
    return -1;
  }

  bufio_truncate(fd, 0);
  cfg_dump_fd(fd);
  bufio_close(fd);
  fd = NULL;
  return 0;
}

int cfg_dump_cmd(bufio **file)
{
  int rv = 0;                   /* [i_a] */

  if (file && *file && bufio_is_open(*file))
  {
    rv = cfg_dump_cmd_fd(*file);
    bufio_close(*file);
    *file = NULL;
  }
  return rv;
}

int cfg_dump_cmd_fd(bufio *fd_wl)
{
  int i, j;
  char pom[8192];
  char pom2[20];
  char **pl;
  bufio *fd;

  VERIFY(!bufio_lock(fd_wl, FALSE, &fd));
  bufio_truncate(fd, 0);
  bufio_puts(fd, cfg.prg_path);
  bufio_puts(fd, " ");
  if (cfg.request)
  {
    dllist *dptr;

    for (dptr = cfg.request; dptr; dptr = dptr->next)
    {
      url_info *ui = (url_info *)dptr->data;

      if (ui->type == URLI_NORMAL && !ui->localname)
      {
        bufio_puts_m(fd, " '", ui->urlstr, "' ", NULL);
      }
    }
  }

  for (i = 0; i < NUM_ELEM(params); i++)
  {
    if (params[i].type & PARAM_UNSUPPORTED)
      continue;
    if (params[i].type & PARAM_FOREIGN)
      continue;
    if (!params[i].long_cmd)
      continue;

    if (!params[i].par_entry && (params[i].type != PARAM_PBOOL) && (params[i].type != PARAM_NBOOL))
      continue;

    switch (params[i].type)
    {
    default:
      ASSERT(!"Unsupported param type");
      break;

    case PARAM_FNAME:
      {
        bufio *fdp = *((bufio **)params[i].val_adr);
        if (fdp->filename)
        {
          bufio_printf(fd, " -%s=%s%s ", params[i].long_cmd,
                       ((fdp->flags & BUFIO_F_APPEND) ? "@" : ""), mk_native(fdp->filename));
        }
      }
      break;

    case PARAM_FD:
      {
        bufio *fdp = *((bufio **)params[i].val_adr);
        if (fdp && fdp->filename)
        {
          bufio_printf(fd, " -%s=@%s%s ", params[i].long_cmd,
                       ((fdp->flags & BUFIO_F_APPEND) ? "@" : ""), mk_native(fdp->filename));
        }
        else if (fdp && fdp->fd >= 0)
        {
          bufio_printf(fd, " -%s=%d ", params[i].long_cmd, fdp->fd);
        }
      }
      break;

    case PARAM_BYTE_NUM:
    case PARAM_NUM:
      bufio_printf(fd, " -%s=%ld ", params[i].long_cmd, *((long *)params[i].val_adr));
      break;

    case PARAM_NBOOL:
      if (! * ((bool_t *)params[i].val_adr))
      {
        bufio_puts(fd, " -");
        bufio_puts(fd, params[i].long_cmd);
        bufio_puts(fd, " ");
      }
      break;

    case PARAM_PBOOL:
      if (*((bool_t *)params[i].val_adr))
      {
        bufio_puts(fd, " -");
        bufio_puts(fd, params[i].long_cmd);
        bufio_puts(fd, " ");
      }
      break;

    case PARAM_PORT_RANGE:
      if (*((long *)params[i].val_adr) >= 0)
      {
        bufio_printf(fd, " -%s=%ld:%ld ", params[i].long_cmd, *((long *)params[i].val_adr),
                     *((long *)params[i].mval_adr));
      }
      break;

    case PARAM_PATH:
      if (*((char **)params[i].val_adr))
      {
        bufio_printf(fd, " -%s '%s' ", params[i].long_cmd, mk_native(*((char **)params[i].val_adr)));
      }
      break;

    case PARAM_PATH_NC:
    case PARAM_STR:
    case PARAM_PASS:
      if (*((char **)params[i].val_adr))
      {
        bufio_printf(fd, " -%s '%s' ", params[i].long_cmd, *((char **)params[i].val_adr));
      }
      break;

    case PARAM_STRLIST:
    case PARAM_STRLIST_EX:
      if (!params[i].mval_adr
          || (params[i].mval_adr
              && (*((bool_t *)params[i].mval_adr) == (bool_t)params[i].mdefault_val.l)))
      {
        pl = *((char ***)params[i].val_adr);
        if (pl && pl[0])
        {
          bufio_printf(fd, " -%s '%s", params[i].long_cmd, pl[0]);

          j = 1;
          while (pl[j])
          {
            bufio_puts(fd, ",");
            bufio_puts(fd, pl[j]);
            j++;
          }
          bufio_puts(fd, "' ");
        }
      }
      break;

    case PARAM_CONN:
      if (*((char **)params[i].val_adr))
      {
        bufio_printf(fd, "-%s %s:%hu ", params[i].long_cmd, *((char **)params[i].val_adr),
                     *((int *)params[i].mval_adr));
      }
      break;

    case PARAM_AUTHSCH:
      bufio_printf(fd,
                   " -%s %s ",
                   params[i].long_cmd,
                   http_auths[*((long *)params[i].val_adr)].name);
      break;

    case PARAM_MODE:
      bufio_printf(fd, " -%s %s ", params[i].long_cmd, mode_get_str(cfg.mode));
      break;

    case PARAM_TIME:
      if (*((time_t *)params[i].val_adr))
      {
        LOCK_TIME;
        strftime(pom2, sizeof(pom2), "%Y.%m.%d.%H:%M", localtime((time_t *)params[i].val_adr));
        UNLOCK_TIME;
        bufio_printf(fd, " -%s %s ", params[i].long_cmd, pom2);
      }
      break;

    case PARAM_HTMLTAG:
      {
        int x, y;
        bool_t first = TRUE;

        snprintf(pom, sizeof(pom), " -%s '", params[i].long_cmd);
        pom[sizeof(pom) - 1] = 0;
        for (x = 0; x < html_link_tags_num(); x++)
        {
          for (y = 0; html_link_tags[x].attribs[y].attrib; y++)
          {
            if (!(html_link_tags[x].attribs[y].stat & LINK_DISABLED) == !params[i].default_val.l)
            {
              if (!first)
              {
                strncat(pom, ";", sizeof(pom) - strlen(pom));
                pom[sizeof(pom) - 1] = '\0';
              }
              strncat(pom, html_link_tags[x].tag, sizeof(pom) - strlen(pom));
              pom[sizeof(pom) - 1] = '\0';
              strncat(pom, ",", sizeof(pom) - strlen(pom));
              pom[sizeof(pom) - 1] = '\0';
              strncat(pom, html_link_tags[x].attribs[y].attrib, sizeof(pom) - strlen(pom));
              pom[sizeof(pom) - 1] = '\0';
              first = FALSE;
            }
          }
        }
        strncat(pom, "' ", sizeof(pom) - strlen(pom));
        pom[sizeof(pom) - 1] = '\0';
        if (!first)
        {
          bufio_puts(fd, pom);
        }
      }
      break;

    case PARAM_TWO_QSTR:
      if (*((char **)params[i].val_adr))
      {
        bufio_printf(fd, " -%s '%s' '%s' ", params[i].long_cmd, *((char **)params[i].val_adr),
                     *((char **)params[i].mval_adr));
      }
      break;

    case PARAM_DOUBLE:
      bufio_printf(fd, " -%s=%.3lf ", params[i].long_cmd, *((double *)params[i].val_adr));
      break;

    case PARAM_LFNAME:
      {
        dllist *pdl = cfg.lfnames;
        while (pdl)
        {
          lfname *lfnm = (lfname *)pdl->data;

          bufio_printf(fd,
                       " -%s \'%s\' \'%s\' \'%s\' ",
                       params[i].long_cmd,
                       (lfnm->type == LFNAME_FNMATCH) ? "F" : "R",
                       lfnm->matchstr,
                       lfnm->transstr);

          pdl = pdl->next;
        }
      }
      break;

    case PARAM_RE:
#ifdef HAVE_REGEX
      {
        dllist *ptr = *((dllist **)params[i].val_adr);
        while (ptr)
        {
          re_entry *ree = (re_entry *)ptr->data;

          bufio_printf(fd, " -%s \'%s\' ", params[i].long_cmd, ree->pattern);

          ptr = ptr->next;
        }
      }
#endif
      break;

    case PARAM_USTRAT:
      bufio_printf(fd, " -%s=%s ", params[i].long_cmd,
                   get_strategy_str(*(strategy *)params[i].val_adr));
      break;

    case PARAM_SSLVER:
      bufio_printf(fd, " -%s=%s ", params[i].long_cmd,
                   get_ssl_version_str(*(long *)params[i].val_adr));
      break;

    case PARAM_HTTPHDR:
      {
        dllist *ptr = *((dllist **)params[i].val_adr);
        while (ptr)
        {
          httphdr *hr = ptr->data;

          bufio_printf(fd,
                       " -%s \"%s%s %s\" ",
                       params[i].long_cmd,
                       hr->all ? "+" : "",
                       hr->name,
                       hr->val);

          ptr = ptr->next;
        }
      }
      break;

    case PARAM_DEBUGL:
#ifdef HAVE_DEBUG_FEATURES
      {
        char *strbuf = debug_level_construct(*((unsigned long *)params[i].val_adr));
        bufio_printf(fd, " -%s \'%s\' ", params[i].long_cmd, strbuf);
        _free(strbuf);
      }
#endif
      break;

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
    case PARAM_HAMMERFLAG:
      {
        char *strbuf = hammerflag_level_construct(*((unsigned long *)params[i].val_adr));
        bufio_printf(fd, " -%s \'%s\' ", params[i].long_cmd, strbuf);
        _free(strbuf);
      }
      break;

#endif
    case PARAM_REQUEST:
      {
        dllist *ptr = *((dllist **)params[i].val_adr);
        while (ptr)
        {
          url_info *ui = (url_info *)ptr->data;

          if (ui->type != URLI_NORMAL || ui->localname)
          {
            char *p = url_info_dump(ui);
            bufio_printf(fd, " -%s \'%s\' ", params[i].long_cmd, p);
            _free(p);
          }
          ptr = ptr->next;
        }
      }
      break;

    case PARAM_TRANSPARENT:
      {
        http_proxy *pr = *((http_proxy **)params[i].val_adr);
        if (pr)                  /* [i_a] crash fix */
        {
          bufio_printf(fd, " -%s=%s:%d ", params[i].long_cmd, pr->addr, pr->port);
        }
      }
      break;

    case PARAM_PROXY:
      {
        dllist *ptr = *((dllist **)params[i].val_adr);
        while (ptr)
        {
          http_proxy *pr = (http_proxy *)ptr->data;
          bufio_printf(fd, " -%s=%s:%hu ", params[i].long_cmd, pr->addr, pr->port);
          ptr = ptr->next;
        }
      }
      break;

    case PARAM_FUNC:
      break;

    case PARAM_JSTRANS:
#ifdef HAVE_REGEX
      {
        dllist *ptr;
        for (ptr = cfg.js_transform; ptr; ptr = ptr->next)
        {
          js_transform_t *jt = (js_transform_t *)ptr->data;
          if (jt->type != params[i].mdefault_val.l)
            continue;

          bufio_printf(fd, " -%s \'%s\' \'%s\' \'%s\' \'%s\' ",
                       params[i].long_cmd, jt->re->pattern, jt->transform, jt->tag, jt->attrib);
        }
      }
#endif
      break;

    case PARAM_NUMLIST:
      if (!params[i].mval_adr
          || (params[i].mval_adr
              && (*((bool_t *)params[i].mval_adr) == (bool_t)params[i].mdefault_val.l)))
      {
        dllist *ptr = *((dllist **)params[i].val_adr);
        if (ptr)
        {
          bufio_printf(fd, "-%s %ld", params[i].long_cmd, (long)ptr->data);

          j = 1;
          for ( ; ptr; ptr = ptr->next)
          {
            bufio_printf(fd, ",%ld", (long)ptr->data);
          }
          bufio_puts(fd, " ");
        }
      }
      break;

    case PARAM_FTPHS:
      {
        dllist *ptr;
        for (ptr = *((dllist **)params[i].val_adr); ptr; ptr = ptr->next)
        {
          char *p;
          ftp_handshake_info *fhi = (ftp_handshake_info *)ptr->data;

          p = ftp_handshake_info_data_dump(fhi);
          if (*fhi->host)
          {
            bufio_printf(fd, "-%s \"%s:%hu\" \"%s\" ", params[i].long_cmd, fhi->host, fhi->port, p);
          }
          else
          {
            bufio_printf(fd, "-%s \"\" \"%s\" ", params[i].long_cmd, p);
          }
          _free(p);
        }
      }
      break;

    case PARAM_TAGPAT:
      {
        dllist *ptr;
        for (ptr = *((dllist **)params[i].val_adr); ptr; ptr = ptr->next)
        {
          tag_pattern_t *tp = (tag_pattern_t *)ptr->data;

          bufio_printf(fd,
                       "-%s \"%s\" \"%s\" \"%s\" ",
                       params[i].long_cmd,
                       tp->tag,
                       tp->attrib,
                       tp->urlp);
        }
      }
      break;
    }
  }
  VERIFY(!bufio_release(fd_wl, &fd));

  return 0;
}

void cfg_free_params(void)
{
  int i;

  for (i = 0; i < NUM_ELEM(params); i++)
  {
    cfg_set_to_default(&(params[i]));
  }
}

#if defined (HAVE_MT) && defined (I_FACE)

static const char **_copy_strnar(const char **orig)
{
  int n, i;
  const char **rv;

  if (!orig)
    return NULL;

  for (n = 0; orig[n]; n++)
    ;
  n++;
  rv = (const char **)_malloc(n * sizeof(rv[0]));

  for (i = 0; i < n; i++)
    rv[i] = tl_strdup(orig[i]);

  return rv;
}

#ifdef HAVE_REGEX
static dllist *_copy_relist(dllist *orig)
{
  dllist *rv, *ptr;

  if (!orig)
    return NULL;

  rv = NULL;
  ptr = orig;

  while (ptr)
  {
    rv = dllist_append(rv, re_make(((re_entry *)ptr->data)->pattern));
    ptr = ptr->next;
  }

  return rv;
}

static dllist *_copy_jstrans(dllist *orig)
{
  dllist *rv, *ptr;

  if (!orig)
    return NULL;

  rv = NULL;
  ptr = orig;

  while (ptr)
  {
    js_transform_t *jt = ptr->data;

    rv = dllist_append(rv,
                       js_transform_new(jt->re->pattern, jt->transform, jt->tag, jt->attrib,
                                        jt->type));
    ptr = ptr->next;
  }

  return rv;
}
#endif

static dllist *_copy_lfnames(dllist *orig)
{
  dllist *rv, *ptr;

  if (!orig)
    return NULL;

  rv = NULL;
  ptr = orig;

  while (ptr)
  {
    lfname *ln = (lfname *)ptr->data;
    rv = dllist_append(rv, lfname_new(ln->type, ln->matchstr, ln->transstr));
    ptr = ptr->next;
  }

  return rv;
}

static dllist *_copy_httphdr(dllist *orig)
{
  dllist *rv, *ptr;

  if (!orig)
    return NULL;

  rv = NULL;
  ptr = orig;

  while (ptr)
  {
    httphdr *ov, *nv;

    ov = (httphdr *)ptr->data;
    nv = _malloc(sizeof(*nv));

    nv->all = ov->all;
    nv->name = tl_strdup(ov->name);
    nv->val = tl_strdup(ov->val);

    rv = dllist_append(rv, nv);
    ptr = ptr->next;
  }

  return rv;
}

static dllist *_copy_urlinfo(dllist *orig)
{
  dllist *rv, *ptr;

  if (!orig)
    return NULL;

  rv = NULL;
  ptr = orig;

  while (ptr)
  {
    url_info *ui;

    ui = url_info_duplicate((url_info *)ptr->data);

    rv = dllist_append(rv, ui);
    ptr = ptr->next;
  }

  return rv;
}

static dllist *_copy_numlist(dllist *orig)
{
  dllist *rv = NULL, *ptr;

  for (ptr = orig; ptr; ptr = ptr->next)
    rv = dllist_append(rv, ptr->data);

  return rv;
}

static dllist *_copy_ftphs(dllist *orig)
{
  dllist *rv = NULL;

  for ( ; orig; orig = orig->next)
    rv = dllist_append(rv, ftp_handshake_info_dup(orig->data));

  return rv;
}

static dllist *_copy_tagpat(dllist *orig)
{
  dllist *rv = NULL;

  for ( ; orig; orig = orig->next)
  {
    tag_pattern_t *tp = orig->data;

    rv = dllist_append(rv, tag_pattern_new(tp->type, tp->tag, tp->attrib, tp->urlp));
  }

  return rv;
}

void privcfg_make_copy(_config_struct_priv_t *pcfg)
{
  LOCK_GCFG;

  memset(pcfg, 0, sizeof(_config_struct_priv_t));

  pcfg->timestamp = time(NULL);

  pcfg->default_prefix = tl_strdup(cfg.default_prefix);
  pcfg->info_dir = tl_strdup(cfg.info_dir);
  pcfg->subdir = tl_strdup(cfg.subdir);
  pcfg->cache_dir = tl_strdup(cfg.cache_dir);
  pcfg->post_cmd = tl_strdup(cfg.post_cmd);
  pcfg->http_proxy_pass = tl_strdup(cfg.http_proxy_pass);
  pcfg->http_proxy_user = tl_strdup(cfg.http_proxy_user);
  pcfg->ftp_proxy_pass = tl_strdup(cfg.ftp_proxy_pass);
  pcfg->ftp_proxy_user = tl_strdup(cfg.ftp_proxy_user);
  pcfg->ftp_proxy = tl_strdup(cfg.ftp_proxy);
  pcfg->ftp_proxy_port = cfg.ftp_proxy_port;
  pcfg->gopher_proxy = tl_strdup(cfg.gopher_proxy);
  pcfg->gopher_proxy_port = cfg.gopher_proxy_port;
  pcfg->name_auth = tl_strdup(cfg.name_auth);
  pcfg->passwd_auth = tl_strdup(cfg.passwd_auth);
  pcfg->index_name = tl_strdup(cfg.index_name);
  pcfg->store_name = tl_strdup(cfg.store_name);
  pcfg->from = tl_strdup(cfg.from);
  pcfg->identity = tl_strdup(cfg.identity);
  pcfg->auth_ntlm_domain = tl_strdup(cfg.auth_ntlm_domain);
  pcfg->ftp_list_options = tl_strdup(cfg.ftp_list_options);

  pcfg->accept_lang = _copy_strnar(cfg.accept_lang);
  pcfg->accept_chars = _copy_strnar(cfg.accept_chars);
  pcfg->cookies_disabled_domains = _copy_strnar(cfg.cookies_disabled_domains);
  pcfg->dont_touch_url_pattern = _copy_strnar(cfg.dont_touch_url_pattern);

  pcfg->lfnames = _copy_lfnames(cfg.lfnames);
  pcfg->http_headers = _copy_httphdr(cfg.http_headers);
  pcfg->formdata = _copy_urlinfo(cfg.formdata);
  pcfg->ftp_login_hs = _copy_ftphs(cfg.ftp_login_hs);
  pcfg->condition.tag_patterns = _copy_tagpat(cfg.condition.tag_patterns);

  pcfg->condition.ports = _copy_numlist(cfg.condition.ports);

  pcfg->condition.sites = _copy_strnar(cfg.condition.sites);
  pcfg->condition.allow_site = cfg.condition.allow_site;
  pcfg->condition.sufix = _copy_strnar(cfg.condition.sufix);
  pcfg->condition.allow_sufix = cfg.condition.allow_sufix;
  pcfg->condition.dir_prefix = _copy_strnar(cfg.condition.dir_prefix);
  pcfg->condition.allow_prefix = cfg.condition.allow_prefix;
  pcfg->condition.domains = _copy_strnar(cfg.condition.domains);
  pcfg->condition.allow_domain = cfg.condition.allow_domain;
  pcfg->condition.mime = _copy_strnar(cfg.condition.mime);
  pcfg->condition.allow_mime = cfg.condition.allow_mime;

  pcfg->condition.pattern = _copy_strnar(cfg.condition.pattern);
  pcfg->condition.url_pattern = _copy_strnar(cfg.condition.url_pattern);
  pcfg->condition.skip_pattern = _copy_strnar(cfg.condition.skip_pattern);
  pcfg->condition.skip_url_pattern = _copy_strnar(cfg.condition.skip_url_pattern);

  pcfg->condition.uexit = tl_strdup(cfg.condition.uexit);
  pcfg->condition.follow_cmd = tl_strdup(cfg.condition.follow_cmd);

  pcfg->tr_del_chr = tl_strdup(cfg.tr_del_chr);
  pcfg->tr_str_s1 = tl_strdup(cfg.tr_str_s1);
  pcfg->tr_str_s2 = tl_strdup(cfg.tr_str_s2);
  pcfg->tr_chr_s1 = tl_strdup(cfg.tr_chr_s1);
  pcfg->tr_chr_s2 = tl_strdup(cfg.tr_chr_s2);

#ifdef HAVE_BDB_18x
  pcfg->ns_cache_dir = tl_strdup(cfg.ns_cache_dir);
  pcfg->moz_cache_dir = tl_strdup(cfg.moz_cache_dir);
#endif

#ifdef HAVE_REGEX
  pcfg->advert_res = _copy_relist(cfg.advert_res);
  pcfg->js_patterns = _copy_relist(cfg.js_patterns);
  pcfg->dont_touch_url_rpattern = _copy_relist(cfg.dont_touch_url_rpattern);
  pcfg->dont_touch_tag_rpattern = _copy_relist(cfg.dont_touch_tag_rpattern);

  pcfg->js_transform = _copy_jstrans(cfg.js_transform);

  pcfg->condition.rpattern = _copy_relist(cfg.condition.rpattern);
  pcfg->condition.rskip_pattern = _copy_relist(cfg.condition.rskip_pattern);
  pcfg->condition.rurl_pattern = _copy_relist(cfg.condition.rurl_pattern);
  pcfg->condition.rskip_url_pattern = _copy_relist(cfg.condition.rskip_url_pattern);

  pcfg->condition.aip = _copy_relist(cfg.condition.aip);
  pcfg->condition.skipip = _copy_relist(cfg.condition.skipip);
#endif

#ifdef USE_SSL
  pcfg->ssl_proxy = tl_strdup(cfg.ssl_proxy);
  pcfg->ssl_proxy_port = cfg.ssl_proxy_port;
  pcfg->ssl_cipher_list = tl_strdup(cfg.ssl_cipher_list);
  pcfg->ssl_cert_passwd = tl_strdup(cfg.ssl_cert_passwd);

#ifdef USE_SSL_IMPL_OPENSSL
  pcfg->ssl_cert_file = tl_strdup(cfg.ssl_cert_file);
  pcfg->ssl_key_file = tl_strdup(cfg.ssl_key_file);
  pcfg->egd_socket = tl_strdup(cfg.egd_socket);
#endif
#ifdef USE_SSL_IMPL_NSS
  pcfg->nss_cert_dir = tl_strdup(cfg.nss_cert_dir);
#endif
#endif
  pcfg->bad_content = _copy_strnar(cfg.bad_content);
  UNLOCK_GCFG;
}

#define __free_strnar(orig) _free_strnar(orig); orig = NULL;

static void _free_strnar(const char **orig)
{
  int n;

  if (!orig)
    return;

  for (n = 0; orig[n]; n++)
    _free(orig[n]);
  _free(orig);
}

#ifdef HAVE_REGEX
#define __free_relist(orig) _free_relist(orig); orig = NULL;

static void _free_relist(dllist *orig)
{
  dllist *rv, *ptr;

  if (!orig)
    return;

  rv = NULL;
  ptr = orig;

  while (ptr)
  {
    re_free((re_entry *)ptr->data);
    ptr = dllist_remove_entry(ptr, ptr);
  }
}

#define __free_jstrans(orig) _free_jstrans(orig); orig = NULL;

static void _free_jstrans(dllist *orig)
{
  dllist *rv, *ptr;

  if (!orig)
    return;

  rv = NULL;
  ptr = orig;

  while (ptr)
  {
    js_transform_free((js_transform_t *)ptr->data);
    ptr = dllist_remove_entry(ptr, ptr);
  }
}

#endif

#define __free_lfnames(orig) _free_lfnames(orig); orig = NULL;

static void _free_lfnames(dllist *orig)
{
  dllist *ptr;

  if (!orig)
    return;

  ptr = orig;

  while (ptr)
  {
    lfname_free((lfname *)ptr->data);
    ptr = dllist_remove_entry(ptr, ptr);
  }
}

#define __free_httphdr(orig) _free_httphdr(orig); orig = NULL;

static void _free_httphdr(dllist *orig)
{
  dllist *ptr;

  if (!orig)
    return;

  ptr = orig;

  while (ptr)
  {
    httphdr *ov = (httphdr *)ptr->data;

    _free(ov->name);
    _free(ov->val);

    ptr = dllist_remove_entry(ptr, ptr);
  }
}

#define __free_urlinfo(orig) _free_urlinfo(orig); orig = NULL;

static void _free_urlinfo(dllist *orig)
{
  dllist *ptr;

  if (!orig)
    return;

  ptr = orig;
  while (ptr)
  {
    url_info_free((url_info *)ptr->data);
    ptr->data = NULL;
    ptr = dllist_remove_entry(ptr, ptr);
  }
}

#define __free_numlist(orig) _free_numlist(orig); orig = NULL;

static void _free_numlist(dllist *orig)
{
  while (orig)
    orig = dllist_remove_entry(orig, orig);
}

#define __free_ftphs(orig) _free_ftphs(orig); orig = NULL;

static void _free_ftphs(dllist *orig)
{
  for ( ; orig; orig = dllist_remove_entry(orig, orig))
    ftp_handshake_info_free(orig->data);
}

#define __free_tagpat(orig) _free_tagpat(orig); orig = NULL;

static void _free_tagpat(dllist *orig)
{
  for ( ; orig; orig = dllist_remove_entry(orig, orig))
    tag_pattern_free(orig->data);
}

void privcfg_free(_config_struct_priv_t *pcfg)
{
  _free(pcfg->default_prefix);
  _free(pcfg->info_dir);
  _free(pcfg->subdir);
  _free(pcfg->cache_dir);
  _free(pcfg->post_cmd);
  _free(pcfg->http_proxy_pass);
  _free(pcfg->http_proxy_user);
  _free(pcfg->ftp_proxy_pass);
  _free(pcfg->ftp_proxy_user);
  _free(pcfg->ftp_proxy);
  _free(pcfg->gopher_proxy);
  _free(pcfg->name_auth);
  _free(pcfg->passwd_auth);
  _free(pcfg->index_name);
  _free(pcfg->store_name);
  _free(pcfg->from);
  _free(pcfg->identity);
  _free(pcfg->auth_ntlm_domain);
  _free(pcfg->auth_proxy_ntlm_domain);
  _free(pcfg->ftp_list_options);

  __free_strnar(pcfg->accept_lang);
  __free_strnar(pcfg->accept_chars);
  __free_strnar(pcfg->cookies_disabled_domains);
  __free_strnar(pcfg->dont_touch_url_pattern);

  __free_lfnames(pcfg->lfnames);
  __free_httphdr(pcfg->http_headers);
  __free_urlinfo(pcfg->formdata);
  __free_ftphs(pcfg->ftp_login_hs);
  __free_tagpat(pcfg->condition.tag_patterns);

  __free_numlist(pcfg->condition.ports);

  __free_strnar(pcfg->condition.sites);
  __free_strnar(pcfg->condition.sufix);
  __free_strnar(pcfg->condition.dir_prefix);
  __free_strnar(pcfg->condition.domains);
  __free_strnar(pcfg->condition.mime);

  __free_strnar(pcfg->condition.pattern);
  __free_strnar(pcfg->condition.url_pattern);
  __free_strnar(pcfg->condition.skip_pattern);
  __free_strnar(pcfg->condition.skip_url_pattern);

  _free(pcfg->condition.uexit);
  _free(pcfg->condition.follow_cmd);

  _free(pcfg->tr_del_chr);
  _free(pcfg->tr_str_s1);
  _free(pcfg->tr_str_s2);
  _free(pcfg->tr_chr_s1);
  _free(pcfg->tr_chr_s2);

#ifdef HAVE_BDB_18x
  _free(pcfg->ns_cache_dir);
  _free(pcfg->moz_cache_dir);
#endif

#ifdef HAVE_REGEX
  __free_relist(pcfg->advert_res);
  __free_relist(pcfg->dont_touch_url_rpattern);
  __free_relist(pcfg->dont_touch_tag_rpattern);
  __free_relist(pcfg->js_patterns);
  __free_jstrans(pcfg->js_transform);

  __free_relist(pcfg->condition.rpattern);
  __free_relist(pcfg->condition.rskip_pattern);
  __free_relist(pcfg->condition.rurl_pattern);
  __free_relist(pcfg->condition.rskip_url_pattern);

  __free_relist(pcfg->condition.aip);
  __free_relist(pcfg->condition.skipip);
#endif

#ifdef USE_SSL
  _free(pcfg->ssl_proxy);
  _free(pcfg->ssl_cipher_list);
  _free(pcfg->ssl_cert_passwd);

#ifdef USE_SSL_IMPL_OPENSSL
  _free(pcfg->egd_socket);
  _free(pcfg->ssl_cert_file);
  _free(pcfg->ssl_key_file);
#endif
#ifdef USE_SSL_IMPL_NSS
  _free(pcf->nss_cert_dir);
#endif
#endif
  __free_strnar(pcfg->bad_content);
  memset(pcfg, 0, sizeof(_config_struct_priv_t));
}

#endif /* HAVE_MT && I_FACE */

