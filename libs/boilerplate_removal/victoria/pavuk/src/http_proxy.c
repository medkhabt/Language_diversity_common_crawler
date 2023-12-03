/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "http.h"
#include "net.h"
#include "tools.h"
#include "mime.h"

void http_proxy_free(http_proxy * hp)
{
  _free(hp->addr);
  _free(hp);
}

int http_proxy_ref(http_proxy * hp)
{
  return ++hp->ref;
}

int http_proxy_unref(http_proxy * hp)
{
  int rv;

  rv = --hp->ref;
  if(hp->ref <= 0)
    http_proxy_free(hp);

  return rv;
}

http_proxy *http_proxy_get(void)
{
  http_proxy *rv = NULL;
  dllist *ptr;

  ptr = cfg.last_used_proxy_node ? cfg.last_used_proxy_node->next : cfg.http_proxy;

  for(; cfg.http_proxy;)
  {
    http_proxy *pr;

    if(!ptr)
      ptr = cfg.http_proxy;

    pr = (http_proxy *) ptr->data;

    if(pr->penault == 0)
    {
      cfg.last_used_proxy_node = ptr;
      break;
    }
    else
      pr->penault--;

    ptr = ptr->next;
  }

  if(cfg.last_used_proxy_node)
    rv = (http_proxy *) cfg.last_used_proxy_node->data;

  return rv;
}

http_proxy *http_proxy_find(char *host, int port)
{
  dllist *ptr;

  for(ptr = cfg.http_proxy; ptr; ptr = ptr->next)
  {
    http_proxy *hp = (http_proxy *) ptr->data;

    if(hp->port == port && !strcasecmp(host, hp->addr))
      return hp;
  }

  return NULL;
}

http_proxy *http_proxy_parse(char *pstr)
{
  http_proxy *rv = NULL;
  unsigned short port;
  char *p;

  port = DEFAULT_HTTP_PROXY_PORT;
  p = strchr(pstr, ':');
  if(p)
  {
    port = _atoi(p + 1);
    if(errno == ERANGE)
    {
      struct servent *se;

      if((se = getservbyname(p + 1, "tcp")))
      {
        port = ntohs(se->s_port);
      }
      else
      {
        xprintf(1, gettext("Unknown port \"%s\"\n"), p + 1);
      }
    }
  }
  if(!port)
    port = DEFAULT_HTTP_PROXY_PORT;

  rv = _malloc(sizeof(*rv));

  rv->addr = p ? tl_strndup(pstr, p - pstr) : tl_strdup(pstr);
  rv->port = port;
  rv->penault = 0;
  rv->fails = 0;
  rv->is_10 = -1;
  rv->ref = 1;

  return rv;
}

void http_proxy_check(http_proxy * hp, doc * docp)
{
  bufio *sock;
  char pom[BUFIO_ADVISED_READLN_BUFSIZE];
  int len;
  http_response *resp;

  if(hp->is_10 >= 0)
    return;

#if 0
//  /* MJF: HACK ATTACK -- when performance is being measured */
//  /* This test screws everything up. */
//  if(cfg.time_logfile->fd >= 0)
//  {
//    hp->is_10 = 0;
//    return;
//  }
#endif

  xprintf(1, gettext("Checking HTTP proxy server %s:%hu\n"), hp->addr, hp->port);

  sock = bufio_sock_fdopen(net_connect(hp->addr, hp->port, docp));
  if(!sock)
  {
    if(_h_errno_ != 0)
      xherror(hp->addr);
    else
      xperror("net_connect");

    xprintf(1, gettext("Failed to check proxy (sock open)!\n"));
    return;
  }

  snprintf(pom, sizeof(pom), "OPTIONS * HTTP/1.1\r\nHost: %s:%hu\r\n\r\n", hp->addr, hp->port);
  pom[sizeof(pom) - 1] = 0;

  len = strlen(pom);
  DEBUG_PROTOC("****************** Proxy check request *****************\n");
  DEBUG_PROTOC("%s", pom);
  DEBUG_PROTOC("********************************************************\n");
  if(bufio_write(sock, pom, len) != len)
  {
    xperror("write (proxy)");
    bufio_close(sock);
    sock = NULL;
    xprintf(1, gettext("Failed to check proxy (write failed)!\n"));
    return;
  }

  if((len = bufio_readln(sock, pom, sizeof(pom) - 1)) <= 0)
  {
    bufio_close(sock);
    sock = NULL;
    xprintf(1, gettext("Failed to check proxy (read failed)!\n"));
    return;
  }
  else
  {
	  ASSERT(len < sizeof(pom));
	  pom[len] = 0;
    DEBUG_PROTOS("***************** Proxy check response *****************\n");
    DEBUG_PROTOS("%.*s%s",
		(strlen(pom) > 1024 ? 1024 : strlen(pom)),
		pom,
		(strlen(pom) > 1024 ? "(Truncated)" : ""));
    DEBUG_PROTOS("**********************************************************\n");
  }

  bufio_close(sock);
  sock = NULL;

  resp = http_get_response_info(pom);
  if(!resp)
  {
    xprintf(1, gettext("Failed to check proxy (bad response)!\n"));
    return;
  }

  hp->is_10 = (resp->ver_maj == 1 && resp->ver_min == 0);

  xprintf(1, gettext("Proxy %s:%hu is HTTP/%d.%d proxy\n"), hp->addr, hp->port, resp->ver_maj, resp->ver_min);

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  if(docp->actrec_ref)
  {
    activity_record *rec = docp->actrec_ref;

    DEBUG_DEVEL("recording response code: %d [%d/%s]\n", resp->ret_code, __LINE__, __FILE__);
    rec->expected_response_code = resp->ret_code;
  }
#endif

  http_response_free(resp);
}