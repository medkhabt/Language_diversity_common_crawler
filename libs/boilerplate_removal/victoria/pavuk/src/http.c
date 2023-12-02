/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "url.h"
#include "form.h"
#include "http.h"
#include "net.h"
#include "base64.h"
#include "tools.h"
#include "cookie.h"
#include "mime.h"
#include "errcode.h"
#include "abstract.h"
#include "myssl.h"
#include "authinfo.h"
#include "gui_api.h"
#include "times.h"
#include "uexit.h"
#include "cookie.h"
#include "ntlm_auth.h"
#include "digest_auth.h"
#include "doc.h"
#include "hammer_mode.h"


static void http_process_response_11flags(doc *, http_response *);

const http_auth_type_info_t http_auths[] = 
{
  {"", HTTP_AUTH_NONE},
  {"user", HTTP_AUTH_USER},
  {"Basic", HTTP_AUTH_BASIC},
  {"Digest", HTTP_AUTH_DIGEST},
  {"NTLM", HTTP_AUTH_NTLM},
  {NULL, HTTP_AUTH_NONE},
};

/*
   HTTP response codes according to RFC2616
 */
static int valid_http_response_codes[] =
{
HTTP_RSP_CONTINUE,
HTTP_RSP_SW_PCOLS,
HTTP_RSP_OK,
HTTP_RSP_CREATED,
HTTP_RSP_ACCEPTED,
HTTP_RSP_NONAUTH_INFO,
HTTP_RSP_NO_CONTENT,
HTTP_RSP_RST_CONTENT,
HTTP_RSP_PART_CONTENT,
HTTP_RSP_MULTICHOICE,
HTTP_RSP_MV_PERMANENT,
HTTP_RSP_FOUND,
HTTP_RSP_SEE_OTHER,
HTTP_RSP_ACTUAL,
HTTP_RSP_USE_PROXY,
HTTP_RSP_306,
HTTP_RSP_REDIR2,
HTTP_RSP_BADRQ,
HTTP_RSP_AUTH,
HTTP_RSP_PAY,
HTTP_RSP_FORB,
HTTP_RSP_NFOUND,
HTTP_RSP_NALLOW,
HTTP_RSP_NACCEPT,
HTTP_RSP_PROXY_AUTH,
HTTP_RSP_TIMEOUT,
HTTP_RSP_CONFLICT,
HTTP_RSP_GONE,
HTTP_RSP_MISSLEN,
HTTP_RSP_PREC_FAIL,
HTTP_RSP_TOO_LARGE,
HTTP_RSP_LONG_URL,
HTTP_RSP_UNSUP_MEDIA,
HTTP_RSP_BAD_RANGE,
HTTP_RSP_EXPECT_FAIL,
HTTP_RSP_SERV,
HTTP_RSP_NOT_IMPL,
HTTP_RSP_BAD_GW,
HTTP_RSP_SERV_UNAVAIL,
HTTP_RSP_GW_TIMEOUT,
HTTP_RSP_VER_UNSUP,
};

static bool_t is_valid_http_response_code(int resp_code)
{
	int i;

	for(i = 0; i < NUM_ELEM(valid_http_response_codes); i++)
	{
		if (valid_http_response_codes[i] == resp_code)
			return TRUE;
	}
	return FALSE;
}

httphdr *httphdr_parse(char *str)
{
  char *p;
  httphdr *rv = _malloc(sizeof(*rv));

  rv->all = FALSE;
  if(*str == '+')
  {
    rv->all = TRUE;
    str++;
  }

  p = strchr(str, ':');

  if(!p)
  {
    _free(rv);
    return NULL;
  }

  rv->name = tl_strndup(str, p - str + 1);

  p++;
  rv->val = (*p == ' ') ? tl_strdup(p + 1) : tl_strdup(p);
  return rv;
}

void httphdr_free(httphdr * hdr)
{
  _free(hdr->val);
  _free(hdr->name);
  _free(hdr);
}

static char *http_get_additional_headers(url * urlp)
{
  char pom[2048];
  char *req = NULL;

  if(priv_cfg.http_headers)
  {
    dllist *ptr;
    ptr = priv_cfg.http_headers;

    while(ptr)
    {
      httphdr *hdr = (httphdr *) ptr->data;

      if(!urlp->parent_url || (urlp->parent_url && hdr->all))
      {
        snprintf(pom, sizeof(pom), "%s %s\r\n", hdr->name, hdr->val);
        pom[sizeof(pom) - 1] = 0;
        req = tl_str_append(req, pom);
      }
      ptr = ptr->next;
    }
  }

  return req;
}

static char *http_get_auth_str(doc * docp, char *method)
{
  char auth[2048];
  char pom[1024];
  char *user, *pass, *p;
  int auth_scheme;
  http_digest_info *auth_digest;

  auth[0] = '\0';

  if(docp->doc_url->type == URLT_FTP)
    return NULL;

  auth_digest = (http_digest_info *) docp->auth_digest;

  user = url_get_user(docp->doc_url, auth_digest ? auth_digest->realm : NULL);
  pass = url_get_pass(docp->doc_url, auth_digest ? auth_digest->realm : NULL);
  auth_scheme = url_get_auth_scheme(docp->doc_url, auth_digest ? auth_digest->realm : NULL);

  if(user)
  {
    if(pass)
    {
      if(auth_scheme == HTTP_AUTH_DIGEST && auth_digest)
      {
        http_get_digest_auth_str(auth_digest, method, user, pass, docp->doc_url, auth, sizeof(auth));
      }
      else if(auth_scheme == HTTP_AUTH_USER)
      {
        snprintf(auth, sizeof(auth), "user %s:%s", user, pass);
        auth[sizeof(auth) - 1] = 0;
      }
      else if(auth_scheme == HTTP_AUTH_BASIC)
      {
        snprintf(pom, sizeof(pom), "%s:%s", user, pass);
        pom[sizeof(pom) - 1] = 0;
        p = base64_encode(pom);
        snprintf(auth, sizeof(auth), "Basic %s", p);
        auth[sizeof(auth) - 1] = 0;
        _free(p);
      }
    }
    else
    {
      strncpy(auth, user, sizeof(auth));
      auth[sizeof(auth) - 1] = '\0';
    }
  }

  return auth[0] ? tl_strdup(auth) : NULL;
}

static char *http_get_proxy_auth_str(doc * docp, char *method)
{
  char auth[2048];
  char pom[1024];
  char *proxy_user, *proxy_pass, *p;
  int proxy_auth_scheme;
  http_digest_info *auth_proxy_digest;

  auth_proxy_digest = (http_digest_info *) docp->auth_proxy_digest;

  auth[0] = '\0';

  proxy_user = priv_cfg.http_proxy_user;
  proxy_pass = priv_cfg.http_proxy_pass;
  proxy_auth_scheme = cfg.proxy_auth_scheme;

  if(docp->http_proxy_port)
  {
    authinfo *ai;

    ai = authinfo_match_entry(docp->doc_url->type, docp->http_proxy, docp->http_proxy_port, NULL, NULL);
    if(ai)
    {
      proxy_user = ai->user;
      proxy_pass = ai->pass;
      proxy_auth_scheme = ai->type;
    }
  }

  if(proxy_user)
  {
    if(proxy_pass)
    {
      if(auth_proxy_digest && proxy_auth_scheme == HTTP_AUTH_DIGEST)
      {
        http_get_digest_auth_str(auth_proxy_digest, method, proxy_user, proxy_pass, docp->doc_url, auth, sizeof(auth));
      }
      else if(proxy_auth_scheme == HTTP_AUTH_USER)
      {
        snprintf(auth, sizeof(auth), "user %s:%s", proxy_user, proxy_pass);
        auth[sizeof(auth) - 1] = 0;
      }
      else if(proxy_auth_scheme == HTTP_AUTH_BASIC)
      {
        snprintf(pom, sizeof(pom), "%s:%s", proxy_user, proxy_pass);
        pom[sizeof(pom) - 1] = 0;
        p = base64_encode(pom);
        snprintf(auth, sizeof(auth), "Basic %s", p);
        auth[sizeof(auth) - 1] = 0;
        _free(p);
      }
    }
    else
    {
      snprintf(auth, sizeof(auth), "user %s", proxy_user);
      auth[sizeof(auth) - 1] = 0;
    }
  }

  return auth[0] ? tl_strdup(auth) : NULL;
}

