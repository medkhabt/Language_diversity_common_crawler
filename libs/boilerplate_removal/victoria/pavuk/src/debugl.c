/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#include "debugl.h"
#include "tools.h"
#include "string.h"

#ifdef HAVE_DEBUG_FEATURES

debug_level_type cfg_debug_levels[] = 
{
  {~0UL, "all", NULL}
  ,
  {DLV1, "html", gettext_nop("HTML parsers")}
  ,
  {DLV2, "protos", gettext_nop("Server responses")}
  ,
  {DLV3, "protoc", gettext_nop("Client requests")}
  ,
  {DLV4, "procs", gettext_nop("Procedure calling")}
  ,
  {DLV5, "locks", gettext_nop("File locking")}
  ,
  {DLV6, "net", gettext_nop("Networking code")}
  ,
  {DLV7, "misc", gettext_nop("Miscelanous")}
  ,
  {DLV8, "user", gettext_nop("Extended user infos")}
  ,
  {DLV9, "mtlock", gettext_nop("Multithreading - locking")}
  ,
  {DLV10, "mtthr", gettext_nop("Multithreading - threads")}
  ,
  {DLV11, "protod", gettext_nop("POST request data")}
  ,
  {DLV12, "limits", gettext_nop("Limiting conditions")}
  ,
  {DLV13, "ssl", gettext_nop("SSL informations")}
  ,
//#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  {DLV14, "hammer", gettext_nop("HAMMER action info")}
  ,
//#endif
  {DLV15, "trace", gettext_nop("TRACE data for a developer")}
  ,
  {DLV16, "dev", gettext_nop("developer info - should be moved to other levels ASAP")}
  ,
  {DLV17, "bufio", gettext_nop("developer info - bufio handling")}
  ,
  {DLV18, "cookie", gettext_nop("cookie handling")}
  ,
  {DLV19, "htmlform", gettext_nop("html FORM processing")}
  ,
  {DLV20, "rules", gettext_nop("fnrules and JavaScript processing")}
  ,
  {0, NULL, NULL}
};

int debug_level_parse(const char *str, unsigned long *dst)
{
  unsigned int dl = 0;
  const char *p = str;
  int l, i;
  bool_t found;

  ASSERT(dst != NULL);
  *dst = 0;
  while(*p)
  {
    p += strspn(p, ",");
    l = strcspn(p, ",");
    found = FALSE;
    for(i = 0; cfg_debug_levels[i].id; i++)
    {
      if(!strncasecmp(p, cfg_debug_levels[i].option, l) && (l == strlen(cfg_debug_levels[i].option)))
      {
        found = TRUE;
        dl |= cfg_debug_levels[i].id;
        break;
      }
      else if(*p == '!' && (!strncasecmp(p + 1, cfg_debug_levels[i].option, l - 1) && (l - 1 == strlen(cfg_debug_levels[i].option))))
      {
        found = TRUE;
        dl &= ~cfg_debug_levels[i].id;
        break;
      }
    }
    if(!found)
    {
      char *ep;

      i = strtol(p, &ep, 0);

      if((ep - p) != l)
      {
        xprintf(0, gettext("Bad debug level selection: %s. Cannot decode '%s' remainder\n"), str, p);
        return -1;
      }
      else
      {
        dl |= i;
      }
    }
    p += l;
  }

  *dst = dl;
  return 0;
}

char *debug_level_construct(unsigned long level)
{
  bool_t frst = TRUE;
  int i;
  char *strbuf = NULL;
  unsigned long orig_level = level;

  if(!level)
  {
    return tl_strdup("0");
  }

  for(i = 0; cfg_debug_levels[i].id; i++)
  {
    if((cfg_debug_levels[i].id & level) != cfg_debug_levels[i].id)
      continue;

    if(!frst)
      strbuf = tl_str_concat(DBGvars(strbuf), ",", NULL);
    strbuf = tl_str_concat(DBGvars(strbuf), cfg_debug_levels[i].option, NULL);
    frst = FALSE;
    level &= ~cfg_debug_levels[i].id;
  }
  if(level)
  {
    char *sbufbuf = strbuf;

    /* if any bits are left, try to encode the levels as 'all,!x,!y,!z,!... first: */
    level = orig_level ^ ~0;
    ASSERT(level != 0);

    strbuf = tl_strdup("all");
    for(i = 0; cfg_debug_levels[i].id; i++)
    {
      if((cfg_debug_levels[i].id & level) != cfg_debug_levels[i].id)
        continue;
      strbuf = tl_str_concat(DBGvars(strbuf), ",!", cfg_debug_levels[i].option, NULL);
      level &= ~cfg_debug_levels[i].id;
    }
    if(level)
    {
      char buf[20];

      _free(strbuf);
      strbuf = sbufbuf;
      sprintf(buf, "%lu", level);
      if(strbuf && *strbuf)
        strbuf = tl_str_concat(DBGvars(strbuf), ",", NULL);
      strbuf = tl_str_concat(DBGvars(strbuf), buf, NULL);
    }
    else
    {
      _free(sbufbuf);
    }
  }
  return strbuf;
}

