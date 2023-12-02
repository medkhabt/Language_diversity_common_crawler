/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "lfname.h"
#include "url.h"
#include "tools.h"
#include "tr.h"
#include "dlhash_tools.h"
#include "http.h"
#include "form.h"
#include "mime.h"
#include "jsbind.h"

enum lfname_lsp_type
{
  LF_LSP_UNKNOWN,         /*** unknown ***/
  LF_LSP_STR,             /*** string variable ***/
  LF_LSP_NUM,             /*** number variable ***/
  LF_LSP_MACRO,           /*** macro variable ***/
  LF_LSP_SUB,             /*** subpart from regex ***/
  LF_LSP_SC,              /*** strcat function ***/
  LF_LSP_SS,              /*** substr function ***/
  LF_LSP_HASH,            /*** hash function ***/
  LF_LSP_MD5,             /*** md5 function ***/
  LF_LSP_LOWER,           /*** lowerstr function ***/
  LF_LSP_UPPER,           /*** upperstr function ***/
  LF_LSP_UENC,            /*** urlencode function ***/
  LF_LSP_UDEC,            /*** urldecode function ***/
  LF_LSP_DELCHR,          /*** delchr function ***/
  LF_LSP_TRCHR,           /*** trchr function ***/
  LF_LSP_TRSTR,           /*** trstr function ***/
  LF_LSP_STRSPN,          /*** strspn function ***/
  LF_LSP_STRCSPN,         /*** strcspn function ***/
  LF_LSP_STRLEN,          /*** strlen function ***/
  LF_LSP_NRSTR,           /*** nrtostr function ***/
  LF_LSP_STRNR,           /*** strtonr function ***/
  LF_LSP_LCHR,            /*** last character offset ***/
  LF_LSP_PLS,             /*** plus ***/
  LF_LSP_MNS,             /*** minus ***/
  LF_LSP_MOD,             /*** mod ***/
  LF_LSP_MUL,             /*** multiply ***/
  LF_LSP_DIV,             /*** divide ***/
  LF_LSP_REMOVEPARAMETER, /*** removes a parameter from an url string */
  LF_LSP_GETVALUE,        /*** reads a value from a parameter of an url string */
  LF_LSP_SIF,             /*** if condition ***/
  LF_LSP_NOT,             /*** logical not ***/
  LF_LSP_AND,             /*** logical and ***/
  LF_LSP_OR,              /*** logical or ***/
  LF_LSP_GETEXT,          /*** get extension from path ***/
  LF_LSP_JSF,             /*** result of JavaScript function ***/
  LF_LSP_SEQ,             /*** string equal ***/
  LF_LSP_FNSEQ,           /*** fnmatch() string equal ***/
};

struct lfname_lsp_var
{
  enum lfname_lsp_type type;
  union
  {
    char *str;
    int   num;
    char  macro;
  }                    val;
  enum lfname_lsp_type rettype;
  union
  {
    char *str;
    int   num;
  }                      ret_val;
  struct lfname_lsp_var *param1;
  struct lfname_lsp_var *param2;
  struct lfname_lsp_var *param3;
};


static char *lfname_lsp_get_by_url(struct lfname_lsp_interp *);
static struct lfname_lsp_var *lfname_lsp_analyze(const char **);
static void lfname_lsp_var_free(struct lfname_lsp_var *);

#ifdef HAVE_REGEX
/*
 * Return the substring of 'urlstr' as matched by the Regex subexpression 'nr'
 * when the '-fnrules R' statement was matched.
 *
 * CAVEAT:
 *
 * 'urlstr' MUST match the string used while matching the Regex or
 * undefined behaviour will occur:
 *
 * This routine assumes a previous call to lfname_match() using the same 'urlstr'
 * content.
 */
char *lfname_re_sub(lfname *lfnamep, const char *urlstr, int nr)
{
  char pom[4096];

  pom[0] = 0;

  if (lfnamep->type != LFNAME_REGEX)
    return tl_strdup(pom);

#ifdef HAVE_POSIX_REGEX
  {
    regmatch_t *pmatch = lfnamep->pmatch;
    if (nr >= 0 && nr <= lfnamep->preg.re_nsub
        && (pmatch[nr].rm_eo - pmatch[nr].rm_so > 0))
    {
      strncpy(pom, urlstr + pmatch[nr].rm_so, pmatch[nr].rm_eo - pmatch[nr].rm_so);
      pom[pmatch[nr].rm_eo - pmatch[nr].rm_so] = 0;
    }
  }
#elif defined (HAVE_V8_REGEX)
#ifdef HAVE_V8_REGSUB
  {
    char ssect[10];
    if (nr)
      sprintf(ssect, "\\%d", nr);
    else
      strcpy(ssect, "&");
    regsub(lfnamep->preg, ssect, pom);
  }
#endif
#elif defined (HAVE_GNU_REGEX)
  if (nr >= 0 && nr < lfnamep->preg.re_nsub
      && (lfnamep->pmatch.end[nr] - lfnamep->pmatch.start[nr] > 0))
  {
    strncpy(pom,
            urlstr + lfnamep->pmatch.start[nr],
            lfnamep->pmatch.end[nr] - lfnamep->pmatch.start[nr]);
    pom[lfnamep->pmatch.end[nr] - lfnamep->pmatch.start[nr]] = 0;
  }
#elif defined (HAVE_PCRE_REGEX)
  if (nr >= 0 && nr < lfnamep->pmatch_nr
      && (lfnamep->pmatch[2 * nr + 1] - lfnamep->pmatch[2 * nr] > 0))
  {
    strncpy(pom, urlstr + lfnamep->pmatch[2 * nr],
            lfnamep->pmatch[2 * nr + 1] - lfnamep->pmatch[2 * nr]);
    pom[lfnamep->pmatch[2 * nr + 1] - lfnamep->pmatch[2 * nr]] = 0;
  }
#elif defined (HAVE_TRE_REGEX)
  {
    regmatch_t *pmatch = lfnamep->pmatch;
    if (nr >= 0 && nr <= lfnamep->preg.re_nsub
        && (pmatch[nr].rm_eo - pmatch[nr].rm_so > 0))
    {
      strncpy(pom, urlstr + pmatch[nr].rm_so, pmatch[nr].rm_eo - pmatch[nr].rm_so);
      pom[pmatch[nr].rm_eo - pmatch[nr].rm_so] = 0;
    }
  }
#endif
  return tl_strdup(pom);
}
#endif

/*
 * Execute the simple/extended rule in 'lfnamep' as defined by the '-fnrules' command line
 * argument (see man page for more info).
 *
 * Supported 'simple rule' macros:
 *
 *   $x - x-th match section
 *   %i - protocol id
 *   %p - password
 *   %u - user name
 *   %h - host name
 *   %m - domain name
 *   %r - port number
 *   %d - doc path
 *   %n - doc name
 *   %b - base name of document
 *   %e - extension
 *   %s - search string
 *   %q - POST query string
 *   %M - mime type
 *   %B - basic mime type, i.e. mime type without ';'-separated attributes
 *   %A - mime type attributes
 *   %E - extension by mime type
 *   %X - file extension or extension by mime type if file extension
 *            does not exist or does not match mime type
 *   %Y - file extension or extension by mime type if file extension
 *            does not exist
 *   %o - default doc name
 *   %-x - x-th dirname from end
 *   %x - x-th dirname from start
 *   %U - URL string
 *
 *
 * CAVEAT:
 *
 * 'urlstr' MUST match the string used while matching the Regex or
 * undefined behaviour will occur:
 *
 * This routine assumes a previous call to lfname_match() using the same 'urlstr'
 * content.
 */