http_auth_type_t http_get_authorization_type(char *auth_field)
{
  char *p = auth_field;
  int l, i;

  while(tl_ascii_isspace(*p))
    p++;

  l = strcspn(p, " \t");

  for(i = 0; http_auths[i].name; i++)
  {
    if(!strncasecmp(p, http_auths[i].name, l))
    {
      return http_auths[i].id;
    }
  }

  return HTTP_AUTH_NONE;
}

int http_handle_site_auth_info(doc * docu)
{
  int rv = -1;
  char *authtag = NULL;
  int is_digest = -1;
#ifdef ENABLE_NTLM
  int is_ntlm = -1;
#endif
  int i;

  for(i = 0; ; i++)
  {
    authtag = get_mime_n_param_val_str("WWW-Authenticate:", docu->mime, i);

    if(authtag)
    {
      switch (http_get_authorization_type(authtag))
      {
      case HTTP_AUTH_DIGEST:
        is_digest = i;
        break;
#ifdef ENABLE_NTLM
      case HTTP_AUTH_NTLM:
        is_ntlm = i;
        break;
#endif
      default:
        break;
      }
      _free(authtag);
    }
	else
	{
		break;
	}
  }

  if(is_digest >= 0)
  {
    authtag = get_mime_n_param_val_str("WWW-Authenticate:", docu->mime, is_digest);

    rv = http_digest_do_auth(docu, authtag);

    _free(authtag);
  }
#ifdef ENABLE_NTLM
  else if(is_ntlm >= 0)
  {
    authtag = get_mime_n_param_val_str("WWW-Authenticate:", docu->mime, is_ntlm);

    rv = ntlm_negotiate_connection(docu, authtag);

    _free(authtag);
  }
#endif

  return rv;
}

int http_handle_proxy_auth_info(doc * docu)
{
  int rv = -1;
  char *authtag = NULL;
  int is_digest = -1;
#ifdef ENABLE_NTLM
  int is_ntlm = -1;
#endif
  int i;

  for(i = 0; ; i++)
  {
    authtag = get_mime_n_param_val_str("Proxy-Authenticate:", docu->mime, i);

    if(authtag)
    {
      switch (http_get_authorization_type(authtag))
      {
      case HTTP_AUTH_DIGEST:
        is_digest = i;
        break;
#ifdef ENABLE_NTLM
      case HTTP_AUTH_NTLM:
        is_ntlm = i;
        break;
#endif
      default:
        break;
      }
      _free(authtag);
    }
	else
	{
		break;
	}
  }

  if(is_digest >= 0)
  {
    authtag = get_mime_n_param_val_str("Proxy-Authenticate:", docu->mime, is_digest);

    rv = http_digest_do_proxy_auth(docu, authtag);

    _free(authtag);
  }
#ifdef ENABLE_NTLM
  else if(is_ntlm >= 0)
  {
    authtag = get_mime_n_param_val_str("Proxy-Authenticate:", docu->mime, is_ntlm);

    rv = ntlm_negotiate_proxy_connection(docu, authtag);

    _free(authtag);
  }
#endif

  return rv;
}

static int http_dummy_proxy_send_connect(doc * docp, char *host, int port)
{
  char pom[1024];
  char *req, *p;

  if(cfg.use_http11)
  {
    snprintf(pom, sizeof(pom), "CONNECT %s:%d HTTP/1.1\r\nHost: %s:%d\r\n", host, port, host, port);
  }
  else
  {
    snprintf(pom, sizeof(pom), "CONNECT %s:%d HTTP/1.0\r\n", host, port);
  }
  pom[sizeof(pom) - 1] = 0;

  req = tl_strdup(pom);

  /**** information for HTTP proxy authorization ****/
  p = http_get_proxy_auth_str(docp, "CONNECT");
  if(p)
  {
    req = tl_str_concat(DBGvars(req), "Proxy-Authorization: ", p, "\r\n", NULL);
    _free(p);
  }

  if(docp->additional_headers)
    req = tl_str_concat(DBGvars(req), docp->additional_headers, NULL);

  /**** additional headers via -httpadd ****/
  if((p = http_get_additional_headers(docp->doc_url)))
  {
    req = tl_str_append(req, p);
    _free(p);
  }

  req = tl_str_concat(DBGvars(req), "\r\n", NULL);

  DEBUG_PROTOC("****************** Proxy connect request *****************\n");
  DEBUG_PROTOC("%s", req);
  DEBUG_PROTOC("**********************************************************\n");

  if(abs_write(docp->datasock, req, strlen(req)) != strlen(req))
  {
    xperror("Proxy connect request");
    _free(req);
    return -1;
  }

  _free(req);

  return 0;
}

int http_dumy_proxy_connect_real(doc * docp, char *host, int port, char *proxy_host, int proxy_port)
{
  char *p;
  int len;
  bool_t rem_proxy = FALSE;
  int rv = 0;

  docp->request_type = HTTP_REQ_CONNECT;
  docp->connect_host = host;
  docp->connect_port = port;

  if(!docp->http_proxy && proxy_host)
  {
    rem_proxy = TRUE;
    docp->http_proxy = tl_strdup(proxy_host);
    docp->http_proxy_port = proxy_port;
  }

  if(http_dummy_proxy_send_connect(docp, host, port))
    return -1;

  p = NULL;
  if((rv = http_read_mime_header(docp, &p, &len)) > 0)
  {
    http_response *resp;

    DEBUG_PROTOS("***************** Proxy connect response *****************\n");
    DEBUG_PROTOS("%s", p);
    DEBUG_PROTOS("**********************************************************\n");

    docp->mime = p;
    resp = http_get_response_info(p);

    http_process_response_11flags(docp, resp);

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
    if(docp->actrec_ref)
    {
      activity_record *rec = docp->actrec_ref;

      DEBUG_DEVEL("recording response code: %d [%d/%s]\n", resp->ret_code, __LINE__, __FILE__);
      rec->expected_response_code = resp->ret_code;
    }
#endif

    /*** proxy authorization required ***/
    if(resp->ret_code == HTTP_RSP_PROXY_AUTH)
    {
      int nauth = docp->num_proxy_auth;

      rv = http_handle_proxy_auth_info(docp);

      if(rv >= 0)
      {
        _free(docp->mime);
        http_response_free(resp);
        return rv ? -1 : 0;
      }
      else if(nauth)
      {
        rv = 0;
      }
    }
    else if(resp->ret_code >= 400)
    {
      rv = -1;
    }
    else
    {
      rv = 0;
    }

    http_response_free(resp);
  }
  else
  {
    if(!rv)
    {
      if(cfg.dump_resp && bufio_is_open(cfg.dump_fd) && !cfg.dump_after)
      {
        /* KNOWN BUG: this info will be lost when dump_after is enabled */
        ASSERT(cfg.dump_fd);
        ASSERT(bufio_is_open(cfg.dump_fd));
        bufio_write_m(cfg.dump_fd, DUMP_SECTION_SEPSTR, strlen(DUMP_SECTION_SEPSTR), p, len, NULL);
      }
    }
    rv = -1;
  }

  if(rem_proxy)
    docp->http_proxy = NULL;

  return rv;
}

