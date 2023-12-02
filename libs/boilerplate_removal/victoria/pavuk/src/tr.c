/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "tools.h"
#include "tr.h"

typedef enum
{
  TR_ALPHA,
  TR_ALNUM,
  TR_NUM,
  TR_XNUM,
  TR_SPACE,
  TR_BLANK,
  TR_CTRL,
  TR_PRINTABLE,
  TR_UPPER,
  TR_LOWER,
  TR_PUNCT,
  TR_GRAPH,
  TR_NONPRINTABLE,
  TR_BADCLS
} tr_cls;

typedef struct
{
  char  *name;
  tr_cls cls;
} tr_cls_str;

static const tr_cls_str tr_cls_map[] =
{
  { "[:upper:]", TR_UPPER },
  { "[:lower:]", TR_LOWER },
  { "[:alpha:]", TR_ALPHA },
  { "[:alnum:]", TR_ALNUM },
  { "[:digit:]", TR_NUM },
  { "[:xdigit:]", TR_XNUM },
  { "[:space:]", TR_SPACE },
  { "[:blank:]", TR_BLANK },
  { "[:cntrl:]", TR_CTRL },
  { "[:print:]", TR_PRINTABLE },
  { "[:nprint:]", TR_NONPRINTABLE },
  { "[:punct:]", TR_PUNCT },
  { "[:graph:]", TR_GRAPH }
};

static int tr_is_cls(int ch, tr_cls cls)
{
  /* ch = 0: char ASCII(0) is not part of ANY class! It is a string sentinel only! */
  if (ch == 0)
    return FALSE;

  switch (cls)
  {
  case TR_ALPHA:
    return tl_ascii_isalpha(ch);

  case TR_ALNUM:
    return tl_ascii_isalnum(ch);

  case TR_NUM:
    return tl_ascii_isdigit(ch);

  case TR_XNUM:
    return tl_ascii_isxdigit(ch);

  case TR_SPACE:
    return tl_ascii_isspace(ch);

  case TR_BLANK:
    return tl_ascii_isblank(ch);

  case TR_CTRL:
    return tl_ascii_iscntrl(ch);

  case TR_PRINTABLE:
    return tl_ascii_isprint(ch);

  case TR_UPPER:
    return tl_ascii_isupper(ch);

  case TR_LOWER:
    return tl_ascii_islower(ch);

  case TR_PUNCT:
    return tl_ascii_ispunct(ch);

  case TR_GRAPH:
    return tl_ascii_isgraph(ch);

  case TR_NONPRINTABLE:
    return !tl_ascii_isprint(ch);

  default:
    return FALSE;
  }
}

/*
 * Copy any character X, which is part of character class 'cls', to destination 'p'.
 *
 * Return the number of characters copied to 'p'.
 */
static unsigned int tr_append_cls(char *p, tr_cls cls)
{
  int i;
  char *orig = p;

  /* i = 1: char ASCII(0) is not part of ANY class! It is a string sentinel only! */
  for (i = 1; i <= 255; i++)
  {
    if (tr_is_cls(i, cls))
    {
      *p++ = (unsigned char)i;
    }
  }
  *p = 0;

  return p - orig;
}

/*
 * Return the UNescaped character code as a return value, while
 * the '*p' is shifted to the next character in the source string
 * (i.e. the decoded escape is skipped).
 *
 * This routine ASSUMES '*p' points at the '\\' escape character.
 *
 * These escapes are recognized:
 *
 \r
 \n
 \t
 \0xnn (hex number representing a char)
 \0Xnn (hex number representing a char)
 *
 * Other escape sequences are not decoded: \c will produce the
 * character 'c' as is.
 */
static char tr_get_escaped_str(const char **p)
{
  const char *ps = *p;
  char rc;

  switch (ps[1])
  {
  case 'n':
    rc = '\n';
    (*p)++;
    break;

  case 'r':
    rc = '\r';
    (*p)++;
    break;

  case 't':
    rc = '\t';
    (*p)++;
    break;

  case '0':
    if (ps[2] == 'x' || ps[2] == 'X')
    {
      if (tl_ascii_isxdigit(ps[3]) && tl_ascii_isxdigit(ps[4]))
      {
        rc = HEX2CHAR(ps + 3);
        (*p) += 4;
      }
      else
        rc = ps[0];
    }
    else
      rc = ps[0];
    break;

  default:
    rc = ps[1];
    (*p)++;
  }

  return rc;
}

/*
 * UNescape the string 'str' as a return value.
 *
 * Return the number of decoded characters (i.e. the
 * length of the returned string) in the optional
 * '*rlen' value ('rlen' MAY be NULL).
 */