char *lfname_get_by_url(url        *urlp,
                        const char *urlstr,
                        const char *local_name,
                        const char *mime_type,
                        lfname     *lfnamep)
{
  char *ps, *pd, *pp;
  const char *p1, *p2;
  char pom[4096];
  char pstr[4096];
  int nr;
  const char *n, *d, *t, *e, *b, *m, *q, *o, *h, *mb, *ma, *s, *pw, *usr;
  const char *mimeext;
  char *retv = NULL;

  p2 = url_get_path(urlp);

  if (urlp->type == URLT_GOPHER)
  {
    if (urlp->p.gopher.selector[0] == '1')
      snprintf(pstr, sizeof(pstr), "/%s/%s", urlp->p.gopher.selector, priv_cfg.index_name);
    else
      snprintf(pstr, sizeof(pstr), "/%s", urlp->p.gopher.selector);
    pstr[sizeof(pstr) - 1] = 0;
  }
  else if (tl_is_dirname(p2)
           || ((urlp->type == URLT_FTP || urlp->type == URLT_FTPS) && urlp->p.ftp.dir))
  {
    snprintf(pstr, sizeof(pstr), "%s%s%s", p2, (tl_is_dirname(p2) ? "" : "/"), priv_cfg.index_name);
    pstr[sizeof(pstr) - 1] = 0;
  }
  else
  {
    strncpy(pstr, p2, sizeof(pstr));
    pstr[sizeof(pstr) - 1] = 0;
  }

  t = get_abs_file_path(pstr);

  strncpy(pstr, t, sizeof(pstr));
  pstr[sizeof(pstr) - 1] = 0;

  p1 = strrchr(pstr, '/');

  d = p1 ? tl_strndup(pstr, p1 - pstr) : tl_strdup("");

  n = p1 ? tl_strdup(p1 + 1) : tl_strdup(pstr);

  e = tl_strdup(tl_get_extension(pstr));

  p1 = strrchr(n, '.');

  if (p1)
    b = tl_strndup(n, p1 - n);
  else
    b = tl_strdup(n);

  h = url_get_site(urlp);
  if (!h)
    h = "";
  m = h;
  p1 = strchr(m, '.');
  if (p1)
    m = p1 + 1;

  q = NULL;
  if (urlp->status & URL_FORM_ACTION)
  {
    form_info *fi = (form_info *)urlp->extension;

    p1 = form_encode_urlencoded(fi->infos);
    if (p1)
    {
      strncpy(pstr, p1, sizeof(pstr) - 1);
      pstr[sizeof(pstr) - 1] = 0;
      q = tl_strdup(pstr);
    }
    _free(p1);
  }
  if (!q)
    q = tl_strdup("");

  s = url_get_search_str(urlp);
  if (!s)
    s = "";

  o = tl_strdup(local_name);

  mb = NULL;
  ma = NULL;
  if (mime_type)
  {
    char *p = tl_strdup(mime_type);
    char *q;

    mb = p;
    q = strchr(p, ';');
    if (q)
    {
      *q++ = 0;
      ma = q;
      for (q -= 2; q >= p && tl_ascii_isspace(*q); *q-- = 0)
        ;
      for ( ; *ma && tl_ascii_isspace(*ma); ma++)
        ;
    }
  }
  mimeext = mime_get_type_ext(mb);

  pw = url_get_pass(urlp, NULL);
  usr = url_get_user(urlp, NULL);

#ifdef HAVE_DEBUG_FEATURES
  if (cfg.debug && (DEBUG_RULES_BITMASK & cfg.debug_level))
  {
    char *re_str;
    char *rule_stmt;

    re_str = escape_str(lfnamep->matchstr, "\"");
    rule_stmt = escape_str(lfnamep->transstr, "\"");

    DEBUG_RULES("Apply fnrule for matching URL:\n"
                "- URL:                     '%s'\n"
                "- MIME type:               '%s'\n"
				"- MIME preferred file ext: '%s'\n"
                "- URL path:                '%s'\n"
                "- (initial) local name:    '%s'\n"
                "- abs file path:           '%s'\n"
				"- scheme:                  '%s'\n"
				"- user:                    '%s'\n"
				"- password:                '%s'\n"
				"- base name:               '%s'\n"
				"- extension:               '%s'\n"
				"- query:                   '%s'\n"
				"- POST query:              '%s'\n"
                "- FNRULE: type           = '%s'\n"
                "-         RE (escaped)   = '%s'\n"
                "-         rule (escaped) = '%s'\n",
                urlstr,
                (mime_type ? mime_type : "---"),
				(mimeext ? mimeext : "---"),
                p2,
                (local_name ? local_name : "---"),
                t,
				prottable[urlp->type].dirname,
				(usr ? usr: "---"),
				(pw ? pw : "---"),
				b,
				e,
				s,
				q,
                (lfnamep->type == LFNAME_UNKNOWN
                 ? "U"
                 : (lfnamep->type == LFNAME_FNMATCH
                    ? "F" : "R")),
                re_str,
                rule_stmt
    );
    _free(re_str);
    _free(rule_stmt);
  }
#endif

  pom[0] = 0;

  if (lfnamep->transstr[0] == '(')
  {
    struct lfname_lsp_interp interp;
    char port[10];

    DEBUG_RULES("Processing macros in 'extended fnrules' style\n");

    interp.urlp = urlp;
    interp.urlstr = urlstr;
    interp.scheme = prottable[urlp->type].dirname;
	interp.passwd = (pw ? pw : "");
	interp.user = (usr ? usr: "");
    interp.host = h;
    interp.domain = m;
    sprintf(port, "%d", url_get_port(urlp));
    interp.port = port;
    interp.path = d;
    interp.name = n;
    interp.basename = b;
    interp.extension = e;
    interp.query = s;
    interp.post_query = q;
    interp.deflt = o;
    interp.mime_type = mime_type;
    interp.basic_mime_type = mb;
    interp.mime_type_attribs = ma;
    interp.mime_type_ext = mimeext;
    interp.orig = lfnamep;

    retv = lfname_lsp_get_by_url(&interp);
  }
  else
  {
    DEBUG_RULES("Processing macros in 'simple fnrules' style\n");

    pom[sizeof(pom) - 1] = 0;            /* [i_a] */
    for (ps = lfnamep->transstr, pd = pom; *ps; ps++)
    {
      int override_extension = FALSE;

      if (sizeof(pom) < (pd - pom) + 2)
        break;/* [i_a] abortus provocatus to prevent buffer overflow */

      if (! * (ps + 1))
      {
        *pd = *ps;
        pd++;
        *pd = 0;
        continue;
      }
      switch (*ps)
      {
      case '\\':
        ps++;
        *pd = *ps;
        pd++;
        *pd = 0;
        break;

#ifdef HAVE_REGEX
      case '$':
        ps++;
        nr = strtol(ps, &pp, 10);
        p1 = lfname_re_sub(lfnamep, urlstr, nr);
        strncpy(pd, p1, sizeof(pom) - (pd - pom));
        pom[sizeof(pom) - 1] = 0;    /* [i_a] */
        DEBUG_RULES("Apply $x macro: submatch %d ('%s') --> '%s'\n", nr, p1, pom);
        _free(p1);
        while (*pd)
          pd++;
        ps = pp - 1;
        break;

#endif
      case '%':
        ps++;
        pstr[0] = 0;
        DEBUG_RULES("Apply %%%c macro:", *ps);
        switch (*ps)
        {
        case 'i':
          strncpy(pstr, prottable[urlp->type].dirname, sizeof(pstr));
          break;

        case 'p':
          strncpy(pstr, url_get_pass(urlp, NULL) ? url_get_pass(urlp, NULL) : "", sizeof(pstr));
          break;

        case 'u':
          strncpy(pstr, url_get_user(urlp, NULL) ? url_get_user(urlp, NULL) : "", sizeof(pstr));
          break;

        case 'h':
          strncpy(pstr, h, sizeof(pstr));
          break;

        case 'm':
          strncpy(pstr, m, sizeof(pstr));
          break;

        case 'r':
          sprintf(pstr, "%d", url_get_port(urlp));
          break;

        case 't':
          strncpy(pstr, t, sizeof(pstr));
          break;

        case 'd':
          strncpy(pstr, d, sizeof(pstr));
          break;

        case 'n':
          strncpy(pstr, n, sizeof(pstr));
          break;

        case 'b':
          strncpy(pstr, b, sizeof(pstr));
          break;

        case 'e':
          strncpy(pstr, e ? e : "", sizeof(pstr));
          break;

        case 's':
          strncpy(pstr, s, sizeof(pstr));
          break;

        case 'q':
          strncpy(pstr, q, sizeof(pstr));
          break;

        case 'M':
          strncpy(pstr, mime_type ? mime_type : "", sizeof(pstr));
          break;

        case 'B':
          strncpy(pstr, mb ? mb : "", sizeof(pstr));
          break;

        case 'A':
          strncpy(pstr, ma ? ma : "", sizeof(pstr));
          break;

        case 'E':
          strncpy(pstr, mimeext ? mimeext : "", sizeof(pstr));
          break;

        case 'Y':
          override_extension = TRUE;

          /* fall through! */
        case 'X':
          if (e && *e && (override_extension ? TRUE : mime_ext_matches_type(e, mb, mimeext)))
          {
            /*
             * if existing extension matches any of the extensions listed for the given mime type,
             * it's OK to use, otherwise override by using the first extension for this mime type:
             */
            strncpy(pstr, e, sizeof(pstr));
          }
          else if (mimeext && *mimeext)
          {
            strncpy(pstr, mimeext, sizeof(pstr));
          }
          else if (e && *e)
          {
		    if (mb && !override_extension)
			{
				ASSERT(mime_type);
		        xprintf(0, gettext("WARNING: no default extension is known for the MIME type '%s'.\n"
					               "         (Full MIME type: '%s')\n"
					               "         You may wish to specify an augmented mime.types(5) file using the\n"
								   "         -mime_types_file command line option.\n"), 
					mb, mime_type);
			}
            strncpy(pstr, e, sizeof(pstr));
          }
          break;

        case 'o':
          strncpy(pstr, o, sizeof(pstr));
          break;

        case '-':
          nr = strtol(ps + 1, &pp, 10);
          p1 = strrfindnchr(d, '/', nr);
          p2 = strrfindnchr(d, '/', nr + 1);
          if (!p1)
            pstr[0] = 0;
          else if (p2)
          {
            strncpy(pstr, p2 + 1, p1 - 1 - p2); /* FIXME: Security */
            *(pstr + (p1 - 1 - p2)) = 0;
          }
          else
            pstr[0] = 0;
          ps = pp - 1;
          break;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          nr = strtol(ps, &pp, 10);
          p1 = strfindnchr(d, '/', nr);
          p2 = strfindnchr(d, '/', nr + 1);
          if (!p1)
            pstr[0] = 0;
          else if (p2)
          {
            strncpy(pstr, p1 + 1, p2 - 1 - p1); /* FIXME: Security */
            *(pstr + (p2 - 1 - p1)) = 0;
          }
          else
            strncpy(pstr, p1 + 1, sizeof(pstr));
          ps = pp - 1;
          break;

        default:
          pstr[0] = *(ps - 1);
          pstr[1] = *ps;
          pstr[2] = 0;
        }
        pstr[sizeof(pstr) - 1] = 0;
        strncat(pd, pstr, sizeof(pom) - (pd - pom));
        pom[sizeof(pom) - 1] = 0;    /* [i_a] */
        DEBUG_RULES("('%s') --> '%s'\n", pstr, pom);
        while (*pd)
          pd++;
        break;

      default:
        *pd = *ps;
        pd++;
        *pd = 0;
      }
    }
    retv = tl_strdup(pom);
  }
  _free(e);
  _free(n);
  _free(t);
  _free(d);
  _free(q);
  _free(o);
  _free(b);
  _free(mb);

  DEBUG_RULES("macros produced raw result (before cleanup): '%s'\n", retv);

  /* clean up path: */
  for (ps = pd = retv; *ps; ps++)
  {
    if (*ps == '/' && ps[1] == '/')
    {
      /* replace "//" by '/' to make sure this is a proper path */
      *pd++ = *ps++;
    }
    else if (*ps == '.' && (ps[1] == '/' || ps[1] == 0))
    {
      /* skip '.' dot at end of directory or file names */
    }
    else
    {
      *pd++ = *ps;
    }
  }
  *pd = 0;

  DEBUG_RULES("macros produced result (cleaned): '%s'\n", retv);

  return retv;
}