int http_dummy_proxy_connect(doc * docp, char *host, int port, char *proxy_host, int proxy_port)
{
  int rv;
  doc docs;

  /* quick save of values */
  memcpy(&docs, docp, sizeof(doc));

  rv = http_dumy_proxy_connect_real(docp, host, port, proxy_host, proxy_port);

  /* restore some of affected values */
  docp->is_parsable = docs.is_parsable;
  docp->size = docs.size;
  docp->totsz = docs.totsz;
  docp->origsize = docs.origsize;
  docp->rest_pos = docs.rest_pos;
  docp->rest_end_pos = docs.rest_end_pos;
  docp->is_http11 = docs.is_http11;
  docp->chunk_size = docs.chunk_size;
  docp->is_chunked = docs.is_chunked;
  docp->read_chunksize = docs.read_chunksize;
  docp->read_trailer = docs.read_trailer;
  docp->is_persistent = docs.is_persistent;
  docp->current_size = docs.current_size;
  docp->adj_sz = docs.adj_sz;

  /* reset errcode */
  if(!rv)
    docp->errcode = ERR_NOERROR;

  return rv;
}

/************************************************/
/* create and send whole HTTP request with      */
/* respective document body (POST req)          */
/************************************************/
static int http_request(doc * docp, char *method, char *data, int datalen, char *conttype)
{
  char *req = NULL;
  char pom[BUFIO_ADVISED_READLN_BUFSIZE];
  char *p;
  const char **al;
  int len, wlen;
  url *urlp = docp->doc_url;

  gui_set_status(gettext("Sending request ..."));

  if((!cfg.http_proxy && urlp->type == URLT_HTTP) || urlp->type == URLT_HTTPS)
  {
    p = url_to_request_urlstr(urlp, FALSE);
  }
  else
  {
    /* authorization of nonanonymous FTP access */
    /* via HTTP gateways is done different way  */
    /* use ftp://user:password@server/... in    */
    /* request instead of Authorization: ...    */
    if(urlp->type == URLT_FTP)
    {
      char *user;
      char *pass;

      user = url_get_user(urlp, NULL);
      pass = url_get_pass(urlp, NULL);

      if(user && !urlp->p.ftp.user)
        urlp->p.ftp.user = user;
      else
        user = NULL;

      if(pass && !urlp->p.ftp.password)
        urlp->p.ftp.password = pass;
      else
        pass = NULL;

      p = url_to_request_urlstr(urlp, TRUE);
      if(user)
        urlp->p.ftp.user = NULL;
      if(pass)
        urlp->p.ftp.password = NULL;
    }
    else
    {
      p = url_to_request_urlstr(urlp, TRUE);
    }
  }

  if(cfg.use_http11)
    req = tl_str_concat(DBGvars(req), method, " ", p, " HTTP/1.1\r\n", NULL);
  else
    req = tl_str_concat(DBGvars(req), method, " ", p, " HTTP/1.0\r\n", NULL);

  _free(p);

  if(priv_cfg.identity)
  {
    snprintf(pom, sizeof(pom), "User-Agent: %s\r\n", priv_cfg.identity);
  }
  else
  {
    snprintf(pom, sizeof(pom), "User-Agent: %s/%s %s\r\n", PACKAGE, VERSION, HOSTTYPE);
  }
  pom[sizeof(pom) - 1] = 0;
  req = tl_str_append(req, pom);

  if(url_get_port(urlp) != prottable[urlp->type].default_port)
  {
    snprintf(pom, sizeof(pom), "Host: %s:%d\r\n", url_get_site(urlp), url_get_port(urlp));
  }
  else
  {
    snprintf(pom, sizeof(pom), "Host: %s\r\n", url_get_site(urlp));
  }
  pom[sizeof(pom) - 1] = 0;
  req = tl_str_append(req, pom);

  if(cfg.send_from && priv_cfg.from)
  {
    snprintf(pom, sizeof(pom), "From: %s\r\n", priv_cfg.from);
    pom[sizeof(pom) - 1] = 0;
    req = tl_str_append(req, pom);
  }

  if(cfg.send_cookies && (p = cookie_get_field(urlp)))
  {
    req = tl_str_append(req, p);
    _free(p);
  }

  /*** HTTP authorization field ****/
  p = http_get_auth_str(docp, method);
  if(p)
  {
    req = tl_str_concat(DBGvars(req), "Authorization: ", p, "\r\n", NULL);
    _free(p);
  }

  /**** information for HTTP proxy authorization ****/
  p = http_get_proxy_auth_str(docp, method);
  if(p)
  {
    req = tl_str_concat(DBGvars(req), "Proxy-Authorization: ", p, "\r\n", NULL);
    _free(p);
  }

  /**** prefered language ****/
  if(priv_cfg.accept_lang)
  {
    bool_t f = FALSE;
    al = priv_cfg.accept_lang;

    if(*al)
    {
      snprintf(pom, sizeof(pom), "Accept-Language: %s", *al);
      pom[sizeof(pom) - 1] = 0;
      al++;
      f = TRUE;
    }
    while(*al)
    {
      strcat(pom, ",");         /* FIXME: Security */
      strcat(pom, *al);
      ASSERT(strlen(pom) < sizeof(pom));
      al++;
    }
    if(f)
    {
      strcat(pom, "\r\n");
      req = tl_str_append(req, pom);
    }
  }

  /*** preffered character sets ***/
  if(priv_cfg.accept_chars)
  {
    bool_t f = FALSE;
    al = priv_cfg.accept_chars;

    if(*al)
    {
      snprintf(pom, sizeof(pom), "Accept-Charset: %s", *al);
      pom[sizeof(pom) - 1] = 0;
      al++;
      f = TRUE;
    }
    while(*al)
    {
      strcat(pom, ",");
      strcat(pom, *al);
      al++;
    }
    if(f)
    {
      strcat(pom, "\r\n");
      req = tl_str_append(req, pom);
    }
  }

  /**** referer URL ****/
  if(cfg.referer)
  {
    if(urlp->parent_url)
    {
      url *par_url;

      LOCK_URL(urlp);
      par_url = (url *) urlp->parent_url->data;
      UNLOCK_URL(urlp);
      if(par_url->type != URLT_FILE)
      {
        p = url_to_urlstr(par_url, FALSE);
        req = tl_str_concat(DBGvars(req), "Referer: ", p, "\r\n", NULL);
        _free(p);
      }
    }
    else if(cfg.auto_referer)
    {
      p = url_to_urlstr(urlp, FALSE);
      req = tl_str_concat(DBGvars(req), "Referer: ", p, "\r\n", NULL);
      _free(p);
    }
  }

  /**** allow transfer encoding with gzip, compress ****/
  if(cfg.use_enc)
  {
#ifdef HAVE_ZLIB
#if 01                           /*MJF: Horrible workaround hack until deflate code works */    /* [i_a] deflate and compress have been tested OK */
    req = tl_str_append(req,
      "Accept-Encoding: x-gzip;q=1.0, gzip;q=1.0, x-compress;q=0.5, compress;q=0.5, deflate;q=0.5, identity;q=0.3\r\n");
#else
    req = tl_str_append(req, "Accept-Encoding: deflate;q=1.0, identity;q=0.5, *;q=0\r\n");
#endif
#else
    req = tl_str_append(req, "Accept-Encoding: identity;q=1.0, *;q=0\r\n");
#endif
  }
  else
  {
    /* [i_a] do NOT accept compressed data streams! The only way to tell a server that
       is by passing a very specific Accept-Encoding: header as no header will have it
       assume you _can_ handle compressed I/O! */
    req = tl_str_append(req, "Accept-Encoding: identity;q=1.0, *;q=0\r\n");
  }

  /**** reget (not supported by all servers) ****/
  if(docp->rest_pos || docp->rest_end_pos > 0)
  {
    if(docp->rest_end_pos > 0)
    {
      snprintf(pom, sizeof(pom), "Range: bytes=%ld-%ld\r\n", (long) docp->rest_pos, (long) docp->rest_end_pos);
    }
    else
    {
      snprintf(pom, sizeof(pom), "Range: bytes=%ld-\r\n", (long) docp->rest_pos);
    }
    pom[sizeof(pom) - 1] = 0;
    req = tl_str_append(req, pom);
    if(cfg.send_if_range && docp->etag)
    {
      snprintf(pom, sizeof(pom), "If-Range: %s\r\n", docp->etag);
      pom[sizeof(pom) - 1] = 0;
      req = tl_str_append(req, pom);
    }
  }

  /*** conditional GET for sync mode ***/
  if(docp->origtime && !docp->rest_pos)
  {
    LOCK_TIME;
    strftime(pom, sizeof(pom), "If-Modified-Since: %a, %d %b %Y %H:%M:%S GMT\r\n", gmtime(&docp->origtime));
    pom[sizeof(pom) - 1] = 0;
    UNLOCK_TIME;
    req = tl_str_append(req, pom);
  }

  /**** no caching ****/
  if(!cfg.cache)
  {
    req = tl_str_append(req, "Pragma: no-cache\r\nCache-Control: no-cache\r\n");
  }

  if(!should_keep_persistent(docp))
  {
    req = tl_str_append(req, "Connection: close\r\n");
  }

  /**** additional headers via -httpadd ****/
  if((p = http_get_additional_headers(urlp)))
  {
    req = tl_str_append(req, p);
    _free(p);
  }

  if(conttype)
  {
    snprintf(pom, sizeof(pom), "Content-type: %s\r\n", conttype);
    pom[sizeof(pom) - 1] = 0;
    req = tl_str_append(req, pom);
  }

  if(datalen)
  {
    snprintf(pom, sizeof(pom), "Content-length: %d\r\n", datalen);
    pom[sizeof(pom) - 1] = 0;
    req = tl_str_append(req, pom);
  }

  if(docp->additional_headers)
  {
    req = tl_str_append(req, docp->additional_headers);
  }

  req = tl_str_append(req, "\r\n");

  /**** send request ****/
  len = strlen(req);
  _free(docp->request_header);
  docp->request_header = req;
  ASSERT(docp->request_data == NULL);
  ASSERT(docp->request_datalen == 0);

  DEBUG_PROTOC("************ Client HTTP MIME header: %d ***************\n", len);
  DEBUG_PROTOC("%s", req);
  DEBUG_PROTOC("****************************************************\n");

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  if(docp->actrec_ref)
  {
    activity_record *rec = docp->actrec_ref;

    DEBUG_DEVEL("set mime header: %d [%d/%s]\n", len, __LINE__, __FILE__);
    rec->req_mime_header_len = len;
    rec->req_mime_header = _malloc(rec->req_mime_header_len + 1);
    strcpy(rec->req_mime_header, req);
  }
#endif

  if(cfg.dump_req && bufio_is_open(cfg.dump_fd) && !cfg.dump_after)
  {
    ASSERT(cfg.dump_fd != NULL);
    ASSERT(bufio_is_open(cfg.dump_fd));
    bufio_write_m(cfg.dump_fd, DUMP_SECTION_SEPSTR, strlen(DUMP_SECTION_SEPSTR), req, len, NULL);
  }
/* [i_a] FIXED: here a lot of write timeouts took place during load testing */
  if((wlen = abs_write(docp->datasock, req, len)) != len)
  {
    DEBUG_DEVEL("abs_write %d -> %d\n", len, wlen);
    xperror("abs_write");
    /* _free(req); [i_a] */
    docp->errcode = ERR_HTTP_SNDREQ;
    return -1;
  }
  /* _free(req); [i_a] */

  if(data)
  {
    gui_set_status(gettext("Sending data ..."));
    DEBUG_PROTOD("************ HTTP request data: %d ***************\n", datalen);
    DEBUG_PROTOD("%s\n", data);
    DEBUG_PROTOD("*************************************************\n");

    ASSERT(datalen > 0);
    docp->request_data = _malloc(datalen);
    memcpy(docp->request_data, data, datalen);
    docp->request_datalen = datalen;

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
    if(docp->actrec_ref)
    {
      activity_record *rec = docp->actrec_ref;

      DEBUG_DEVEL("set client data: %d [%d/%s]\n", datalen, __LINE__, __FILE__);
      rec->req_data_len = datalen;
      rec->req_data = _malloc(rec->req_data_len);
      memcpy(rec->req_data, data, datalen);
    }
#endif

    if(cfg.dump_req && bufio_is_open(cfg.dump_fd) && !cfg.dump_after)
    {
      ASSERT(cfg.dump_fd);
      ASSERT(bufio_is_open(cfg.dump_fd));
      bufio_asciidump(cfg.dump_fd, data, datalen, NULL);
    }

    if(abs_write(docp->datasock, data, datalen) != datalen)
    {
      docp->errcode = ERR_HTTP_SNDREQDATA;
      return -1;
    }
  }

  gui_set_status(gettext("Waiting for response ..."));
  /*** handling of 1xx response codes ***/
  while((wlen = abs_readln(docp->datasock, pom, sizeof(pom) - 1)) > 0)
  {
    http_response *resp;

    DEBUG_PROTOD("client received %d bytes:\n%s\n", wlen, pom);
    if(!timerisset(&docp->first_byte_time))
    {
      gettimeofday(&docp->first_byte_time, NULL);
    }

    resp = http_get_response_info(pom);
#if 0
    if(resp)
    {
      _free(resp->text);
    }
#endif

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
    if(docp->actrec_ref && resp)
    {
      activity_record *rec = docp->actrec_ref;

      DEBUG_DEVEL("recording response code: %d [%d/%s]\n", resp->ret_code, __LINE__, __FILE__);
      rec->expected_response_code = resp->ret_code;
    }
#endif

    bufio_unread(docp->datasock, pom, wlen);
    if(resp && (resp->ret_code < HTTP_RSP_OK))
    {
      int rv1;
#if 0
      _free(resp);
#else
      http_response_free(resp);
      resp = NULL;
#endif
      if((rv1 = http_read_mime_header(docp, &p, &len)) <= 0)
      {
        if(!rv1)
        {
          if(cfg.dump_resp && bufio_is_open(cfg.dump_fd) && !cfg.dump_after)
          {
            /* KNOWN BUG: this info will be lost when dump_after is enabled */
            ASSERT(cfg.dump_fd);
            ASSERT(bufio_is_open(cfg.dump_fd));
            bufio_write_m(cfg.dump_fd, DUMP_SECTION_SEPSTR, strlen(DUMP_SECTION_SEPSTR), p, len, NULL);
          }
        }
        xprintf(1, gettext("Error reading HTTP 1xx class response\n"));
        break;
      }
      else
      {
        DEBUG_PROTOS("***************** class 1xx HTTP response ****************\n");
        DEBUG_PROTOS("%s", p);
        DEBUG_PROTOS("***********************************************************\n");
        /* KNOWN BUG for dump_after: does not dump this - intermediate - response */
        if(cfg.dump_resp && bufio_is_open(cfg.dump_fd) && !cfg.dump_after)
        {
          bufio *fd;

          ASSERT(cfg.dump_fd);
          ASSERT(bufio_is_open(cfg.dump_fd));
          VERIFY(!bufio_lock(cfg.dump_fd, FALSE, &fd));
          bufio_puts(fd, DUMP_SECTION_SEPSTR);
          bufio_write(fd, p, len);
          VERIFY(!bufio_release(cfg.dump_fd, &fd));
        }
      }
    }
    else
    {
#if 0
      _free(resp);
#else
      http_response_free(resp);
      resp = NULL;
#endif
      break;
    }
  }

  if(docp->is_http11 && docp->is_persistent && (!wlen || wlen < 0))
  {
    docp->is_persistent = FALSE;
    docp->errcode = ERR_HTTP_CLOSURE;
    DEBUG_DEVEL("abs_close_socket call @ [%d/%s]\n", __LINE__, __FILE__);
    abs_close_socket(docp, FALSE);
    return -1;
  }

  if(wlen < 0)
  {
    DEBUG_DEVEL("ERR_HTTP_RCVRESP [%d/%s]\n", __LINE__, __FILE__);
    docp->errcode = ERR_HTTP_RCVRESP;
    return -1;
  }

  return 0;
}

