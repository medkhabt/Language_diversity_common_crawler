/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "errcode.h"


void report_error(doc * docp, char *str)
{
  char *ustr;
  char *mstr;

  if(cfg.rbreak)
  {
    xprintf(1, gettext("%s: user break\n"), str);
    return;
  }

  if(cfg.report_url_with_err && docp->doc_url)
  {
    ustr = url_to_urlstr(docp->doc_url, TRUE);
  }
  else
  {
    ustr = NULL;
  }
  if(!ustr)
  {
    str = tl_strdup(str ? str : "???");
  }
  else
  {
    str = tl_str_concat(DBGvars(NULL), "[", ustr, "] ", str, NULL);
  }

  mstr = doc_strerror(docp);
  str = tl_str_concat(DBGvars(str), ": ", mstr, NULL);

  switch (docp->errcode)
  {
  case ERR_NOERROR:
    if(cfg.verbose)
    {
      xprintf(1, "%s\n", str);
    }
    break;
  default:
    xprintf(1, "%s\n", str);
    break;
  }
  _free(str);
  _free(ustr);
  _free(mstr);
}


#undef nw_strerror
#undef doc_strerror

/*
   Return descriptive error message for the given document 'docp'.

   'docp' MUST NOT be NULL.
 */
char *doc_strerror(DBGdecl(doc * docp))
{
  if(cfg.rbreak)
  {
    char *ustr = gettext("user break");
    return tld_strdup(DBGpass(ustr));
  }

  ASSERT(docp != NULL);
  return nw_strerror(DBGpass(docp), docp->errcode);
}

/*
   Return descriptive message for the given error code 'errcode'.

   'docp' may be NULL.

   NOTE: the returned string is allocated on the heap; use _free() to release
         the memory when done processing.
 */