static char *tr_expand_str(const char *str, int *rlen)
{
  const char *p;
  char *pom;
  int i;
  int dstlen;

  if (str == NULL)
    return NULL;

  /* worst case output: each set has maximum size for 8-bit characters */
  dstlen = strlen(str) + 1 + str_cnt_chr(str, '[') * 255;
  pom = malloc(dstlen);
  pom[0] = 0;

  for (i = 0, p = str; *p; p++)
  {
    switch (*p)
    {
    case '\\':
      pom[i++] = tr_get_escaped_str(&p);
      pom[i] = 0;
      break;

    case '[':
      {
        int j;
        tr_cls cls = TR_BADCLS;

        for (j = 0; j < NUM_ELEM(tr_cls_map); j++)
        {
          if (!strncmp(p, tr_cls_map[j].name, strlen(tr_cls_map[j].name)))
          {
            cls = tr_cls_map[j].cls;
            p += strlen(tr_cls_map[j].name) - 1;
            break;
          }
        }
        if (cls != TR_BADCLS)
        {
          i += tr_append_cls((pom + i), cls);
        }
        else
        {
          pom[i++] = *p;
          pom[i] = 0;
        }
      }
      break;

    case '-':
      {
        int strtc;
        int endc;
        int pc;

        if (i)
          strtc = ((unsigned char *)pom)[i - 1] + 1;
        else
          strtc = 0;

        if (*(p + 1))
        {
          p++;
          if (*p == '\\')
          {
            endc = tr_get_escaped_str(&p);
          }
          else
            endc = ((unsigned char *)p)[0];
        }
        else
        {
          endc = 255;           /* [i_a] \255 is NOT 255, it's OCTAL 255 */
        }

        for (pc = strtc; pc <= endc; pc++)
        {
          pom[i++] = (unsigned char)pc;
        }
      }
      pom[i] = 0;
      break;

    default:
      pom[i++] = *p;
      pom[i] = 0;
      break;
    }
    ASSERT(strlen(pom) < dstlen);
  }

  if (rlen)
    *rlen = i;

  return pom;
}

/*
 * Convert every character in the set 'fset' (from) occurring
 * in the input string 'str' to the corresponding character
 * in the set 'tset' (to).
 *
 * The characters in the sets 'fset' and 'fset' have a 1:1
 * relationship, e.g. the second character in 'fset' will
 * be replaced by the second character in 'tset'.
 *
 * CAVEAT:
 * If the set 'tset' is smaller than the set 'fset', any
 * characters in the set 'fset' at positions at or beyond the
 * size of the set 'tset' will be replaced by the last character
 * in the set 'tset'. For example,
 *
 *   tr_chr_chr('abcd', 'AB', 'abcde')
 *
 * will produce the result
 *
 *   'ABBBe'
 *
 * as 'c' and 'd' in 'fset' are beyond the range of 'tset', hence
 * these are replaced by the last character in 'tset': 'B'.
 */
char *tr_chr_chr(const char *fset, const char *tset, const char *str)
{
  const char *p;
  char *d;
  int i;
  int tsetlen = strlen(tset);
  char *retv = tl_strdup(str);

  for (p = str, d = retv; *p; p++, d++)
  {
    for (i = 0; fset[i]; i++)
    {
      if (fset[i] == *p)
      {
        *d = tset[(tsetlen > i) ? i : (tsetlen - 1)];
        break;
      }
    }
  }

  return retv;
}

char *tr_del_chr(const char *set, const char *str)
{
  const char *p;
  char *d;
  char *retv = tl_strdup(str);

  if (!set)
    return retv;

  for (p = str, d = retv; *p; p++)
  {
    if (NULL != strchr(set, *p))
    {
      *d++ = *p;
    }
  }
  *d = 0;

  return retv;
}

char *tr_str_str(const char *s1, const char *s2, const char *str)
{
  const char *p = str;
  const char *p1;
  char *retv;
  int i = 0;

  while (p)
  {
    if ((p = strstr(p, s1)))
    {
      i++;
      p += strlen(s1);
    }
  }

  i = 1 + strlen(str) - i *strlen(s1) + i * (s2 ? strlen(s2) : 0);
  retv = (char *)_malloc(i);
  memset(retv, 0, i);

  p = p1 = str;

  while (p1)
  {
    p1 = strstr(p, s1);
    if (p1)
    {
      strncat(retv, p, p1 - p);
      if (s2)
        strcat(retv, s2);
    }
    else
    {
      strcat(retv, p);
    }
    p = p1 + strlen(s1);
  }

  return retv;
}

/*
 * Transform the input string 'str' according to the rules set at the
 * pavuk configuration (i.e. the command line or GUI):
 *
 * cfg.tr_str_s1
 * cfg.tr_str_s2
 * cfg.tr_del_chr
 * cfg.tr_chr_s1
 * cfg.tr_chr_s2
 *
 * See also these pavuk command line options:
 *
 *    -tr_del_chr $str   - characters that will be deleted
 *
 *    -tr_str_str $str1 $str2
 *                       - translate $str1 to $str2
 *
 *    -tr_chr_chr $chrset1 $chrset2
 *                       - translate $chrset1 to $chrset2
 */
char *tr(const char *str)
{
  char *p1, *p2;
  char *s1, *s2;

  if (priv_cfg.tr_str_s1 && priv_cfg.tr_str_s2)
  {
    p1 = tr_str_str(priv_cfg.tr_str_s1, priv_cfg.tr_str_s2, str);
  }
  else
  {
    p1 = tl_strdup(str);
  }

  if (priv_cfg.tr_del_chr)
  {
    s1 = tr_expand_str(priv_cfg.tr_del_chr, NULL);
    p2 = tr_del_chr(s1, p1);
    _free(s1);
    _free(p1);
  }
  else
  {
    p2 = p1;
  }

  if (priv_cfg.tr_chr_s1 && priv_cfg.tr_chr_s2)
  {
    s1 = tr_expand_str(priv_cfg.tr_chr_s1, NULL);
    s2 = tr_expand_str(priv_cfg.tr_chr_s2, NULL);
    p1 = tr_chr_chr(s1, s2, p2);
    _free(s1);
    _free(s2);
    _free(p2);
  }
  else
  {
    p1 = p2;
  }

  return p1;
}