int http_get_request(doc * docp)
{
  docp->request_type = HTTP_REQ_GET;
  return http_request(docp, "GET", 0, 0, 0);
}

int http_head_request(doc * docp)
{
  docp->request_type = HTTP_REQ_HEAD;

  return http_request(docp, "HEAD", 0, 0, 0);
}

int http_post_request(doc * docp)
{
  form_info *fi = (form_info *) docp->doc_url->extension;
  char *data = NULL;
  int datalen = 0;
  char *type = NULL;
  int rv;

  docp->request_type = HTTP_REQ_POST;

  /* [i_a] fix UNKNOWN! */
  if(fi->encoding == FORM_E_UNKNOWN)
  {
    /* assume MULTIPART by default for POST? */
    DEBUG_HTMLFORM("unknown encoding detected; assuming urlencoded\n");
    fi->encoding = FORM_E_URLENCODED;
  }
  if(fi->encoding == FORM_E_MULTIPART)
  {
    fi->text = form_encode_multipart_boundary();
    type = tl_str_concat(DBGvars(type), "multipart/form-data; boundary=", fi->text, NULL);
  }
  else                          /* if (fi->encoding == FORM_E_URLENCODED) */
  {
    type = tl_strdup("application/x-www-form-urlencoded");
  }

  data = form_encode_query(docp->doc_url, fi, &datalen);

  if(!data && fi->infos)
  {
    _free(data);
    _free(type);
    _free(fi->text);

    docp->errcode = ERR_HTTP_BADRQ;

    return -1;
  }

  rv = http_request(docp, "POST", data, datalen, type);

  _free(data);
  _free(type);
  _free(fi->text);

  return rv;
}

