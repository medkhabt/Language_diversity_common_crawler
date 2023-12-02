/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "doc.h"
#include "mime.h"
#include "mimetype.h"
#include "tools.h"

/*
 * Return the n-th occurrence of the MIME header 'param_id' found in the
 * 'mimet' string.
 *
 * Return NULL if no entry can be found.
 *
 * NOTES: MIME headers contained in the 'mimet' string are assumed to conform to
 * RFC2045 formatting rules. (See also HTTP 1.1: RFC1626 section 3.)
 *
 * The 'mimet' string is assumed to contain MIME headers only, where each line
 * contains a single MIME header and has been properly terminated (using the
 * "\r\n" CR LF character sequence).
 */
char *get_mime_n_param_val_str(char *param_id, char *mimet, int n)
{
  char *p;
  int i = 0;
  int ilen;

  if (!mimet)
    return NULL;

  p = mimet;

  while (*p)
  {
    ilen = strcspn(p, "\r\n");

    if (!strncasecmp(p, param_id, strlen(param_id)))
    {
      if (i == n)
      {
        char *p1;
        p1 = p + strlen(param_id);
        while (tl_ascii_isspace(*p1))
          p1++;
        p += ilen;
        while (*p)
        {
          p += strspn(p, "\n\r");
          ilen = strcspn(p, "\r\n");
          if ((*p == ' ' || *p == '\t'))
          {
            p += ilen;
          }
          else
            break;
        }
        p1 = tl_strndup(p1, p - p1);
        omit_chars(p1, "\r\n");
        return p1;
      }
      i++;
    }
    p += ilen;
    p += strspn(p, "\n\r");
  }
  return NULL;
}


/*
 * Return the preferred filename extension for the given MIME type 'type'.
 *
 * Return NULL if no default/preferred extension is known.
 */
const char *mime_get_type_ext(const char *type)
{
  const struct mime_type_ext *p;

  if (!type)
    return NULL;

  ASSERT(get_mime_type_ext_collection() != NULL);
  for (p = get_mime_type_ext_collection(); p->mimet; ++p)
  {
    if (!fnmatch(p->mimet, type, 0))
    {
      return p->ext;
    }
    /*
     * As the collection may carry multiple entries for each MIME type
     * (different extensions!), we'll skip any subsequent entry with
     * an identical mimetype value. No need for useless extra
     * fnmatch() overhead: skip those identical MIME type entries.
     */
    while (p[1].mimet && strcmp(p->mimet, p[1].mimet) == 0)
      p++;
  }
  return NULL;
}


/*
 * Return TRUE when the extension_to_test is a valid extension for the given mime type.
 *
 * When the opt_mime_type_ext is given, also compare against this extension and return
 * TRUE when there's a match.
 *
 * Otherwise, return FALSE.
 *
 * NOTES:
 *
 * This code will always return FALSE for empty or NULLed extensions as these do not really
 * 'relate' to any MIME type.
 *
 * Purpose:
 *
 * This is used by the -fnrules %X macro and can be employed to, for instance, append the
 * proper '.zip' extension to application/x-zip files which have been produced by the server
 * through a server-side script page with, for example, a '.jsp' or '.asp' extension.
 * Those extensions might be assumed to produce .html (text!) output otherwise, but for ease of
 * local browsing and access to the grabbed files, a fitting extension like '.zip' or '.html'
 * helps the 'user experience' tremendously.
 */
int mime_ext_matches_type(const char *extension_to_test, const char *mime_type, const char *opt_mime_type_ext)
{
  const struct mime_type_ext *p;

  if (!extension_to_test || ! * extension_to_test)
    return FALSE;

  /* matches the optional 'default extension'? */
  if (opt_mime_type_ext && !strcasecmp(opt_mime_type_ext, extension_to_test))
    return TRUE;

  if (!mime_type)
    return FALSE;

  ASSERT(get_mime_type_ext_collection() != NULL);
  for (p = get_mime_type_ext_collection(); p->mimet; ++p)
  {
    if (!fnmatch(p->mimet, mime_type, 0))
    {
      /*
       * As the collection may carry multiple entries for each MIME type
       * (different extensions!), we must check each subsequent entry with
       * an identical mimetype value. No need for useless extra
       * fnmatch() overhead here.
       */
      do
      {
        if (p->ext && !strcasecmp(p->ext, extension_to_test))
          return TRUE;

        p++;
      } while (p->mimet && strcmp(p[-1].mimet, p->mimet) == 0);
      p--;

      /*
       * WARNING: the mimetypes in the collection MAY contain wildcards, so
       * our mime_type MAY match multiple, different, p->mimet mimetype
       * records.
       *
       * Hence we should NOT exit the for() loop, but continue till the end
       * of the collection, even though we've already found one mimetype
       * match already.
       */
    }
  }
  return FALSE;
}