void lfname_free(lfname *lfnamep)
{
#ifdef HAVE_REGEX
  if (lfnamep->type == LFNAME_REGEX)
  {
#ifdef HAVE_POSIX_REGEX
    regfree(&(lfnamep->preg));
    _free(lfnamep->pmatch);
#elif defined (HAVE_V8_REGEX)
    _free(lfnamep->preg);
#elif defined (HAVE_GNU_REGEX)
    regfree(&lfnamep->preg);
    _free(lfnamep->pmatch.start);
    _free(lfnamep->pmatch.end);
#elif defined (HAVE_PCRE_REGEX)
    _free(lfnamep->preg);
    _free(lfnamep->preg_extra);
    _free(lfnamep->pmatch);
#elif defined (HAVE_TRE_REGEX)
    regfree(&(lfnamep->preg));
    _free(lfnamep->pmatch);
#endif
  }
#endif

  _free(lfnamep->matchstr);
  _free(lfnamep->transstr);
  _free(lfnamep);
}


lfname *lfname_new(lfname_type type, const char *mpt, const char *str)
{
  lfname *rv;
  const char *p;

  rv = _malloc(sizeof(*rv));
  rv->type = type;
  rv->matchstr = NULL;
  rv->transstr = NULL;
#ifdef HAVE_REGEX
  if (type == LFNAME_REGEX)
  {
#ifdef HAVE_POSIX_REGEX
    int ec;
    if ((ec = regcomp(&(rv->preg), mpt, REG_EXTENDED)))
    {
      char pom[PATH_MAX];
      xprintf(0, gettext("Error compiling regular expression : %s\n"), mpt);
      regerror(ec, &(rv->preg), pom, sizeof(pom));
      xprintf(0, "%s\n", pom);
      regfree(&(rv->preg));
      _free(rv);
      return NULL;
    }
    rv->pmatch = _malloc((rv->preg.re_nsub + 1) * sizeof(regmatch_t));
#elif defined (HAVE_V8_REGEX)
    if (!(rv->preg = regcomp(mpt)))
    {
      xprintf(0, gettext("Error compiling regular expression : %s\n"), mpt);
      _free(rv->preg);
      _free(rv);
      return NULL;
    }
#elif defined (HAVE_BSD_REGEX)
    if ((p = re_comp(mpt)))
    {
      xprintf(0, gettext("Error compiling regular expression : %s\n"), mpt);
      xprintf(0, "%s", p);
      _free(rv);
      return NULL;
    }
#elif defined (HAVE_GNU_REGEX)
    rv->preg.allocated = 0;
    rv->preg.buffer = NULL;
    rv->preg.fastmap = NULL;
    re_set_syntax(r_2phase_star);
    if ((p = re_compile_pattern(mpt, strlen(mpt), &rv->preg)))
    {
      xprintf(0, gettext("Error compiling regular expression : %s\n"), mpt);
      xprintf(0, "%s\n", p);
      regfree(&(rv->preg));
      _free(rv);
      return NULL;
    }
    rv->pmatch.start = _malloc((rv->preg.re_nsub + 1) * sizeof(*rv->pmatch.start));
    rv->pmatch.end = _malloc((rv->preg.re_nsub + 1) * sizeof(*rv->pmatch.end));
    rv->pmatch.num_regs = rv->preg.re_nsub + 1;
    rv->preg.regs_allocated = REGS_FIXED;
#elif defined (HAVE_PCRE_REGEX)
    int errcode = 0;

    if ((rv->preg = pcre_compile(mpt, 0, (const char **)&p, &errcode, NULL)))
    {
      rv->preg_extra = pcre_study(rv->preg, 0, (const char **)&p);
      pcre_fullinfo(rv->preg, rv->preg_extra, PCRE_INFO_CAPTURECOUNT, &rv->pmatch_nr);
      rv->pmatch_nr++;
      rv->pmatch = (int *)_malloc(rv->pmatch_nr * 3 * sizeof(int));
    }
    else
    {
      xprintf(0, gettext("Error compiling regular expression : %s\n"), mpt);
      xprintf(0, "%s\n", p);
      _free(rv);
      return NULL;
    }
#elif defined (HAVE_TRE_REGEX)
    int ec;
    if ((ec = regcomp(&(rv->preg), mpt, REG_EXTENDED)))
    {
      char pom[PATH_MAX];
      xprintf(0, gettext("Error compiling regular expression : %s\n"), mpt);
      regerror(ec, &(rv->preg), pom, sizeof(pom));
      xprintf(0, "%s\n", pom);
      regfree(&(rv->preg));
      _free(rv);
      return NULL;
    }
    rv->pmatch = _malloc((rv->preg.re_nsub + 1) * sizeof(regmatch_t));
#endif
  }
#endif
  if (str[0] == '(')
  {
    struct lfname_lsp_var *variant;
    p = str;
    if ((variant = lfname_lsp_analyze(&p)))
    {
      lfname_lsp_var_free(variant);
      if (*p)
      {
        xprintf(0, gettext("LSP analyze error: bad token at - %s\n"), p);
        lfname_free(rv);
        return NULL;
      }
      else
      {
        rv->transstr = tl_strdup(str);
      }
    }
    else
    {
      lfname_free(rv);
      return NULL;
    }
  }
  else
  {
    rv->transstr = tl_strdup(str);
  }
  rv->matchstr = tl_strdup(mpt);
  return rv;
}