static bufio *http_open_socket(doc * docp)
{
  char *host = NULL;
  int port = 0;


#define PENAULT_VAL     10

  docp->errcode = ERR_NOERROR;

  if(docp->http_proxy)
  {
    host = docp->http_proxy;
    port = docp->http_proxy_port;
  }
  else
  {
    host = url_get_site(docp->doc_url);
    port = url_get_port(docp->doc_url);
  }

  if(!docp->datasock)
  {
    gui_set_status(gettext("Connecting ..."));
#if defined(HAVE_DEBUG_FEATURES)
    if(cfg.verbose)
    {
      DEBUG_USER("Connecting to host:port: %s:%d\n", host, port);
    }
#endif

    docp->datasock = bufio_sock_fdopen(net_connect(host, port, docp));
#ifdef USE_SSL
    if(docp->datasock && docp->doc_url->type == URLT_HTTPS)
    {
      bufio *ssl_sock;

      ssl_sock = my_ssl_do_connect(docp, docp->datasock, NULL);
      if(!ssl_sock)
      {
        if(!docp->errcode)
          docp->errcode = ERR_HTTPS_CONNECT;
        bufio_close(docp->datasock);
        docp->datasock = NULL;
        return NULL;
      }
      else
      {
        docp->datasock = ssl_sock;
      }
    }
#endif
  }

  if(!docp->datasock)
  {
    if(_h_errno_ != 0)
      xherror(host);
    else
      xperror("net_connect");

    if(docp->http_proxy)
    {
      docp->errcode = ERR_HTTP_PROXY_CONN;

      /*** for penaulting failed HTTP proxy servers ***/
      if(docp->doc_url->type == URLT_HTTP)
      {
        http_proxy *pr;

        LOCK_PROXY;
        pr = http_proxy_find(docp->http_proxy, docp->http_proxy_port);
        if(pr)
        {
          pr->penault = PENAULT_VAL + pr->fails;
          pr->fails++;
        }
        UNLOCK_PROXY;
      }
      _free(docp->http_proxy);
    }
    else
    {
      docp->errcode = ERR_HTTP_CONNECT;
    }
    return NULL;
  }

  return docp->datasock;
}

void http_handle_redirect(doc * docu, int redir_code)
{
  url *pomurl;
  char *pomcr = NULL;

  pomcr = get_mime_param_val_str("Location:", docu->mime);

  if(!pomcr)
  {
    pomcr = get_mime_param_val_str("URI:", docu->mime);

    if(pomcr)
    {
      char *p;

      p = strchr(pomcr, ';');
      if(p)
        *p = '\0';
      p = strchr(pomcr, '>');
      if(p)
        *p = '\0';
      if(pomcr[0] == '<')
      {
        p = pomcr;
        pomcr = tl_strdup(pomcr + 1);
        _free(p);
      }
    }
    else
    {
      docu->errcode = ERR_HTTP_BADREDIRECT;
    }
  }

  if(pomcr)
  {
    pomurl = url_parse(pomcr);
    ASSERT(pomurl->type != URLT_FROMPARENT);

    if(pomurl->type == URLT_FILE)
    {
      char *base;
      char *baset;
      char *xp;

      baset = url_to_urlstr(docu->doc_url, FALSE);
      if((xp = strrchr(baset, '#')))
        *xp = '\0';
      if((xp = strrchr(baset, '?')))
        *xp = '\0';

      base = url_to_urlstr(docu->doc_url, FALSE);
      if(!tl_is_dirname(base))
      {
        xp = strrchr(base, '/');
        if(xp)
          *(xp + 1) = '\0';
      }

      xp = url_to_absolute_url(base, baset, docu->doc_url, pomcr);
      free_deep_url(pomurl);
      _free(pomurl);
      pomurl = url_parse(xp);
      _free(xp);
      _free(base);
      _free(baset);
    }
    _free(pomcr);

    if(pomurl && prottable[pomurl->type].supported)
    {
      if(docu->is_robot)
      {
        docu->doc_url->moved_to = pomurl;
        docu->errcode = ERR_HTTP_REDIR;
      }
      else
      {
        if(url_redirect_to(docu->doc_url, pomurl, (redir_code == 303)))
          docu->errcode = ERR_HTTP_CYCLIC;
        else
          docu->errcode = ERR_HTTP_REDIR;
      }
    }
    else
    {
      if(pomurl)
      {
        free_deep_url(pomurl);
        _free(pomurl);
        docu->errcode = ERR_HTTP_UNSUPREDIR;
      }
      else
      {
        docu->errcode = ERR_HTTP_BADREDIRECT;
      }
    }
  }
  else
  {
    docu->errcode = ERR_HTTP_BADREDIRECT;
  }
}

static void http_process_response_11flags(doc * docu, http_response * resp)
{
  char *p;

  if(cfg.use_http11 /* && !docu->http_proxy_10 */ 
	  && resp->ver_maj == 1 && resp->ver_min == 1)
  {
    docu->is_http11 = TRUE;
    docu->is_persistent = TRUE;
    docu->is_chunked = FALSE;
    docu->read_trailer = FALSE;
  }
  else
  {
    docu->is_http11 = FALSE;
    docu->is_persistent = FALSE;
    docu->is_chunked = FALSE;
    docu->read_trailer = FALSE;
  }

  p = get_mime_param_val_str("Content-Length:", docu->mime);
  if(p)
  {
    docu->totsz = _atoi(p);
    if(errno == ERANGE)
      docu->totsz = -1;
    _free(p);
  }

  p = get_mime_param_val_str("Transfer-Encoding:", docu->mime);
  if(p)
  {
    if(!strcasecmp(p, "chunked") || !strncasecmp(p, "chunked;", 8))
    {
      docu->is_chunked = TRUE;
      docu->read_chunksize = TRUE;
      docu->read_trailer = FALSE;
    }
    _free(p);
  }

  p = get_mime_param_val_str("Connection:", docu->mime);
  if(p)
  {
    if(!strcasecmp(p, "close"))
      docu->is_persistent = FALSE;
    else if(!strcasecmp(p, "keep-alive"))
      docu->is_persistent = TRUE;
    _free(p);
  }

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  if(docu->actrec_ref)
  {
    activity_record *rec = docu->actrec_ref;

    DEBUG_DEVEL("set persistent flag: %d [%d/%s]\n", docu->is_persistent, __LINE__, __FILE__);
    rec->expect_persistent = docu->is_persistent;
  }
#endif

  if(docu->http_proxy)
  {
    p = get_mime_param_val_str("Proxy-Connection:", docu->mime);
    if(p)
    {
      if(!strcasecmp(p, "close"))
        docu->is_persistent = FALSE;
      else if(!strcasecmp(p, "keep-alive"))
        docu->is_persistent = TRUE;
      _free(p);
    }
  }
}