#endif /* HAVE_DEBUG_FEATURES */

#ifndef __GNUC__

void DEBUG_HTML(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[HTML]", str, NULL);
  xvadebug(DLV1, str, &args);
  _free(str);
  va_end(args);
#endif
}

void DEBUG_PROTOS(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[PROTOS]  ", str, NULL);
  xvadebug(DLV2, str, &args);
  _free(str);
  va_end(args);
#endif
}

void DEBUG_PROTOC(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[PROTOC]  ", str, NULL);
  xvadebug(DLV3, str, &args);
  _free(str);
  va_end(args);
#endif
}

void DEBUG_PROCS(char *str)
{
#ifdef HAVE_DEBUG_FEATURES
  xdebug(DLV4, "[PROCS]   ", "calling - %s\n", str);
#endif
}

void DEBUG_PROCE(char *str)
{
#ifdef HAVE_DEBUG_FEATURES
  xdebug(DLV4, "[PROCE]   ", "exiting - %s\n", str);
#endif
}

void DEBUG_LOCKS(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[LOCKS]   ", str, NULL);
  xvadebug(DLV5, str, &args);
  _free(str);
  va_end(args);
#endif
}

void DEBUG_NET(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[NET]     ", str, NULL);
  xvadebug(DLV6, str, &args);
  _free(str);
  va_end(args);
#endif
}

void DEBUG_MISC(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[MISC]    ", str, NULL);
  xvadebug(DLV7, str, &args);
  _free(str);
  va_end(args);
#endif
}

void DEBUG_USER(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[USER]    ", str, NULL);
  xvadebug(DLV8, str, &args);
  _free(str);
  va_end(args);
#endif
}

void DEBUG_MTLOCK(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[MTLOCK]  ", str, NULL);
  xvadebug(DLV9, str, &args);
  _free(str);
  va_end(args);
#endif
}

void DEBUG_MTTHR(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[MTTHR]   ", str, NULL);
  xvadebug(DLV10, str, &args);
  _free(str);
  va_end(args);
#endif
}

void DEBUG_PROTOD(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[PROTOD]  ", str, NULL);
  xvadebug(DLV11, str, &args);
  _free(str);
  va_end(args);
#endif
}

void DEBUG_LIMITS(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[LIMITS]  ", str, NULL);
  xvadebug(DLV12, str, &args);
  _free(str);
  va_end(args);
#endif
}
void DEBUG_SSL(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[SSL]     ", str, NULL);
  xvadebug(DLV13, str, &args);
  _free(str);
  va_end(args);
#endif
}

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
void DEBUG_HAMMER(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[HAMMER]  ", str, NULL);
  xvadebug(DLV14, str, &args);
  _free(str);
  va_end(args);
#endif
}
#endif

void DEBUG_TRACE(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[TRACE]   ", str, NULL);
  xvadebug(DLV15, str, &args);
  _free(str);
  va_end(args);
#endif
}

void DEBUG_DEVEL(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[DEV]     ", str, NULL);
  xvadebug(DLV16, str, &args);
  _free(str);
  va_end(args);
#endif
}

void DEBUG_BUFIO(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[BUFIO]   ", str, NULL);
  xvadebug(DLV17, str, &args);
  _free(str);
  va_end(args);
#endif
}

void DEBUG_COOKIE(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[COOKIE]  ", str, NULL);
  xvadebug(DLV18, str, &args);
  _free(str);
  va_end(args);
#endif
}

void DEBUG_HTMLFORM(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[HTMLFORM]", str, NULL);
  xvadebug(DLV19, str, &args);
  _free(str);
  va_end(args);
#endif
}

void DEBUG_RULES(char *str, ...)
{
#ifdef HAVE_DEBUG_FEATURES
  va_list args;
  va_start(args, str);
  str = tl_str_concat(DBGvars(NULL), "[DEV]     ", str, NULL);
  xvadebug(DLV20, str, &args);
  _free(str);
  va_end(args);
#endif
}

#endif /* !__GNUC__ */