/*
 * Try to match the 'urlstr' string against the match expression 'lfnamep'.
 *
 * Match expressions can be of either 'F' (fnmatch) or 'R' (regex) type, as
 * defined for the '-fnrules' command line argument. (See the man page for
 * more info.)
 *
 * Return TRUE if the 'urlstr' string matches the expression, FALSE otherwise.
 */
int lfname_match(lfname *lfnamep, const char *urlstr)
{
  int retv = 0;

#ifdef HAVE_REGEX
  if (lfnamep->type == LFNAME_REGEX)
  {
#ifdef HAVE_POSIX_REGEX
    retv = !regexec(&(lfnamep->preg), urlstr, lfnamep->preg.re_nsub + 1, lfnamep->pmatch, 0);

#elif defined (HAVE_V8_REGEX)
    retv = regexec(lfnamep->preg, urlstr);

#elif defined (HAVE_BSD_REGEX)
    re_comp(lfnamep->matchstr);
    retv = re_exec(urlstr);

#elif defined (HAVE_GNU_REGEX)
    retv = re_match(&(lfnamep->preg), urlstr, strlen(urlstr), 0, &lfnamep->pmatch) >= 0;

#elif defined (HAVE_PCRE_REGEX)
    retv = pcre_exec(lfnamep->preg, lfnamep->preg_extra, urlstr, strlen(
                       urlstr), 0, 0, lfnamep->pmatch, 3 * lfnamep->pmatch_nr) >= 0;

#elif defined (HAVE_TRE_REGEX)
    retv = !regexec(&(lfnamep->preg), urlstr, lfnamep->preg.re_nsub + 1, lfnamep->pmatch, 0);

#endif
  }
  else
#endif
  retv = !fnmatch(lfnamep->matchstr, urlstr, 0);

#ifdef HAVE_DEBUG_FEATURES
  if (cfg.debug && (DEBUG_RULES_BITMASK & cfg.debug_level))
  {
    char *re_str;

    re_str = escape_str(lfnamep->matchstr, "\"");

    DEBUG_RULES("Matching fnrule(type: %s, RE (escaped): '%s') for URL ('%s') --> %d (%s)\n",
                (lfnamep->type ==
                 LFNAME_UNKNOWN ? "U" : (lfnamep->type == LFNAME_FNMATCH ? "F" : "R")),
                re_str,
                urlstr,
                retv,
                (retv ? "SUCCESS" : "FAIL")
    );
    _free(re_str);
  }
#endif

  return retv;
}

/*
 * Validate the rule string 'str', assuming it is a
 * -fnrules 'extended rule' statement.
 *
 * Return TRUE on success, FALSE on failure to parse
 * the statement.
 *
 * Note: validation is performed by parsing the
 *       complete statement.
 */
int lfname_check_rule(const char *str)
{
  if (str[0] == '(')
  {
    const char *p = str;
    struct lfname_lsp_var *variant;

    if ((variant = lfname_lsp_analyze(&p)))
    {
      lfname_lsp_var_free(variant);
      if (*p)
      {
        xprintf(0, gettext("LSP analyze error: bad token at - %s\n"), p);
        return FALSE;
      }
      else
      {
        return TRUE;
      }
    }
    else
    {
      return FALSE;
    }
  }
  return TRUE;
}


/*
 * Validate the check pattern 'str', assuming it is a
 * -fnrules check pattern of type 'type' (Regex/Fnmatch).
 *
 * Return TRUE on success, FALSE on failure to parse
 * the statement.
 *
 * Note: validation is performed by compiling the Regex.
 *       Fnmatch patterns are ALWAYS assumed to be correct,
 *       unless their length is zero.
 */
int lfname_check_pattern(lfname_type type, const char *str)
{
#ifdef HAVE_REGEX
  if (type == LFNAME_REGEX)
  {
#ifdef HAVE_POSIX_REGEX
    int ec;
    char pom[PATH_MAX];
    regex_t preg;

    ec = regcomp(&preg, str, REG_EXTENDED);

    if (ec)
    {
      xprintf(0, gettext("Error compiling regular expression: %s\n"), str);
      regerror(ec, &preg, pom, sizeof(pom));
      xprintf(0, "%s\n", pom);
    }
    regfree(&preg);
    return !ec;

#elif defined (HAVE_V8_REGEX)
    regexp *preg;

    preg = regcomp(str);
    if (!preg)
    {
      xprintf(0, gettext("Error compiling regular expression: %s\n"), str);
    }
    else
    {
      _free(preg);
    }
    return preg != NULL;

#elif defined (HAVE_BSD_REGEX)
    char *p;

    p = re_comp(str);

    if (p)
    {
      xprintf(0, gettext("Error compiling regular expression: %s\n"), str);
      xprintf(0, "%s", p);
    }
    return p == NULL;

#elif defined (HAVE_GNU_REGEX)
    char *p;
    struct re_pattern_buffer preg;

    preg.allocated = 0;
    preg.buffer = NULL;
    preg.fastmap = NULL;

    if ((p = re_compile_pattern(str, strlen(str), &preg)))
    {
      xprintf(0, gettext("Error compiling regular expression: %s\n"), str);
      xprintf(0, "%s\n", p);
    }
    regfree(&preg);
    return p == NULL;

#elif defined (HAVE_PCRE_REGEX)
    pcre *re;
    const char *errmsg = NULL;
    int errcode = 0;

    if (!(re = pcre_compile(str, 0, &errmsg, &errcode, NULL)))
    {
      xprintf(0, gettext("Error compiling regular expression: %s\n"), str);
      xprintf(0, "%s\n", errmsg);
      return -1;
    }
    else
    {
      _free(re);
    }

    return re != NULL;

#elif defined (HAVE_TRE_REGEX)
    int ec;
    char pom[PATH_MAX];
    regex_t preg;

    ec = regcomp(&preg, str, REG_EXTENDED);
    if (ec)
    {
      xprintf(0, gettext("Error compiling regular expression: %s\n"), str);
      regerror(ec, &preg, pom, sizeof(pom));
      xprintf(0, "%s\n", pom);
    }
    regfree(&preg);
    return !ec;

#endif
  }
  else
#endif
  return strlen(str) > 0;
}

/*
 * Return the current value from 'interp' for the -fnrules % macro 'macro'.
 *
 * This function is used when processing -fnrules 'extended rule' statements.
 *
 * CAVEAT:
 *
 * This routine may return NULL.
 */