/*
 * -1 - failure before sending auth data - http_process_response() can
 *      continue safely
 *  0 - OK - http_process_response() must return immediately
 *  1 - failure after sending auth data - http_process_response() must
 *      return immediately
 */
static int http_do_proxy_redirect(doc * docp)
{
  char *loc;
  int port = 80;                /* assume default proxy port: 80 */
  char proxy[256];
  int rv = 0;

  loc = get_mime_param_val_str("Location:", docp->mime);

  if(!loc)
    return -1;

  if(sscanf(loc, "http://%255[.0-9A-Za-z_-]:%d", proxy, &port) < 1)
    if(sscanf(loc, "%255[.0-9A-Za-z_-]:%d", proxy, &port) != 2)
      return -1;

  _free(docp->http_proxy);
  docp->http_proxy = tl_strdup(proxy);
  docp->http_proxy_port = port;

  /*** read body of the 305 response ***/
  rv = http_throw_message_body(docp);

  /*** we need to establish total new connection ***/
  docp->is_persistent = FALSE;
  DEBUG_DEVEL("abs_close_socket call @ [%d/%s]\n", __LINE__, __FILE__);
  abs_close_socket(docp, FALSE);
  _free(docp->mime);
  _free(docp->type_str);

  if(rv)
    return 1;

  /*** send new request through new proxy ***/
  if(http_repeat_request(docp))
    rv = 1;

  return rv;
}