char *nw_strerror(DBGdecl(doc * docp), int errcode)
{
  char *ustr;
  char pom[1024];

  switch (errcode)
  {
  case ERR_NOERROR:
    ustr = gettext("OK");
    break;
  case ERR_STORE_DOC:
    ustr = gettext("ERROR: storing document");
    break;
  case ERR_FILE_OPEN:
    ustr = gettext("ERROR: opening file");
    break;
  case ERR_DIR_URL:
    ustr = gettext("ERROR: URL pointing to local directory is not supported");
    break;
  case ERR_UNKNOWN:
    ustr = gettext("ERROR: unknown");
    break;
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  case ERR_NOTHING_TO_SEND:
    ustr = gettext("ERROR: No replay data available to send to server");
    break;
  case ERR_UNEXPECTED_RESPONSE:
    ustr = gettext("ERROR: Web server returned an unexpected response code");
    break;
#endif
  case ERR_LOCKED:
    ustr = gettext("ERROR: document is locked");
    break;
  case ERR_READ:
    ustr = gettext("ERROR: reading socket");
    break;
  case ERR_BIGGER:
    ustr = gettext("MESSAGE: bigger than maximal allowed size");
    break;
  case ERR_SCRIPT_DISABLED:
    ustr = gettext("MESSAGE: disabled by user-exit script condition");
    break;
  case ERR_SMALLER:
    ustr = gettext("MESSAGE: smaller than minimal allowed size");
    break;
  case ERR_NOMIMET:
    snprintf(pom, sizeof(pom), gettext("MESSAGE: this mime type is not allowed (%s)"), (docp && docp->type_str ? docp->type_str : "unknown"));
    pom[sizeof(pom) - 1] = 0;
    ustr = pom;
    break;
  case ERR_PROXY_CONNECT:
    ustr = gettext("ERROR: error in proxy connect");
    break;
  case ERR_BREAK:
    ustr = gettext("ERROR: transfer broken by user");
    break;
  case ERR_OUTTIME:
    ustr = gettext("MESSAGE: file modification time doesn't fit to specified interval");
    break;
  case ERR_ZERO_SIZE:
    ustr = gettext("ERROR: file has zero size - possible error");
    break;
  case ERR_PROCESSED:
    ustr = gettext("MESSAGE: document was already processed");
    break;
  case ERR_UDISABLED:
    ustr = gettext("MESSAGE: document was disabled by user");
    break;
  case ERR_RDISABLED:
    ustr = gettext("MESSAGE: document was disabled by limiting rules");
    break;
  case ERR_LOW_TRANSFER_RATE:
    ustr = gettext("WARNING: transfer rate lower than minimal allowed");
    break;
  case ERR_QUOTA_FILE:
    ustr = gettext("WARNING: file size quota exceeded, rest will be truncated");
    break;
  case ERR_QUOTA_TRANS:
    ustr = gettext("WARNING: transfer quota exceeded, breaking download");
    break;
  case ERR_QUOTA_FS:
    ustr = gettext("ERROR: low free space on filesystem, breaking transfer");
    break;
  case ERR_QUOTA_TIME:
    ustr = gettext("WARNING: maximal allowed running time exceeded, downloading will break");
    break;
  case ERR_BAD_CONTENT:
    ustr = gettext("WARNING: 'bad content' match located, downloading will break");
    break;
  case ERR_FTP_UNKNOWN:
    ustr = gettext("ERROR: unnown FTP error");
    break;
  case ERR_FTP_NOREGET:
    ustr = gettext("ERROR: FTP server doesn't support REST command");
    break;
  case ERR_FTP_BDIR:
  case ERR_FTP_NODIR:
    ustr = gettext("ERROR: unable to list directory content");
    break;
  case ERR_FTP_CONNECT:
    ustr = gettext("ERROR: unable to connect to FTP server");
    break;
  case ERR_FTP_BUSER:
    ustr = gettext("ERROR: FTP authentification - bad username");
    break;
  case ERR_FTP_BPASS:
    ustr = gettext("ERROR: FTP authentification - bad password");
    break;
  case ERR_FTP_BPROXYUSER:
    ustr = gettext("ERROR: FTP proxy authentification - bad username");
    break;
  case ERR_FTP_BPROXYPASS:
    ustr = gettext("ERROR: FTP proxy authentification - bad password");
    break;
  case ERR_FTP_DATACON:
    ustr = gettext("ERROR: unable to open FTP data connection");
    break;
  case ERR_FTP_GET:
    ustr = gettext("ERROR: unable to get file from FTP server");
    break;
  case ERR_FTP_NOMDTM:
    ustr = gettext("ERROR: FTP server doesn't support MDTM command");
    break;
  case ERR_FTP_TRUNC:
    ustr = gettext("ERROR: file from FTP server is truncated");
    break;
  case ERR_FTP_ACTUAL:
    ustr = gettext("MESSAGE: reget unneeded - file is up to date");
    break;
  case ERR_FTP_NOTRANSFER:
    ustr = gettext("MESSAGE: FTP transfer not allowed because of rules");
    break;
  case ERR_FTP_DIRNO:
    ustr = gettext("WARNING: FTP directory URL, but FTP directory not allowed (-FTPdir)");
    break;
  case ERR_FTP_LOGIN_HANDSHAKE:
    ustr = gettext("WARNING: FTP login_handshake failed");
    break;
  case ERR_HTTP_UNKNOWN:
    ustr = gettext("ERROR: unknown HTTP error");
    break;
  case ERR_HTTP_CONNECT:
    ustr = gettext("ERROR: unable to connect to HTTP server");
    break;
  case ERR_HTTP_NOREGET:
    ustr = gettext("ERROR: HTTP server doesn't support partial content retrieving");
    break;
  case ERR_HTTP_SNDREQ:
    ustr = gettext("ERROR: broken HTTP request send");
    break;
  case ERR_HTTP_RCVRESP:
    ustr = gettext("ERROR: failed to read HTTP response");
    break;
  case ERR_HTTP_FAILREGET:
    ustr = gettext("ERROR: unexpected HTTP response code after trying to reget");
    break;
  case ERR_HTTP_SNDREQDATA:
    ustr = gettext("ERROR: unable to send HTTP request data");
    break;
  case ERR_HTTP_REDIR:
    ustr = gettext("MESSAGE: redirecting to another location");
    break;
  case ERR_HTTP_TRUNC:
    ustr = gettext("ERROR: HTTP document is truncated");
    break;
  case ERR_HTTP_CYCLIC:
    ustr = gettext("ERROR: cyclic redirection!");
    break;
  case ERR_HTTP_UNSUPREDIR:
    ustr = gettext("ERROR: redirecting to unsupported URL");
    break;
  case ERR_HTTP_PROXY_CONN:
    ustr = gettext("ERROR: unable to connect to proxy server");
    break;
  case ERR_HTTP_BADREDIRECT:
    ustr = gettext("ERROR: received bad redirect response from server");
    break;
  case ERR_HTTP_AUTH_NTLM:
    ustr = gettext("ERROR: unable to do NTLM authorization");
    break;
  case ERR_HTTP_AUTH_DIGEST:
    ustr = gettext("ERROR: unable to do HTTP Digest authorization");
    break;
  case ERR_HTTP_PROAUTH_NTLM:
    ustr = gettext("ERROR: unable to do NTLM proxy authorization");
    break;
  case ERR_HTTP_PROAUTH_DIGEST:
    ustr = gettext("ERROR: unable to do HTTP proxy Digest authorization");
    break;
  case ERR_GOPHER_CONNECT:
    ustr = gettext("ERROR: unable to connect to GOPHER server");
    break;
  case ERR_GOPHER_UNKNOWN:
    ustr = gettext("ERROR: unknown GOPHER error");
    break;
  case ERR_HTTPS_CONNECT:
    ustr = gettext("ERROR: unable to connect to HTTPS server");
    break;
  case ERR_FTPS_CONNECT:
    ustr = gettext("ERROR: unable to establish SSL control connection to FTPS server");
    break;
  case ERR_FTPS_UNSUPPORTED:
    ustr = gettext("ERROR: this FTP server doesn't support SSL connections");
    break;
  case ERR_FTPS_DATASSLCONNECT:
    ustr = gettext("ERROR: unable to establish SSL data connection to FTPS server");
    break;

  case ERR_HTTP_CONTINUE:
    ustr = gettext("ERROR: RFC2616: Section 10.1.1: Continue (HTTP response code 100)");
    break;
  case ERR_HTTP_SW_PCOLS:
    ustr = gettext("ERROR: RFC2616: Section 10.1.2: Switching Protocols (HTTP response code 101)");
    break;
  case ERR_HTTP_OK:
    ustr = gettext("ERROR: RFC2616: Section 10.2.1: OK (HTTP response code 200)");
    break;
  case ERR_HTTP_CREATED:
    ustr = gettext("ERROR: RFC2616: Section 10.2.2: Created (HTTP response code 201)");
    break;
  case ERR_HTTP_ACCEPTED:
    ustr = gettext("ERROR: RFC2616: Section 10.2.3: Accepted (HTTP response code 202)");
    break;
  case ERR_HTTP_NONAUTH_INFO:
    ustr = gettext("ERROR: RFC2616: Section 10.2.4: Non-Authoritative Information (HTTP response code 203)");
    break;
  case ERR_HTTP_NO_CONTENT:
    ustr = gettext("ERROR: RFC2616: Section 10.2.5: No Content (HTTP response code 204)");
    break;
  case ERR_HTTP_RST_CONTENT:
    ustr = gettext("ERROR: RFC2616: Section 10.2.6: Reset Content (HTTP response code 205)");
    break;
  case ERR_HTTP_PART_CONTENT:
    ustr = gettext("ERROR: RFC2616: Section 10.2.7: Partial Content (HTTP response code 206)");
    break;
  case ERR_HTTP_MULTICHOICE:
    ustr = gettext("ERROR: RFC2616: Section 10.3.1: Multiple Choices (HTTP response code 300)");
    break;
  case ERR_HTTP_MV_PERMANENT:
    ustr = gettext("ERROR: RFC2616: Section 10.3.2: Moved Permanently (HTTP response code 301)");
    break;
  case ERR_HTTP_FOUND:
    ustr = gettext("ERROR: RFC2616: Section 10.3.3: Found (HTTP response code 302)");
    break;
  case ERR_HTTP_SEE_OTHER:
    ustr = gettext("ERROR: RFC2616: Section 10.3.4: See Other (HTTP response code 303)");
    break;
  case ERR_HTTP_ACTUAL:
    ustr = gettext("MESSAGE: reget unneeded - file is up to date; RFC2616: Section 10.3.5: Not Modified (HTTP response code 304)");
    break;
  case ERR_HTTP_USE_PROXY:
    ustr = gettext("ERROR: you must use proxy to access this URL; RFC2616: Section 10.3.6: Use Proxy (HTTP response code 305)");
    break;
  case ERR_HTTP_306:
    ustr = gettext("ERROR: HTTP error 306 - unsupported by HTTP/1.1; obsolete (HTTP response code 306)");
    break;
  case ERR_HTTP_REDIR2:
    ustr = gettext("ERROR: RFC2616: Section 10.3.8: Temporary Redirect (HTTP response code 307)");
    break;
  case ERR_HTTP_BADRQ:
    ustr = gettext("ERROR: HTTP client sends bad request; RFC2616: Section 10.4.1: Bad Request (HTTP response code 400)");
    break;
  case ERR_HTTP_AUTH:
    ustr = gettext("ERROR: HTTP authentication is required; RFC2616: Section 10.4.2: Unauthorized (HTTP response code 401)");
    break;
  case ERR_HTTP_PAY:
    ustr = gettext("ERROR: HTTP payment required; RFC2616: Section 10.4.3: Payment Required (HTTP response code 402)");
    break;
  case ERR_HTTP_FORB:
    ustr = gettext("ERROR: forbidden HTTP request; RFC2616: Section 10.4.4: Forbidden (HTTP response code 403)");
    break;
  case ERR_HTTP_NFOUND:
    ustr = gettext("ERROR: HTTP document not found; RFC2616: Section 10.4.5: Not Found (HTTP response code 404)");
    break;
  case ERR_HTTP_NALLOW:
      ustr = gettext("ERROR: used HTTP method is not supported or not allowed for this URL; RFC2616: Section 10.4.6: Method Not Allowed (HTTP response code 405)");
	  break;
  case ERR_HTTP_NACCEPT:
      ustr = gettext("ERROR: client doesn't accept MIME type of requested URL; RFC2616: Section 10.4.7: Not Acceptable (HTTP response code 406)");
	  break;
  case ERR_HTTP_PROXY_AUTH:
      ustr = gettext("ERROR: HTTP proxy authentication is required; RFC2616: Section 10.4.8: Proxy Authentication Required (HTTP response code 407)");
	  break;
  case ERR_HTTP_TIMEOUT:
      ustr = gettext("ERROR: HTTP server replied with connection timeout response; RFC2616: Section 10.4.9: Request Time-out (HTTP response code 408)");
	  break;
  case ERR_HTTP_CONFLICT:
    ustr = gettext("ERROR: HTTP server replied with conflict response; RFC2616: Section 10.4.10: Conflict (HTTP response code 409)");
    break;
  case ERR_HTTP_GONE:
    ustr = gettext("ERROR: document was removed from HTTP server; RFC2616: Section 10.4.11: Gone (HTTP response code 410)");
    break;
  case ERR_HTTP_MISSLEN:
      ustr = gettext("ERROR: in request header is missing Content-Length: header; RFC2616: Section 10.4.12: Length Required (HTTP response code 411)");
	  break;
  case ERR_HTTP_PREC_FAIL:
      ustr = gettext("ERROR: preconditions in request failed for requested URL; RFC2616: Section 10.4.13: Precondition Failed (HTTP response code 412)");
	  break;
  case ERR_HTTP_TOO_LARGE:
      ustr = gettext("ERROR: request body too large; RFC2616: Section 10.4.14: Request Entity Too Large (HTTP response code 413)");
	  break;
  case ERR_HTTP_LONG_URL:
      ustr = gettext("ERROR: request URL too long; RFC2616: Section 10.4.15: Request-URI Too Large (HTTP response code 414)");
	  break;
  case ERR_HTTP_UNSUP_MEDIA:
      ustr = gettext("ERROR: resource in format unsupported for this request; RFC2616: Section 10.4.16: Unsupported Media Type (HTTP response code 415)");
	  break;
  case ERR_HTTP_BAD_RANGE:
      ustr = gettext("ERROR: requested bad range of document; RFC2616: Section 10.4.17: Requested range not satisfiable (HTTP response code 416)");
	  break;
  case ERR_HTTP_EXPECT_FAIL:
      ustr = gettext("ERROR: failed to fulfill expectation from request; RFC2616: Section 10.4.18: Expectation Failed (HTTP response code 417)");
	  break;
  case ERR_HTTP_SERV:
      ustr = gettext("ERROR: HTTP remote server error; RFC2616: Section 10.5.1: Internal Server Error (HTTP response code 500)");
	  break;
  case ERR_HTTP_NOT_IMPL:
      ustr = gettext("ERROR: requested HTTP method not implemented on server side; RFC2616: Section 10.5.2: Not Implemented (HTTP response code 501)");
	  break;
  case ERR_HTTP_BAD_GW:
      ustr = gettext("ERROR: gatewaying failed for this URL; RFC2616: Section 10.5.3: Bad Gateway (HTTP response code 502)");
	  break;
  case ERR_HTTP_SERV_UNAVAIL:
      ustr = gettext("ERROR: HTTP service currently not available, check later; RFC2616: Section 10.5.4: Service Unavailable (HTTP response code 503)");
	  break;
  case ERR_HTTP_GW_TIMEOUT:
      ustr = gettext("ERROR: timeout occured in communication between gateway and remote server; RFC2616: Section 10.5.5: Gateway Time-out (HTTP response code 504)");
	  break;
  case ERR_HTTP_VER_UNSUP:
      ustr = gettext("ERROR: HTTP version used in request is unsupported by remote server; RFC2616: Section 10.5.6: HTTP Version not supported (HTTP response code 505)");
	  break;

  default:
    snprintf(pom, sizeof(pom), gettext("ERROR: unknown errcode: %d"), errcode);
    pom[sizeof(pom) - 1] = 0;
    ustr = pom;
    break;
  }
  return tld_strdup(DBGpass(ustr));
}