const char *lfname_interp_get_macro(struct lfname_lsp_interp *interp, int macro)
{
  int override_extension = FALSE;

  switch (macro)
  {
  case 'i':
    return interp->scheme;

  case 'p':
    return interp->passwd;

  case 'u':
    return interp->user;

  case 'h':
    return interp->host;

  case 'm':
    return interp->domain;

  case 'r':
    return interp->port;

  case 'd':
    return interp->path;

  case 'n':
    return interp->name;

  case 'b':
    return interp->basename;

  case 'e':
    return interp->extension;

  case 's':
    return interp->query;

  case 'q':
    return interp->post_query;

  case 'U':
    return interp->urlstr;

  case 'o':
    return interp->deflt;

  case 'M':
    return interp->mime_type;

  case 'B':
    return interp->basic_mime_type;

  case 'A':
    return interp->mime_type_attribs;

  case 'E':
    return interp->mime_type_ext;

  case 'Y':
    override_extension = TRUE;

    /* fall through! */
  case 'X':
    if (interp->extension && *interp->extension
        && (override_extension
            ? TRUE
            : mime_ext_matches_type(interp->extension,
                                    interp->basic_mime_type,
                                    interp->mime_type_ext)))
    {
      /*
       * if existing extension matches any of the extensions listed for the given mime type,
       * it's OK to use, otherwise override by using the first extension for this mime type:
       */
      return interp->extension;
    }
    else if (interp->mime_type_ext && *interp->mime_type_ext)
    {
      return interp->mime_type_ext;
    }
    if (interp->basic_mime_type && !override_extension)
	{
		ASSERT(interp->mime_type);
        xprintf(0, gettext("WARNING: no default extension is known for the MIME type '%s'.\n"
			               "         (Full MIME type: '%s')\n"
			               "         You may wish to specify an augmented mime.types(5) file using the\n"
						   "         -mime_types_file command line option.\n"), 
				interp->basic_mime_type, interp->mime_type);
	}
    return interp->extension;
  }
  return NULL;
}

/*
 * Return TRUE if 'macro' is a valid -fnrules % macro.
 *
 * Return FALSE otherwise.
 *
 * This function is used when processing -fnrules 'extended rule' statements.
 */
int lfname_check_macro(int macro)
{
  return strchr("ipuhmrdnbesUoqEMXYAB", macro) != NULL;
}

static const struct
{
  enum lfname_lsp_type type;
  enum lfname_lsp_type rettype;
  char                *name;
  short                params;
  enum lfname_lsp_type p1type;
  enum lfname_lsp_type p2type;
  enum lfname_lsp_type p3type;
} lfname_lsp_ftbl[] =
{
  {
    LF_LSP_UNKNOWN, LF_LSP_UNKNOWN, NULL, 0, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN
  },
  {
    LF_LSP_STR, LF_LSP_STR, NULL, 0, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN
  },
  {
    LF_LSP_NUM, LF_LSP_NUM, NULL, 0, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN
  },
  {
    LF_LSP_MACRO, LF_LSP_STR, NULL, 0, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN
  },
  {
    /* return URL subpart from the matching -fnrules 'R' regex: sp(number) */
    LF_LSP_SUB, LF_LSP_STR, "sp ", 1, LF_LSP_NUM, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN
  },
  {
    /* concat two string parameters: concat(str1, str2) */
    LF_LSP_SC, LF_LSP_STR, "sc ", 2, LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN
  },
  {
    /* substring from string: substring(str, start, end) */
    LF_LSP_SS, LF_LSP_STR, "ss ", 3, LF_LSP_STR, LF_LSP_NUM, LF_LSP_NUM
  },
  {
    /* compute modulo hash value from string with specified base: hash(str, base) */
    LF_LSP_HASH, LF_LSP_NUM, "hsh ", 2, LF_LSP_STR, LF_LSP_NUM, LF_LSP_UNKNOWN
  },
  {
    /* compute MD5 checksum for string: md5(str) */
    LF_LSP_MD5, LF_LSP_STR, "md5 ", 1, LF_LSP_STR, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN
  },
  {
    /* convert all characters inside string to lower case: lower(str) */
    LF_LSP_LOWER, LF_LSP_STR, "lo ", 1, LF_LSP_STR, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN
  },
  {
    /* convert all characters inside string to upper case: upper(str) */
    LF_LSP_UPPER, LF_LSP_STR, "up ", 1, LF_LSP_STR, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN
  },
  {
    /* encode unsafe characters in string: URLencode(str, unsafe_chars) */
    LF_LSP_UENC, LF_LSP_STR, "ue ", 2, LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN
  },
  {
    /* Decode any URL entities to the actual characters: URLdecode(str) */
    LF_LSP_UDEC, LF_LSP_STR, "ud ", 1, LF_LSP_STR, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN
  },
  {
    /* delete unwanted characters from string: delete(str, chars_to_remove) */
    LF_LSP_DELCHR, LF_LSP_STR, "dc ", 2, LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN
  },
  {
    /* replace character with other character in string: tr_chr_chr(str, chars_to_replace, replacement_chars) */
    LF_LSP_TRCHR, LF_LSP_STR, "tc ", 3, LF_LSP_STR, LF_LSP_STR, LF_LSP_STR
  },
  {
    /* replace some string inside string with any other string: tr_str_str(str, str_to_replace, replacement_str) */
    LF_LSP_TRSTR, LF_LSP_STR, "ts ", 3, LF_LSP_STR, LF_LSP_STR, LF_LSP_STR
  },
  {
    /* calculate initial length of string which contains only specified set of characters: strspn(str, char_set) */
    LF_LSP_STRSPN, LF_LSP_NUM, "spn ", 2, LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN
  },
  {
    /* calculate initial length of string which doesn't contain specified set of characters: strcspn(str, char_set) */
    LF_LSP_STRCSPN, LF_LSP_NUM, "cspn ", 2, LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN
  },
  {
    /* calculate length of string: strlen(str) */
    LF_LSP_STRLEN, LF_LSP_NUM, "sl ", 1, LF_LSP_STR, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN
  },
  {
    /* convert number to string by format: printf(fmt, number) */
    LF_LSP_NRSTR, LF_LSP_STR, "ns ", 2, LF_LSP_STR, LF_LSP_NUM, LF_LSP_UNKNOWN
  },
  {
    /* convert string to number by radix: strtol(str, radix_number) */
    LF_LSP_STRNR, LF_LSP_NUM, "sn ", 2, LF_LSP_STR, LF_LSP_NUM, LF_LSP_UNKNOWN
  },
  {
    /* return position of last occurrence of specified character inside string: strrchrs(str, char_set) */
    LF_LSP_LCHR, LF_LSP_NUM, "lc ", 2, LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN
  },
  {
    /* add two numeric values */
    LF_LSP_PLS, LF_LSP_NUM, "+ ", 2, LF_LSP_NUM, LF_LSP_NUM, LF_LSP_UNKNOWN
  },
  {
    /* subtract two numeric values */
    LF_LSP_MNS, LF_LSP_NUM, "- ", 2, LF_LSP_NUM, LF_LSP_NUM, LF_LSP_UNKNOWN
  },
  {
    /* calculate modulo remainder: mod(num, divisor) */
    LF_LSP_MOD, LF_LSP_NUM, "% ", 2, LF_LSP_NUM, LF_LSP_NUM, LF_LSP_UNKNOWN
  },
  {
    /* multiply two numeric values */
    LF_LSP_MUL, LF_LSP_NUM, "* ", 2, LF_LSP_NUM, LF_LSP_NUM, LF_LSP_UNKNOWN
  },
  {
    /* divide two numeric values: divide(num, divisor) */
    LF_LSP_DIV, LF_LSP_NUM, "/ ", 2, LF_LSP_NUM, LF_LSP_NUM, LF_LSP_UNKNOWN
  },
  {
    /* remove parameter from query string: rmpar(url_str, param_name) */
    LF_LSP_REMOVEPARAMETER, LF_LSP_STR, "rmpar ", 2, LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN
  },
  {
    /* get query string parameter value: getqstr(url_str, param_name) */
    LF_LSP_GETVALUE, LF_LSP_STR, "getval ", 2, LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN
  },
  {
    /* logical decision: sif(number, str_for_zero, str_for_nonzero) */
    LF_LSP_SIF, LF_LSP_STR, "sif ", 3, LF_LSP_NUM, LF_LSP_STR, LF_LSP_STR
  },
  {
    /* logical not: not(number) */
    LF_LSP_NOT, LF_LSP_NUM, "! ", 1, LF_LSP_NUM, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN
  },
  {
    /* logical and */
    LF_LSP_AND, LF_LSP_NUM, "& ", 2, LF_LSP_NUM, LF_LSP_NUM, LF_LSP_UNKNOWN
  },
  {
    /* logical or */
    LF_LSP_OR, LF_LSP_NUM, "| ", 2, LF_LSP_NUM, LF_LSP_NUM, LF_LSP_UNKNOWN
  },
  {
    /* get file extension: getext(filepath) */
    LF_LSP_GETEXT, LF_LSP_STR, "getext ", 1, LF_LSP_STR, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN
  },
  {
    /* execute JavaScript function: jsf(js_func_name) */
    LF_LSP_JSF, LF_LSP_STR, "jsf ", 1, LF_LSP_STR, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN
  },
  {
    /* compare two strings: 0 is diff, 1 is equal */
    LF_LSP_SEQ, LF_LSP_NUM, "seq ", 2, LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN
  },
  {
    /* compare two string parameters: fnmatch(wildcarded_str, str_to_match) */
    LF_LSP_FNSEQ, LF_LSP_STR, "fnseq ", 2, LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN
  },
};