static void http_process_response(doc * docu)
{
  int len;
  char *p;
  int mres;

  if((mres = http_read_mime_header(docu, &p, &len)) > 0)
  {
    http_response *resp;

    docu->mime = p;

    docu->adj_sz = len;

    if(cfg.htdig)
    {
      xprintf(0, "%s", docu->mime);
    }

    DEBUG_PROTOS("*********** HTTP Server response MIME header **********\n");
    DEBUG_PROTOS("%s", docu->mime);
    DEBUG_PROTOS("*******************************************************\n");
    if(cfg.dump_resp && bufio_is_open(cfg.dump_fd) && !cfg.dump_after)
    {
      ASSERT(cfg.dump_fd);
      ASSERT(bufio_is_open(cfg.dump_fd));
      bufio_puts_m(cfg.dump_fd, DUMP_SECTION_SEPSTR, docu->mime, NULL);
    }

    resp = http_get_response_info(docu->mime);

    http_process_response_11flags(docu, resp);

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
    if(docu->actrec_ref)
    {
      activity_record *rec = docu->actrec_ref;

      DEBUG_DEVEL("recording response code: %d [%d/%s]\n", resp->ret_code, __LINE__, __FILE__);
      rec->expected_response_code = resp->ret_code;
    }
#endif

    docu->type_str = get_mime_param_val_str("Content-type:", docu->mime);

    /*
       [i_a] It is RIGHT HERE where we are first (and ever) aware of the MIME type
       of the data/file we are going to fetch from local/remote storage.

       Local?! Yes, because shortly after this we may detect the server reporting
       a '304' No Change HTTP response code, which will drive pavuk to fetch the
       data from local (file system) storage instead.

       And because we'd like to have the _proper_ local filename ready to use
       with the least amount of 'hack' fuzz (see old code in doc_open_new_in_file() -
       that was rather too late) we should 'recalculate' the local filename here.
       Once that has been done, we should check if it actually _changed_ from the 
       previously calculated file path, because only then do we need to worry about
       possibly 'unexpected behaviour' as described in the man page for 
       -fnrules %E/%M/%X:

       --- pavuk.man ---
       %M - is replaced with MIME type of document. When you are using this macro, 
       you *must* use also -post_update option else it won't work.

       %E - is replaced with default extension assigned to MIME type of document. 
       When you are using this macro, you *must* use also -post_update option 
       else it won't work.

       %X - is replaced with file extension if one is available. Otherwise, the default
       extension assigned to the MIME type of the document is used instead. When 
       you are using this macro, you *must* also use the -post_update option for 
       this to work as expected.
       --- pavuk.man ---

       With this new code here, you do not need the -post_update option for those
       -fnrules macros to work.
     */

    /* First of all, this is only needed when we have -fnrules active and recognized a
       MIME type: */
    if(cfg.lfnames && docu->type_str)
    {
      char *old_lfn = url_to_filename(docu->doc_url, FALSE);
      char *new_lfn = NULL;

      ASSERT(old_lfn == docu->doc_url->local_name);
      old_lfn = tl_strdup(old_lfn);

      /* calculate filepath using its MIME type */
      url_forget_filename(docu->doc_url);
      new_lfn = url_to_filename_with_type(docu->doc_url, docu->type_str, FALSE);

      if(old_lfn && new_lfn && strcmp(old_lfn, new_lfn))
      {
        /* local filename changed, compared to what we thought it to be before 
           this: make sure we don't leave the old file floating around:

           CLEANUP
         */
        if(!tl_access(old_lfn, F_OK))
        {
          if(tl_access(new_lfn, F_OK))
          {
            if(tl_rename(old_lfn, new_lfn))
            {
				xperror2("Cannot move old temp local file to final destination: rename failed", 
					mk_native(old_lfn), mk_native(new_lfn));
            }
          }
		  else
		  {
			  xperror2("Cannot move old temp local file to final destination: cannot rename as the new file already exists!",
				  mk_native(old_lfn), mk_native(new_lfn));
		  }
        }
      }
      _free(old_lfn);
    }
    /* local file [re]naming done! */

    if(cfg.recv_cookies)
    {
      int i;
      char *cookie_field;

      for(i = 0; (cookie_field = get_mime_n_param_val_str("Set-Cookie:", docu->mime, i)); i++)
      {
#if 0                           /* [i_a] */
        cookie_insert_field(cookie_field, docu->doc_url);
#else
        cookie_entry *cookie;
        DEBUG_COOKIE("Insert cookie #%d: '%s'\n", i, cookie_field);
        cookie = cookie_parse(cookie_field, docu->doc_url);
        if(cookie)
        {
          store_cookie_in_global_store(cookie);
          cookie_deep_free(&cookie);
        }
#endif
        _free(cookie_field);
      }

      if(cfg.update_cookies)
      {
        cookie_update_file(FALSE);
      }
    }

    if(docu->type_str && (!strncasecmp("text/html", docu->type_str, 9) || !strncasecmp("text/css", docu->type_str, 8)))
    {
      if(!strncasecmp("text/css", docu->type_str, 8))
        docu->doc_url->status |= URL_STYLE;
      docu->is_parsable = TRUE;

      /* dirty hack to detect that FTP document downloaded */
      /* through HTTP gateway is directory content */
      if(docu->doc_url->type == URLT_FTP && !docu->doc_url->p.ftp.dir)
      {
        if(resp->ret_code == HTTP_RSP_OK)
        {
          docu->doc_url->p.ftp.dir = TRUE;
          url_changed_filename(docu->doc_url);
        }
      }
    }
    else
    {
      docu->is_parsable = ((docu->doc_url->status & URL_ISSCRIPT) != 0);
    }

    /* use proxy to request resource ! */
    if(resp->ret_code == HTTP_RSP_USE_PROXY)
    {
      int retv = http_do_proxy_redirect(docu);

      if(retv >= 0)
      {
        http_response_free(resp);
        return;
      }
    }

    /* document not found on server */
    if(resp->ret_code == HTTP_RSP_NFOUND)
      docu->doc_url->status |= URL_NOT_FOUND;

    /* proxy authorization required    */
    /* try to authenticate immediately */
    if(resp->ret_code == HTTP_RSP_PROXY_AUTH)
    {
      int retv = http_handle_proxy_auth_info(docu);

      if(retv >= 0)
      {
        http_response_free(resp);
        return;
      }
    }

    /* authorization required          */
    /* try to authenticate immediately */
    if(resp->ret_code == HTTP_RSP_AUTH)
    {
      int retv = http_handle_site_auth_info(docu);

      if(retv >= 0)
      {
        http_response_free(resp);
        return;
      }
    }

    if(docu->rest_pos && (resp->ret_code == HTTP_RSP_OK || resp->ret_code == HTTP_RSP_PART_CONTENT))
    {
      p = get_mime_param_val_str("Content-Range:", docu->mime);
      if(!p)
      {
        if(docu->is_chunked)
        {
          /*
             [i_a] just treat as if loading the first chunk.

             Is this some screwed-up IIS server we're talking to?
           */
          xprintf(1, gettext("Reloading first chunk (RFC2616 chunked download)\n"));
          docu->rest_pos = 0;
        }
        else if(cfg.freget)
        {
          xprintf(1, gettext("Regetting whole file\n"));
          docu->rest_pos = 0;
        }
        else
        {
          docu->errcode = ERR_HTTP_NOREGET;
          docu->is_persistent = FALSE;
          http_response_free(resp);
          return;
        }
      }
      else
      {
        if(resp->ret_code != HTTP_RSP_PART_CONTENT)
        {
          docu->rest_pos = 0;
          xprintf(1, gettext("Modified from last download - regetting whole\n"));
        }
        else
        {
          if(docu->totsz < 0)
          {
            char *p4 = get_mime_param_val_str("Content-Range:", docu->mime);
            if(p4)
            {
              int n1, n2, n3, r;

              r = sscanf(p4, "%*[^0-9]%d-%d/%d", &n1, &n2, &n3);
              _free(p4);

              if(r == 3)
                docu->totsz = n3;
              else if(r == 2)
                docu->totsz = n2 + 1;
            }
          }
          else
          {
            docu->totsz += docu->rest_pos;
          }
        }

        _free(p);
      }
    }

    if(docu->rest_pos && (resp->ret_code != HTTP_RSP_OK && resp->ret_code != HTTP_RSP_PART_CONTENT))
    {
      docu->rest_pos = 0;
      docu->errcode = ERR_HTTP_FAILREGET;
      xprintf(1, gettext("Unexpected response \"%d %s\" when trying to reget!\n"), resp->ret_code, resp->text ? resp->text : "");
    }

    /**** get document creation time ****/
    p = get_mime_param_val_str("Last-Modified:", docu->mime);
    if(p)
    {
      docu->dtime = scntime(p, FALSE);
#if defined(HAVE_DEBUG_FEATURES)
	  if(cfg.debug && (DEBUG_HTML_BITMASK & cfg.debug_level))
	  {
        LOCK_TIME;
        DEBUG_HTML("Last-Modified date decoded: '%s' (original timestamp text: '%s')\n", ctime(&docu->dtime), p);
        UNLOCK_TIME;
	  }
#endif
	  if (docu->dtime == (time_t)-1)
	  {
          xprintf(1, "ERROR: cannot decode Last-Modified date '%s'\n", p + 8);
		  docu->dtime = scntime(p, TRUE);
	  }
      _free(p);
    }
    else if(cfg.mode == MODE_SYNC || cfg.mode == MODE_MIRROR)
    {
      docu->dtime = 0;
    }

    if(docu->check_limits && resp->ret_code == HTTP_RSP_OK)
    {
      cond_info_t condp;
      int r;

      condp.level = 3;
      condp.urlnr = docu->doc_nr;
      condp.size = docu->totsz;
      condp.time = docu->dtime;
      condp.mimet = docu->type_str;
      condp.full_tag = NULL;
      condp.params = NULL;
      condp.html_doc = NULL;
      condp.html_doc_offset = 0;
      condp.tag = NULL;
      condp.attrib = NULL;

      r = url_append_condition(docu->doc_url, &condp);

      if(!r)
      {
        switch (condp.reason)
        {
        case CONDT_MAX_SIZE:
          docu->errcode = ERR_BIGGER;
          break;
        case CONDT_MIN_SIZE:
          docu->errcode = ERR_SMALLER;
          break;
        case CONDT_NEWER_THAN:
        case CONDT_OLDER_THAN:
          docu->errcode = ERR_OUTTIME;
          break;
        case CONDT_USER_CONDITION:
          docu->errcode = ERR_SCRIPT_DISABLED;
          break;
        case CONDT_AMIME_TYPE:
        case CONDT_DMIME_TYPE:
          docu->errcode = ERR_NOMIMET;
          break;
        default:
          docu->errcode = ERR_RDISABLED;
          break;
        }
        docu->is_persistent = FALSE;
        http_response_free(resp);
        return;
      }
    }

    /**************/
    if((cfg.mode == MODE_SYNC || cfg.mode == MODE_MIRROR) &&
      !docu->doreget && (resp->ret_code == HTTP_RSP_ACTUAL || resp->ret_code == HTTP_RSP_OK))
    {
      if(resp->ret_code == HTTP_RSP_ACTUAL)
      {
        docu->errcode = ERR_HTTP_ACTUAL;
        docu->remove_lock = TRUE;
        if(!docu->is_parsable)
        {
          docu->is_parsable = file_is_html(url_to_filename(docu->doc_url, TRUE));
        }
        http_response_free(resp);
        return;
      }
      else if(!docu->is_parsable && docu->origsize && (docu->totsz >= 0) && (docu->origsize != docu->totsz))
      {
        xprintf(1, gettext("Size differs, regeting whole\n"));
      }
      else if(docu->origtime)
      {
        if(!docu->dtime || difftime(docu->dtime, docu->origtime) > 0)
        {
          if(docu->rest_pos)
          {
            docu->rest_pos = 0;
            xprintf(1, gettext("Modified from last download - regeting whole\n"));
          }
        }
        else
        {
          docu->errcode = ERR_HTTP_ACTUAL;
          docu->remove_lock = TRUE;
          docu->is_persistent = FALSE;
          http_response_free(resp);
          return;
        }
      }
    }

    p = NULL;
    len = 0;
    http_response_free(resp);
  }
  else if(!mres)
  {
    /* at least in case of reget we */
    /* can reject HTTP/0.9 response */
    if(cfg.dump_resp && bufio_is_open(cfg.dump_fd) && !cfg.dump_after)
    {
      /* KNOWN BUG: this info will be lost when dump_after is enabled */
      ASSERT(cfg.dump_fd);
      ASSERT(bufio_is_open(cfg.dump_fd));
      bufio_puts(cfg.dump_fd, DUMP_SECTION_SEPSTR);
    }

    if(docu->rest_pos)
    {
      docu->is_persistent = FALSE;
      DEBUG_DEVEL("ERR_HTTP_RCVRESP [%d/%s]\n", __LINE__, __FILE__);
      docu->errcode = ERR_HTTP_RCVRESP;
      return;
    }
    else if(p)
    {
      bufio_unread(docu->datasock, p, len);
      docu->is_parsable = TRUE;
    }
    else
    {
      DEBUG_DEVEL("ERR_HTTP_RCVRESP [%d/%s]\n", __LINE__, __FILE__);
      docu->errcode = ERR_HTTP_RCVRESP;
    }
  }
  else
  {
    docu->is_persistent = FALSE;
    if(!docu->doreget)
      docu->remove_lock = FALSE;
    DEBUG_DEVEL("ERR_HTTP_RCVRESP [%d/%s]\n", __LINE__, __FILE__);
    docu->errcode = ERR_HTTP_RCVRESP;
    return;
  }

  return;
}