/*
 * Parse the next token in a -fnrules 'extended rule' statement.
 *
 * '*pstr' is moved forward to the next token.
 *
 * CAVEAT:
 *
 * - In case this code parses a '%' macro escape character, '*pstr' is
 *   ACTUALLY pointing at the macro character itself (one of the set as known by
 *   lfname_interp_get_macro()/lfname_check_macro())!
 *
 * - In case a STRING literal was found, '*pstr' only skipped the initial
 *   " double quote, and is thus pointing at the start of the literal string
 *       value itself.
 *
 * - Likewise, when this parser code found a literal NUMBER, '*pstr' points
 *   at the start of the value itself.
 */
static enum lfname_lsp_type lfname_lsp_token_type(const char **pstr)
{
  const char *p = *pstr;
  enum lfname_lsp_type retv = LF_LSP_UNKNOWN;

  while (*p == ' ')
    p++;

  if (*p == '(')
  {
    int i;
    for (i = 0; i < NUM_ELEM(lfname_lsp_ftbl); i++)
    {
      if (lfname_lsp_ftbl[i].name
          && !strncmp(p + 1, lfname_lsp_ftbl[i].name, strlen(lfname_lsp_ftbl[i].name)))
      {
        retv = lfname_lsp_ftbl[i].type;
        p += 1 + strlen(lfname_lsp_ftbl[i].name);
        break;
      }
    }
  }
  else if (*p == '\"')
  {
    retv = LF_LSP_STR;
    p++;
  }
  else if (*p == '%')
  {
    retv = LF_LSP_MACRO;
    p++;
  }
  else if (tl_ascii_isdigit(*p) || *p == '-')
  {
    retv = LF_LSP_NUM;
  }

  *pstr = p;

  return retv;
}

static struct lfname_lsp_var *lfname_lsp_var_new(enum lfname_lsp_type type)
{
  struct lfname_lsp_var *retv = NULL;

  retv = _malloc(sizeof(*retv));
  retv->type = type;
  retv->val.str = NULL;
  retv->rettype = lfname_lsp_ftbl[type].rettype;
  retv->ret_val.str = NULL;
  retv->param1 = NULL;
  retv->param2 = NULL;
  retv->param3 = NULL;

  return retv;
}

static void lfname_lsp_var_ret_free(struct lfname_lsp_var *var)
{
  if (!var)
    return;

  lfname_lsp_var_ret_free(var->param1);
  lfname_lsp_var_ret_free(var->param2);
  lfname_lsp_var_ret_free(var->param3);
  if (var->rettype == LF_LSP_STR)
    _free(var->ret_val.str);
}

static void lfname_lsp_var_free(struct lfname_lsp_var *var)
{
  if (!var)
    return;

  lfname_lsp_var_free(var->param1);
  lfname_lsp_var_free(var->param2);
  lfname_lsp_var_free(var->param3);
  if (var->type == LF_LSP_STR)
    _free(var->val.str);
  _free(var);
}

/*
 * Stack-based LISP like parser, which produces a statement list from the program
 * specified in '*str'.
 *
 * Returns the parsed token list as a return value and increments '*str' to point at the
 * end of the program (or a non-NUL character position in the program where the parser
 * has hit a syntax error).
 */
static struct lfname_lsp_var *lfname_lsp_analyze(const char **pstr)
{
  struct lfname_lsp_var *retv = NULL;
  enum lfname_lsp_type type;
  char *p;
  const char *cp;

  type = lfname_lsp_token_type(pstr);

  switch (type)
  {
  case LF_LSP_UNKNOWN:
    xprintf(0, gettext("LSP analyze error: bad token at - %s\n"), *pstr);
    break;

  case LF_LSP_NUM:
    {
      int nval;

      errno = 0;
      nval = strtol(*pstr, &p, 0);

      if ((errno == ERANGE) || (*p != 0 && *p != ')' && *p != ' '))
      {
        xprintf(0, gettext("LSP analyze error: bad numeric value at - %s\n"), *pstr);
        break;
      }
      retv = lfname_lsp_var_new(type);
      retv->val.num = nval;
      *pstr = p;
    }
    break;

  case LF_LSP_MACRO:
    {
      if (!lfname_check_macro(**pstr)
          || (*(*pstr + 1) != 0 && *(*pstr + 1) != ')' && *(*pstr + 1) != ' '))
      {
        xprintf(0, gettext("LSP analyze error: bad macro at - %s\n"), *pstr - 1);
        break;
      }
      retv = lfname_lsp_var_new(type);
      retv->val.macro = **pstr;
      *pstr += 1;
    }
    break;

  case LF_LSP_STR:
    {
      char *tmp = _malloc(strlen(*pstr) + 1);
      char *tp;

      cp = *pstr;
      tp = tmp;

      while (*cp)
      {
        if (*cp == '\"')
          break;
        if (*cp == '\\')
          cp++;
        *tp = *cp;
        tp++;
        if (*cp)
          cp++;
      }
      *tp = 0;

      if (*cp != '\"')
      {
        xprintf(0, gettext("LSP analyze error: unterminated string at - %s\n"), *pstr - 1);
        break;
      }
      retv = lfname_lsp_var_new(type);
      retv->val.str = tl_strdup(tmp);
      _free(tmp);
      *pstr = cp + 1;
    }
    break;

  default:
    {
      struct lfname_lsp_var *p1 = NULL;
      struct lfname_lsp_var *p2 = NULL;
      struct lfname_lsp_var *p3 = NULL;
      if (lfname_lsp_ftbl[type].params >= 1)
      {
        cp = *pstr;
        p1 = lfname_lsp_analyze(pstr);
        if (!p1)
          break;
        if (p1->rettype != lfname_lsp_ftbl[type].p1type)
        {
          xprintf(0, gettext("LSP analyze error: bad parameter type at - %s\n"), cp);
          lfname_lsp_var_free(p1);
          break;
        }
      }
      if (p1 && lfname_lsp_ftbl[type].params >= 2)
      {
        cp = *pstr;
        p2 = lfname_lsp_analyze(pstr);
        if (!p2)
        {
          lfname_lsp_var_free(p1);
          break;
        }
        if (p2->rettype != lfname_lsp_ftbl[type].p2type)
        {
          xprintf(0, gettext("LSP analyze error: bad parameter type at - %s\n"), cp);
          lfname_lsp_var_free(p1);
          lfname_lsp_var_free(p2);
          break;
        }
      }
      if (p2 && lfname_lsp_ftbl[type].params >= 3)
      {
        cp = *pstr;
        p3 = lfname_lsp_analyze(pstr);
        if (!p3)
        {
          lfname_lsp_var_free(p1);
          lfname_lsp_var_free(p2);
          break;
        }
        if (p3->rettype != lfname_lsp_ftbl[type].p3type)
        {
          xprintf(0, gettext("LSP analyze error: bad parameter type at - %s\n"), cp);
          lfname_lsp_var_free(p1);
          lfname_lsp_var_free(p2);
          lfname_lsp_var_free(p3);
          break;
        }
      }
      while (**pstr == ' ')
        (*pstr)++;
      if (**pstr != ')')
      {
        xprintf(0, gettext("LSP analyze error: bad token at - %s\n"), *pstr);
        if (p1)
          lfname_lsp_var_free(p1);
        if (p2)
          lfname_lsp_var_free(p2);
        if (p3)
          lfname_lsp_var_free(p3);
      }
      else
      {
        (*pstr)++;
        retv = lfname_lsp_var_new(type);
        retv->param1 = p1;
        retv->param2 = p2;
        retv->param3 = p3;
      }
    }
    break;
  }

  return retv;
}

/*
 * Removes a parameter from an URL-String.
 * e.g. removeparameter("myurl.php3?var=something","var") will convert the
 * URL to "myurl.php3?"
 */
static char *lfname_fn_removeparameter(char *urlstr, char *var)
{
  char *p, *found;
  int pos1;
  int parlen;

  /* &var= */
  p = tl_str_concat(DBGvars(NULL), "&", var, "=", NULL);
  parlen = strlen(p);

  found = strstr(urlstr, p);
  if (!found)
  {
    /* ?var= */
    *p = '?';
    found = strstr(urlstr, p);
    if (!found)
    {
      /* var= */
      if ((parlen > 1) && !strncmp(urlstr, p + 1, parlen - 1))
      {
        found = urlstr;
      }
    }
  }
  _free(p);

  if (!found)
    return tl_strdup(urlstr);

  pos1 = found - urlstr + 1;

  found = strstr((urlstr + pos1 + 1), "&");
  if (!found)
  {
    return tl_strndup(urlstr, pos1 - 1);
  }

  urlstr = tl_strndup(urlstr, pos1);

  return tl_str_concat(DBGvars(urlstr), found + 1, NULL);
}

/*
 * reads a value from parameter of an URL-String.
 * e.g. lfname_fn_getvalue("myurl.php3?var=value","var") results in value
 * "value"
 *
 * Return "" when the variable couldn't be found.
 */
static char *lfname_fn_getvalue(const char *urlstr, const char *var)
{
  char *p;
  const char *found;
  int parlen;

  /* &var= */
  p = tl_str_concat(DBGvars(NULL), "&", var, "=", NULL);
  parlen = strlen(p);

  found = strstr(urlstr, p);
  if (!found)
  {
    /* ?var= */
    *p = '?';
    found = strstr(urlstr, p);
    if (!found)
    {
      /* var= - only acceptable at the start of the urlstr string */
      parlen--;
      if ((parlen > 0) && !strncmp(urlstr, p + 1, parlen))
      {
        found = urlstr;
      }
    }
  }
  _free(p);

  if (!found)
    return tl_strdup("");

  return tl_strndup(found + parlen, strcspn(found + parlen, "&"));
}

/*
 * Stack-based LISP like interpreter.
 *
 * Interpretes the statement list 'interp' (as produced by the parser lfname_lsp_analyze())
 * and returns the result in 'var'.
 */