bufio *http_get_data_socket(doc * docp)
{
  int rv = -1;

  do
  {
    if(!(docp->datasock = http_open_socket(docp)))
      return NULL;

    if(docp->request_type == HTTP_REQ_HEAD)
    {
      rv = http_head_request(docp);
      if(rv)
      {
        xperror("http_head_request");
      }
    }
    else
    {
      if((docp->doc_url->status & URL_FORM_ACTION))
      {
        form_info *fi = (form_info *) docp->doc_url->extension;

        if((fi->method == FORM_M_POST) || (fi->method == FORM_M_UNKNOWN))
        {
          rv = http_post_request(docp);
          if(rv)
          {
            xperror("http_post_request");
          }
        }
        else if(fi->method == FORM_M_GET)
        {
          int l;
          char *reqstr = form_encode_query(docp->doc_url,
            (form_info *) docp->doc_url->extension, &l);
          char *old_reqstr = docp->doc_url->p.http.searchstr;
          docp->doc_url->p.http.searchstr = reqstr;
          rv = http_get_request(docp);
          if(rv)
          {
            xperror("http_get_request[FORM GET]");
          }
          _free(reqstr);
          docp->doc_url->p.http.searchstr = old_reqstr;
        }
      }
      else
      {
        rv = http_get_request(docp);
        if(rv)
        {
          xperror("http_get_request");
        }
      }
    }

    if(rv)
    {
      if(docp->datasock)
      {
#ifdef USE_SSL
        if(docp->doc_url->type == URLT_HTTPS)
        {
          my_ssl_print_last_error();
        }
#endif
        bufio_close(docp->datasock);
        docp->datasock = NULL;
      }
    }
    else
    {
      http_process_response(docp);
    }
  }
  while(!cfg.stop && !cfg.rbreak && (docp->errcode == ERR_HTTP_CLOSURE));

  return docp->datasock;
}

int http_repeat_request(doc * docp)
{
  int rv;

  if(docp->request_type == HTTP_REQ_CONNECT)
  {
    if(!docp->datasock)
    {
      docp->datasock = bufio_sock_fdopen(net_connect(docp->http_proxy, docp->http_proxy_port, docp));
    }

    if(!docp->datasock)
    {
      if(_h_errno_ != 0)
        xherror(docp->http_proxy);
      else
        xperror("net_connect");

      docp->errcode = ERR_PROXY_CONNECT;
      rv = -1;
    }
    else
    {
      rv = http_dumy_proxy_connect_real(docp, docp->connect_host, docp->connect_port, docp->http_proxy, docp->http_proxy_port);
    }
  }
  else
  {
    rv = http_get_data_socket(docp) ? 0 : -1;
  }

  return rv;
}

/**********************************************/
/* parse HTTP server response status line     */
/**********************************************/
/* [i_a] adapted so we can use it for the hammer code too; no time spent on malloc! */
int http_get_response_info2(http_response * dst, const char *doc_txt)
{
  char *p;
  char pom[1024];
  int len;

  ASSERT(dst);
  memset(dst, 0, sizeof(*dst));
  if(!doc_txt)
    return -1;

  len = strcspn(doc_txt, "\r\n");

  if(len)
  {
    if(len >= sizeof(pom))
      len = sizeof(pom) - 1;

    strncpy(pom, doc_txt, len);
    *(pom + len) = '\0';

    if(!strncmp(pom, "HTTP/", 5))
    {
      p = pom + 5;
      dst->ver_maj = atoi(p);

      p += strspn(p, "0123456789");
      dst->ver_min = 0;
      if(*p == '.');
      {
        p++;
        dst->ver_min = atoi(p);
      }

      p += strspn(p, "0123456789");
      p += strspn(p, " ");
      dst->ret_code = atoi(p);

      p += strspn(p, "0123456789");
      p += strspn(p, " ");

      dst->text = tl_strdup(p);

      return 0;
    }
	else
	{
		/*
		   try to handle non RFC compliant web server responses.

		   Assume the format: <unidentified token> <response code> <response message>

		   where <unidentified token> apparently is something else than the RFC-specified
		   'HTTP/major.minor' text.
	     */

      p = pom;
		/* assume a HTTP/1.0 response */
		dst->ver_maj = 1;
        dst->ver_min = 0;

	    p += strcspn(p, " ");
        p += strspn(p, " ");
        dst->ret_code = atoi(p);

      p += strspn(p, "0123456789");
      p += strspn(p, " ");

      dst->text = tl_strdup(p);

	  /* make sure at least the response code is one of the RFC-defined ones! */
	  if (is_valid_http_response_code(dst->ret_code))
	  {
		  xprintf(0, "non-RFC-compliant HTTP response received. pavuk guestimated HTTP/1.x equivalent response code: %d for this input: '%s'\n",
			  dst->ret_code, pom);
	  }
	  else
	  {
		  xprintf(1, "non-RFC-compliant HTTP response received. pavuk cannot guestimate a HTTP equivalent for this input: '%s'\n",
			  dst->ret_code, pom);
		  return -1;
	  }

      return 0;
	}
  }
  return -1;
}

/**********************************************/
/* parse HTTP server response status line     */
/**********************************************/
http_response *http_get_response_info(const char *doc_txt)
{
  http_response *ret_val = _malloc(sizeof(*ret_val));

  if(http_get_response_info2(ret_val, doc_txt))
  {
	_free(ret_val->text);
    _free(ret_val);
    ret_val = NULL;
  }
  return ret_val;
}

void http_response_free(http_response * resp)
{
  if (resp)
  {
	  _free(resp->text);
	  _free(resp);
  }
}

/****************************************/
/* just read document body and throw it */
/****************************************/
int http_throw_message_body(doc * docp)
{
  char buf[256];
  int len;
  int sr;

  sr = docp->rest_pos;
  docp->rest_pos = 0;

  docp->size = 0;
  while((len = abs_read_data(docp, docp->datasock, buf, sizeof(buf))) > 0)
  {
    docp->size += len;

    if(cfg.dump_resp && bufio_is_open(cfg.dump_fd) && !cfg.dump_after)
    {
      ASSERT(cfg.dump_fd);
      ASSERT(bufio_is_open(cfg.dump_fd));
      bufio_asciidump(cfg.dump_fd, buf, len, NULL);
    }
  }

  docp->size = 0;
  docp->rest_pos = sr;

  return len;
}

/************************************************/
/* read whole HTTP server response into buf     */
/************************************************/
int http_read_mime_header(doc * docp, char **buf, int *len)
{
  char pom[BUFIO_ADVISED_READLN_BUFSIZE];
  char *rb;
  int alen, tlen = 0;
  rb = NULL;

  *buf = NULL;

  if((alen = abs_readln(docp->datasock, pom, sizeof(pom))) > 0)
  {
    if(!strncmp(pom, "HTTP/", 5))
    {
      rb = _malloc(alen + 1);
      tlen += alen;
      memmove(rb, pom, alen + 1);
      while((alen = abs_readln(docp->datasock, pom, sizeof(pom))) > 0)
      {
        tlen += alen;
        rb = _realloc(rb, tlen + 1);
        memmove(rb + tlen - alen, pom, alen + 1);
        *(rb + tlen) = '\0';
        if(pom[0] == '\r' || pom[0] == '\n')
          break;
      }
      if(alen < 0)
      {
        docp->errcode = ERR_READ;
        xperror("http_response");
        _free(rb);
        *len = 0;
        *buf = NULL;
        return -1;
      }
      else
      {
        *len = tlen;
        *buf = rb;
        return 1;
      }
    }
    else
    {
      *len = tlen;
      *buf = rb;
      return 0;
    }
  }
  else
  {
    *buf = NULL;
    *len = 0;
    if(alen < 0)
    {
      docp->errcode = ERR_READ;
      xperror("http_response");
      return -1;
    }
    return 0;
  }
}