static int lfname_lsp_eval(struct lfname_lsp_interp *interp, struct lfname_lsp_var *var)
{
  if (var->param1)
    lfname_lsp_eval(interp, var->param1);
  if (var->param2)
    lfname_lsp_eval(interp, var->param2);
  if (var->param3)
    lfname_lsp_eval(interp, var->param3);

  var->ret_val.str = NULL;
  var->ret_val.num = 0;

  switch (var->type)
  {
  default:
    DEBUG_RULES("???: pop unidentified opcode %d (BUG in pavuk!) --> (null)\n", var->type);
    break;

  case LF_LSP_UNKNOWN:
    DEBUG_RULES("UNKNOWN: pop unknown opcode --> (null)\n");
    break;

  case LF_LSP_STR:
    var->ret_val.str = tl_strdup(var->val.str);
    DEBUG_RULES("STR: pop string --> '%s'\n", var->ret_val.str);
    break;

  case LF_LSP_NUM:
    var->ret_val.num = var->val.num;
    DEBUG_RULES("NUM: pop number --> %d\n", var->ret_val.num);
    break;

  case LF_LSP_MACRO:
    var->ret_val.str = tl_strdup(lfname_interp_get_macro(interp, var->val.macro));
    DEBUG_RULES("MACRO: %%%c --> '%s'\n", var->val.macro, var->ret_val.str);
    break;

  case LF_LSP_SUB:
#ifdef HAVE_REGEX
    var->ret_val.str = lfname_re_sub(interp->orig, interp->urlstr, var->param1->ret_val.num);
#endif
    DEBUG_RULES("SUB: regex sub %d --> '%s'\n",
                var->param1->ret_val.num, (var->ret_val.str ? var->ret_val.str : "(null)"));
    break;

  case LF_LSP_SC:
    var->ret_val.str = tl_str_concat(DBGvars(
                                       NULL), var->param1->ret_val.str, var->param2->ret_val.str,
                                     NULL);
    DEBUG_RULES("SC: concat('%s', '%s') --> '%s'\n",
                var->param1->ret_val.str, var->param2->ret_val.str, var->ret_val.str);
    break;

  case LF_LSP_SS:
    {
      char *p;
      p = var->param1->ret_val.str;
      if (var->param2->ret_val.num > 0)
        p += (strlen(p) >= var->param2->ret_val.num) ? var->param2->ret_val.num : strlen(p);
      if (var->param3->ret_val.num > 0)
        var->ret_val.str = tl_strndup(p, var->param3->ret_val.num);
      else
        var->ret_val.str = tl_strdup(p);
      DEBUG_RULES("SS: substr('%s', %d, %d) --> '%s'\n",
                  var->param1->ret_val.str,
                  var->param2->ret_val.num,
                  var->param3->ret_val.num,
                  var->ret_val.str);
    }
    break;

  case LF_LSP_HASH:
    var->ret_val.num = str_hash_func(var->param2->ret_val.num, (dllist_t)var->param1->ret_val.str);
    DEBUG_RULES("HASH: hash('%s', %d) --> %d\n",
                var->param1->ret_val.str, var->param2->ret_val.num, var->ret_val.num);
    break;

  case LF_LSP_MD5:
    var->ret_val.str = _md5(var->param1->ret_val.str);
    DEBUG_RULES("MD5: md5('%s') --> '%s'\n",
                var->param1->ret_val.str, var->ret_val.str);
    break;

  case LF_LSP_LOWER:
    var->ret_val.str = lowerstr(var->param1->ret_val.str);
    DEBUG_RULES("LOWER: lower('%s') --> '%s'\n",
                var->param1->ret_val.str, var->ret_val.str);
    break;

  case LF_LSP_UPPER:
    var->ret_val.str = upperstr(var->param1->ret_val.str);
    DEBUG_RULES("UPPER: upper('%s') --> '%s'\n",
                var->param1->ret_val.str, var->ret_val.str);
    break;

  case LF_LSP_UENC:
    var->ret_val.str = url_encode_str(var->param1->ret_val.str, var->param2->ret_val.str);
    DEBUG_RULES("UENC: URLencode('%s', '%s') --> '%s'\n",
                var->param1->ret_val.str, var->param2->ret_val.str, var->ret_val.str);
    break;

  case LF_LSP_UDEC:
    var->ret_val.str = url_decode_str(var->param1->ret_val.str, strlen(var->param1->ret_val.str));
    DEBUG_RULES("UDEC: URLdecode('%s') --> '%s'\n",
                var->param1->ret_val.str, var->ret_val.str);
    break;

  case LF_LSP_DELCHR:
    var->ret_val.str = tr_del_chr(var->param2->ret_val.str, var->param1->ret_val.str);
    DEBUG_RULES("DELCHR: delchr('%s', '%s') --> '%s'\n",
                var->param1->ret_val.str, var->param2->ret_val.str, var->ret_val.str);
    break;

  case LF_LSP_TRCHR:
    var->ret_val.str = tr_chr_chr(var->param2->ret_val.str,
                                  var->param3->ret_val.str,
                                  var->param1->ret_val.str);
    DEBUG_RULES("TRCHR: trchr('%s', '%s', '%s') --> '%s'\n",
                var->param1->ret_val.str,
                var->param2->ret_val.str,
                var->param3->ret_val.str,
                var->ret_val.str);
    break;

  case LF_LSP_TRSTR:
    var->ret_val.str = tr_str_str(var->param2->ret_val.str,
                                  var->param3->ret_val.str,
                                  var->param1->ret_val.str);
    DEBUG_RULES("TRSTR: trstr('%s', '%s', '%s') --> '%s'\n",
                var->param1->ret_val.str,
                var->param2->ret_val.str,
                var->param3->ret_val.str,
                var->ret_val.str);
    break;

  case LF_LSP_STRSPN:
    var->ret_val.num = strspn(var->param1->ret_val.str, var->param2->ret_val.str);
    DEBUG_RULES("STRSPN: strspn('%s', '%s') --> %d\n",
                var->param1->ret_val.str, var->param2->ret_val.str, var->ret_val.num);
    break;

  case LF_LSP_STRCSPN:
    var->ret_val.num = strcspn(var->param1->ret_val.str, var->param2->ret_val.str);
    DEBUG_RULES("STRCSPN: strcspn('%s', '%s') --> %d\n",
                var->param1->ret_val.str, var->param2->ret_val.str, var->ret_val.num);
    break;

  case LF_LSP_STRLEN:
    var->ret_val.num = strlen(var->param1->ret_val.str);
    DEBUG_RULES("STRLEN: strspn('%s') --> %d\n",
                var->param1->ret_val.str, var->ret_val.num);
    break;

  case LF_LSP_NRSTR:
    {
      char pom[1024];
      snprintf(pom, sizeof(pom), var->param1->ret_val.str, var->param2->ret_val.num);
      pom[sizeof(pom) - 1] = 0;
      var->ret_val.str = tl_strdup(pom);
      DEBUG_RULES("NR2STR: printf('%s', %d) --> '%s'\n",
                  var->param1->ret_val.str, var->param2->ret_val.num, var->ret_val.str);
    }
    break;

  case LF_LSP_STRNR:
    {
      char *endofnum;
      var->ret_val.num = strtol(var->param1->ret_val.str, &endofnum, var->param2->ret_val.num);
      DEBUG_RULES("STR2NR2STR: strtol('%s', %d) --> %d\n",
                  var->param1->ret_val.str, var->param2->ret_val.num, var->ret_val.num);
    }
    break;

  case LF_LSP_LCHR:
    {
      char *p;
      p = strrchr(var->param1->ret_val.str, *var->param2->ret_val.str);
      var->ret_val.num = (p ? p - var->param1->ret_val.str : 0);
      DEBUG_RULES("LCHR: strrchr('%s', '%c' [%d]) --> %d\n",
                  var->param1->ret_val.str,
                  *var->param2->ret_val.str,
                  *var->param2->ret_val.str,
                  var->ret_val.num);
    }
    break;

  case LF_LSP_PLS:
    var->ret_val.num = var->param1->ret_val.num + var->param2->ret_val.num;
    DEBUG_RULES("PLUS: %d + %d --> %d\n",
                var->param1->ret_val.num, var->param2->ret_val.num, var->ret_val.num);
    break;

  case LF_LSP_MNS:
    var->ret_val.num = var->param1->ret_val.num - var->param2->ret_val.num;
    DEBUG_RULES("MINUS: %d - %d --> %d\n",
                var->param1->ret_val.num, var->param2->ret_val.num, var->ret_val.num);
    break;

  case LF_LSP_MOD:
    if (var->param2->ret_val.num == 0)
      var->ret_val.num = 0;
    else
      var->ret_val.num = var->param1->ret_val.num % var->param2->ret_val.num;
    DEBUG_RULES("MOD: %d %% %d --> %d\n",
                var->param1->ret_val.num, var->param2->ret_val.num, var->ret_val.num);
    break;

  case LF_LSP_MUL:
    var->ret_val.num = var->param1->ret_val.num * var->param2->ret_val.num;
    DEBUG_RULES("MUL: %d * %d --> %d\n",
                var->param1->ret_val.num, var->param2->ret_val.num, var->ret_val.num);
    break;

  case LF_LSP_DIV:
    if (var->param2->ret_val.num == 0)
      var->ret_val.num = 0;
    else
      var->ret_val.num = var->param1->ret_val.num / var->param2->ret_val.num;
    DEBUG_RULES("DIV: %d / %d --> %d\n",
                var->param1->ret_val.num, var->param2->ret_val.num, var->ret_val.num);
    break;

  case LF_LSP_REMOVEPARAMETER:
    var->ret_val.str = lfname_fn_removeparameter(var->param1->ret_val.str,
                                                 var->param2->ret_val.str);
    DEBUG_RULES("REMOVEPARAMETER: rmpar('%s', '%s') --> '%s'\n",
                var->param1->ret_val.str, var->param2->ret_val.str, var->ret_val.str);
    break;

  case LF_LSP_GETVALUE:
    var->ret_val.str = lfname_fn_getvalue(var->param1->ret_val.str, var->param2->ret_val.str);
    DEBUG_RULES("GETVALUE: getval('%s', '%s') --> '%s'\n",
                var->param1->ret_val.str, var->param2->ret_val.str, var->ret_val.str);
    break;

  case LF_LSP_SIF:
    var->ret_val.str = var->param1->ret_val.num
                       ? tl_strdup(var->param2->ret_val.str)
                       : tl_strdup(var->param3->ret_val.str);
    DEBUG_RULES("SIF: if (%d) '%s' else '%s' --> '%s'\n",
                var->param1->ret_val.num,
                var->param2->ret_val.str,
                var->param3->ret_val.str,
                var->ret_val.str);
    break;

  case LF_LSP_NOT:
    var->ret_val.num = !var->param1->ret_val.num;
    DEBUG_RULES("NOT: ! %d --> %d\n",
                var->param1->ret_val.num, var->ret_val.num);
    break;

  case LF_LSP_AND:
    var->ret_val.num = var->param1->ret_val.num && var->param2->ret_val.num;
    DEBUG_RULES("AND: %d AND %d --> %d\n",
                var->param1->ret_val.num, var->param2->ret_val.num, var->ret_val.num);
    break;

  case LF_LSP_OR:
    var->ret_val.num = var->param1->ret_val.num || var->param2->ret_val.num;
    DEBUG_RULES("OR: %d OR %d --> %d\n",
                var->param1->ret_val.num, var->param2->ret_val.num, var->ret_val.num);
    break;

  case LF_LSP_GETEXT:
    var->ret_val.str = tl_strdup(tl_get_extension(var->param1->ret_val.str));
    DEBUG_RULES("GETEXT: getext('%s') --> '%s'\n", var->param1->ret_val.str, var->ret_val.str);
    break;

  case LF_LSP_SEQ:
    var->ret_val.num = !strcmp(var->param1->ret_val.str, var->param2->ret_val.str);
    DEBUG_RULES("SEQ: ! strcmp('%s', '%s') --> %d\n",
                var->param1->ret_val.str, var->param2->ret_val.str, var->ret_val.num);
    break;

  case LF_LSP_FNSEQ:
    var->ret_val.num = (FNM_NOMATCH != fnmatch(
                          var->param1->ret_val.str,
                          var->param2->ret_val.str,
                          0));
    DEBUG_RULES("FNSEQ: ! fnmatch('%s', '%s') --> %d\n",
                var->param1->ret_val.str, var->param2->ret_val.str, var->ret_val.num);
    break;

  case LF_LSP_JSF:
#ifdef HAVE_MOZJS
    var->ret_val.str = pjs_run_fnrules_func(var->param1->ret_val.str, interp);
#endif
    DEBUG_RULES("JSF: JavaScript: %s(...) --> '%s'\n",
                var->param1->ret_val.str, (var->ret_val.str ? var->ret_val.str : "(null)"));
    break;
  }
  if (var->rettype == LF_LSP_STR && !var->ret_val.str)
    var->ret_val.str = tl_strdup("");

  return 0;
}

/*
 * Parse and execute the LISP like statement referenced by 'interp'.
 *
 * Return the string result (allocated on the heap) or NULL in case of
 * failure to parse the statement.
 */
static char *lfname_lsp_get_by_url(struct lfname_lsp_interp *interp)
{
  char *retv = NULL;
  struct lfname_lsp_var *variant;
  const char *p;

  p = interp->orig->transstr;

  variant = lfname_lsp_analyze(&p);

  DEBUG_RULES("LISP parser: %p\n", variant);

  if (variant)
  {
    lfname_lsp_eval(interp, variant);
    if (variant->rettype == LF_LSP_NUM)
    {
      char nr[20];
      sprintf(nr, "%d", variant->ret_val.num);
      retv = tl_strdup(nr);

      DEBUG_RULES("LISP interpreter returned NUM result: %d --> %s\n", variant->ret_val.num, retv);
    }
    else
    {
      retv = tl_strdup(variant->ret_val.str);

      DEBUG_RULES("LISP interpreter returned STR result: %s\n", retv);
    }
    lfname_lsp_var_free(variant);
  }

  return retv;
}

