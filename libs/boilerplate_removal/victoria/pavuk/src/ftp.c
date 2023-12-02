/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "absio.h"
#include "abstract.h"
#include "dns.h"
#include "doc.h"
#include "errcode.h"
#include "ftp.h"
#include "http.h"
#include "mode.h"
#include "myssl.h"
#include "net.h"
#include "times.h"
#include "tools.h"
#include "uexit.h"
#include "url.h"


static ftp_handshake_info *ftp_handshake_info_data_find(char *host, int port);
static int ftp_do_login_handshake_cust(ftp_handshake_info *fhi, doc *docp, char *user, char *password);


static int ftp_control_write(doc *docp, char *buf, size_t bufsize)
{
  return bufio_write(docp->ftp_control, buf, bufsize);
}


static int ftp_control_readln(doc *docp, char *buf, int bufsize)
{
  return bufio_readln(docp->ftp_control, buf, bufsize);
}


/*
 * Parse FTP server response line, which must be formatted
 * as
 *
 *    NNN text
 *
 * where NNN is a numeric response code as defined per RFC959 et al.
 *
 * The line received from the server is returned through '*mbuf'
 * (the string will be allocated on the heap).
 *
 * The FTP server response code will be returned as the function
 * return value.
 *
 * If something went wrong in unexpected ways, the return code will
 * be '600', which is a response code outside the range as defined
 * per the RFCs mentioned above.
 *
 * NOTES:
 *
 * 'mbuf' may be NULL.
 *
 * 'setrc' may be set to TRUE to request this routine to store
 * the response code in 'docp->ftp_respc'.
 */
int ftp_get_response(doc *docp, char **mbuf, int setrc)
{
  char buf[BUFIO_ADVISED_READLN_BUFSIZE];
  bool_t end = FALSE;
  int alen, tlen = 0;
  char *sresp = NULL;

  while (!end)
  {
    if ((alen = ftp_control_readln(docp, buf, sizeof(buf))) > 0)
    {
      DEBUG_PROTOS("%s", buf);
      if (mbuf)
      {
        tlen += alen;
        sresp = _realloc(sresp, tlen + 1);
        memcpy(sresp + tlen - alen, buf, alen + 1);
      }

      if (strlen(buf) > 3)
      {
        if (tl_ascii_isdigit(*(buf))
            && tl_ascii_isdigit(*(buf + 1))
            && tl_ascii_isdigit(*(buf + 2))
            && tl_ascii_isspace(*(buf + 3)))
        {
          end = TRUE;
        }
      }
    }
    else
    {
      if (alen)
        xperror("ftp_get_response: ftp control connection closed before any response");
      else
        xprintf(1, gettext("ftp_get_response: ftp control connection closed before any response\n"));
      if (setrc)
        docp->ftp_respc = 600;
      return 600;
    }
  }

  if (mbuf)
    *mbuf = sresp;

  if (setrc)
    docp->ftp_respc = _atoi(buf);

  return _atoi(buf);
}


static bufio *ftp_open_data_finish(doc *docp)
{
  if (docp->ftp_data_con_finished && docp->datasock)
    return docp->datasock;

  if (!cfg.ftp_activec || (priv_cfg.ftp_proxy && cfg.ftp_dirtyp))
  {
    ASSERT(!docp->datasock);
    if (priv_cfg.ftp_proxy && cfg.ftp_dirtyp)
    {
      if (!(docp->datasock = bufio_sock_fdopen(
              net_connect(priv_cfg.ftp_proxy, cfg.ftp_proxy_port, docp))))
      {
        if (_h_errno_ != 0)
        {
          xvherror(DBGvars(priv_cfg.ftp_proxy),
                   "net_connect(): Cannot connect to proxy @ port %d\n",
                   cfg.ftp_proxy_port);
        }
        else
        {
          xvperror(DBGvars(priv_cfg.ftp_proxy),
                   "net_connect(): Cannot connect to proxy @ port %d\n",
                   cfg.ftp_proxy_port);
        }

        docp->errcode = ERR_PROXY_CONNECT;
        return NULL;
      }
      if (http_dummy_proxy_connect(docp, docp->ftp_pasv_host, docp->ftp_pasv_port,
                                   priv_cfg.ftp_proxy, cfg.ftp_proxy_port))
      {
        docp->errcode = ERR_PROXY_CONNECT;
        bufio_close(docp->datasock);
        docp->datasock = NULL;
        return NULL;
      }
    }
    else
    {
      if (!(docp->datasock = bufio_sock_fdopen(
              net_connect(docp->ftp_pasv_host, docp->ftp_pasv_port, docp))))
      {
        if (_h_errno_ != 0)
        {
          xvherror(DBGvars(docp->ftp_pasv_host),
                   "net_connect(): Cannot connect to PASSIVE host @ port %d\n",
                   docp->ftp_pasv_port);
        }
        else
        {
          xvperror(DBGvars(docp->ftp_pasv_host),
                   "net_connect(): Cannot connect to PASSIVE host @ port %d\n",
                   docp->ftp_pasv_port);
        }

        docp->errcode = ERR_FTP_DATACON;
        return NULL;
      }
    }
  }
  else
  {
    bufio *dsock = bufio_sock_fdopen(net_accept(bufio_getfd(docp->datasock)));
    if (!dsock)
    {
      const char *host;
      int port;

      host = (docp->ftp_pasv_host ? docp->ftp_pasv_host : docp->doc_url
              ? docp->doc_url->p.ftp.host : NULL);
      port = (docp->ftp_pasv_port ? docp->ftp_pasv_port : docp->doc_url
              ? docp->doc_url->p.ftp.port : 0);
      if (_h_errno_ != 0)
      {
        xvherror(DBGvars(host),
                 "net_accept(): Cannot accept a PASSIVE connection from host (port: %s)\n",
                 port);
      }
      else
      {
        xvperror(DBGvars(host),
                 "net_accept(): Cannot accept a PASSIVE connection from host (port: %d)\n",
                 port);
      }
    }
    bufio_close(docp->datasock);
    docp->datasock = dsock;
#if 0 /* [i_a] TO BE TESTED: shouldn't it be like this when this accept() fails? */
    docp->errcode = ERR_FTP_DATACON;
    return NULL;

#endif
  }

#ifdef IP_TOS
#ifdef IPTOS_THROUGHPUT
  if (docp->datasock)
  {
    int v = IPTOS_THROUGHPUT;

    if (setsockopt(bufio_getfd(docp->datasock), IPPROTO_IP, IP_TOS, (char *)&v, sizeof(v)))
    {
      xperror(gettext("ftp: setsockopt TOS - THROUGHPUT failed"));
    }
  }
#endif
#endif

#ifdef USE_SSL
  if (docp->doc_url->type == URLT_FTPS)
  {
    bufio *ssl_sock;

    ssl_sock = my_ssl_do_connect(docp, docp->datasock, bufio_get_ssl_hook_data(docp->ftp_control));

    if (!ssl_sock)
    {
      if (!docp->errcode)
        docp->errcode = ERR_FTPS_DATASSLCONNECT;
      bufio_close(docp->datasock);
      docp->datasock = NULL;
    }
    else
    {
      docp->datasock = ssl_sock;
    }
  }
#endif

  docp->ftp_data_con_finished = (docp->datasock != NULL);

  return docp->datasock;
}


static int ftp_open_data_init(doc *docp)
{
  char buf[256];
  char *mbuf;
  struct sockaddr_storage scaddr;
  struct sockaddr *caddr;
  int reply;
  socklen_t socklen;
  int n;

  caddr = (struct sockaddr *)&scaddr;

  socklen = sizeof(scaddr);
  if (getsockname(bufio_getfd(docp->ftp_control), caddr, &socklen))
  {
    xperror("FTP control getsockname");
    docp->datasock = NULL;
    return -1;
  }

  if (!cfg.ftp_activec || (priv_cfg.ftp_proxy && cfg.ftp_dirtyp))
  {
#ifdef HAVE_INET6
    if (caddr->sa_family == AF_INET6)
    {
      strcpy(buf, "EPSV\r\n");
      DEBUG_PROTOC("%s", buf);

      ftp_control_write(docp, buf, strlen(buf));

      mbuf = NULL;

      if ((reply = ftp_get_response(docp, &mbuf, TRUE)) >= 400)
      {
        int h[16], p[2], hal, af, pal;
        abs_addr haddr;

        _free(mbuf);
        if (reply == 600)
        {
          docp->ftp_fatal_err = TRUE;
          return -1;
        }
        strcpy(buf, "LPSV\r\n");
        DEBUG_PROTOC("%s", buf);

        ftp_control_write(docp, buf, strlen(buf));

        mbuf = NULL;

        if ((reply = ftp_get_response(docp, &mbuf, TRUE)) >= 400)
        {
          docp->ftp_fatal_err = (reply == 600);
          _free(mbuf);
          return -1;
        }
        n = sscanf(mbuf,
                   "%d %*[^0-9]%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                   &reply,
                   &af,
                   &hal,
                   &h[0],
                   &h[1],
                   &h[2],
                   &h[3],
                   &h[4],
                   &h[5],
                   &h[6],
                   &h[7],
                   &h[8],
                   &h[9],
                   &h[10],
                   &h[11],
                   &h[12],
                   &h[13],
                   &h[14],
                   &h[15],
                   &pal,
                   &p[0],
                   &p[1]);
        if (n != 22 || reply != 228 || af != 6 || hal != 16 || pal != 2)
        {
          xprintf(1, gettext("Error parsing FTP response to %s command - %s\n"), "LPSV", mbuf);
          _free(mbuf);
          return -1;
        }
        _free(mbuf);
        docp->ftp_pasv_port = (p[0] << 8) + p[1];
        haddr.family = AF_INET6;
        for (n = 0; n < 16; n++)
          haddr.addr[n] = (unsigned char)h[n];
        docp->ftp_pasv_host = dns_get_abs_addr_ip(&haddr);
      }
      else
      {
        int port;
        char d[4];

        n = sscanf(mbuf, "%d %*[^0-9(]\(%c%c%c%d%c", &reply, &d[0], &d[1], &d[2], &port, &d[3]);
        if (n != 6 || reply != 229 || d[0] != d[1] || d[0] != d[2] || d[0] != d[3])
        {
          xprintf(1, gettext("Error parsing FTP response to %s command - %s\n"), "EPSV", mbuf);
          _free(mbuf);
          return -1;
        }
        _free(mbuf);
        docp->ftp_pasv_port = port;
        docp->ftp_pasv_host = dns_get_sockaddr_ip(caddr);
      }
    }
    else
#endif
    {
      int h0, h1, h2, h3, p0, p1;

      strcpy(buf, "PASV\r\n");
      DEBUG_PROTOC("%s", buf);

      ftp_control_write(docp, buf, strlen(buf));

      mbuf = NULL;

      if ((reply = ftp_get_response(docp, &mbuf, TRUE)) >= 400)
      {
        docp->ftp_fatal_err = (reply == 600);
        _free(mbuf);
        return -1;
      }

      if (!mbuf)
      {
        return -1;
      }

      n = sscanf(mbuf, "%d %*[^0-9]%d,%d,%d,%d,%d,%d", &reply, &h0, &h1, &h2, &h3, &p0, &p1);


      if (n != 7 || reply != FTP_RSP_ERR227)
      {
        xprintf(1, gettext("Error parsing FTP response to %s command - %s\n"), "PASV", mbuf);
        _free(mbuf);
        return -1;
      }
      _free(mbuf);

      docp->ftp_pasv_port = (p0 << 8) + p1;

      sprintf(buf, "%d.%d.%d.%d", h0, h1, h2, h3);
      docp->ftp_pasv_host = tl_strdup(buf);
    }
  }
  else
  {
    struct sockaddr_storage saddr;
    struct sockaddr *addr;
    char *p;
    int port;
    socklen_t socklen;

    addr = (struct sockaddr *)&saddr;

    if (!docp->datasock)
    {
      int bfd;
      abs_addr laddr;

      docp->ftp_data_con_finished = FALSE;

      memset(laddr.addr, '\0', sizeof(laddr.addr));
      laddr.family = caddr->sa_family;

      if (priv_cfg.local_ip)
      {
        memcpy(laddr.addr, cfg.local_ip_addr.addr, sizeof(laddr.addr));
        laddr.family = cfg.local_ip_addr.family;
      }

      bfd = net_bindport(&laddr, cfg.active_ftp_min_port, cfg.active_ftp_max_port);
      if (!(docp->datasock = bufio_sock_fdopen(bfd)))
      {
        xvperror(DBGvars(NULL),
                 "bind: failed to bind to local port in range %d..%d\n",
                 cfg.active_ftp_min_port, cfg.active_ftp_max_port);
        return -1;
      }
    }

    socklen = sizeof(saddr);
    if (getsockname(bufio_getfd(docp->datasock), addr, &socklen))
    {
      xperror("FTP data getsockname");
      bufio_close(docp->datasock);
      docp->datasock = NULL;
      return -1;
    }

    port = dns_get_sockaddr_port(addr);
    mbuf = dns_get_sockaddr_ip(caddr);
#ifdef HAVE_INET6
    if (caddr->sa_family == AF_INET6)
    {
      snprintf(buf, sizeof(buf), "EPRT |2|%s|%d|\r\n", mbuf, port);
      buf[sizeof(buf) - 1] = 0;
      _free(mbuf);

      ftp_control_write(docp, buf, strlen(buf));
      DEBUG_PROTOC("%s", buf);

      if ((reply = ftp_get_response(docp, NULL, TRUE)) >= 400)
      {
        unsigned char *h;

        if (reply == 600)
        {
          bufio_close(docp->datasock);
          docp->datasock = NULL;
          docp->ftp_fatal_err = TRUE;
          return -1;
        }
        h = ((struct sockaddr_in6 *)caddr)->sin6_addr.s6_addr;

        sprintf(buf,
                "LPRT 6,16,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,2,%d,%d\r\n",
                h[0],
                h[1],
                h[2],
                h[3],
                h[4],
                h[5],
                h[6],
                h[7],
                h[8],
                h[9],
                h[10],
                h[11],
                h[12],
                h[13],
                h[14],
                h[15],
                port / 256,
                port % 256);

        ftp_control_write(docp, buf, strlen(buf));
        DEBUG_PROTOC("%s", buf);

        if ((reply = ftp_get_response(docp, NULL, TRUE)) >= 400)
        {
          bufio_close(docp->datasock);
          docp->datasock = NULL;
          docp->ftp_fatal_err = (reply == 600);
          return -1;
        }
      }
    }
    else
#endif
    {
      for (p = mbuf; (p = strchr(p, '.')); *p = ',')
        ;
      snprintf(buf, sizeof(buf), "PORT %s,%d,%d\r\n", mbuf, port / 256, port % 256);
      buf[sizeof(buf) - 1] = 0;
      _free(mbuf);

      ftp_control_write(docp, buf, strlen(buf));
      DEBUG_PROTOC("%s", buf);

      if ((reply = ftp_get_response(docp, NULL, TRUE)) >= 400)
      {
        bufio_close(docp->datasock);
        docp->datasock = NULL;
        docp->ftp_fatal_err = (reply == 600);
        return -1;
      }
    }
  }

  return 0;
}

static int ftp_do_login_handshake(doc *docp, char *ruser, char *user, char *password)
{
  ftp_handshake_info *fhi;

  fhi = ftp_handshake_info_data_find(docp->doc_url->p.ftp.host, docp->doc_url->p.ftp.port);

  if (!fhi)
    fhi = ftp_handshake_info_data_find("", DEFAULT_FTP_PORT);

  if (fhi)
  {
    if (ftp_do_login_handshake_cust(fhi, docp, ruser, password))
    {
      docp->ftp_fatal_err = TRUE;
      docp->errcode = ERR_FTP_LOGIN_HANDSHAKE;
      return -1;
    }
  }
  else
  {
    char buf[2048];
    int reply;

    if (priv_cfg.ftp_proxy_user && priv_cfg.ftp_proxy && !cfg.ftp_via_http && !cfg.ftp_dirtyp)
    {
      snprintf(buf, sizeof(buf), "USER %s\r\n", priv_cfg.ftp_proxy_user);
      buf[sizeof(buf) - 1] = 0;
      ftp_control_write(docp, buf, strlen(buf));
      DEBUG_PROTOC("%s", buf);

      if ((reply = ftp_get_response(docp, NULL, TRUE)) >= 400)
      {
        docp->ftp_fatal_err = TRUE;
        docp->errcode = ERR_FTP_BPROXYUSER;
        return -1;
      }
    }
    else
    {
      reply = 0;
    }

    if (priv_cfg.ftp_proxy_pass && priv_cfg.ftp_proxy && !cfg.ftp_via_http && !cfg.ftp_dirtyp)
    {
      if (reply == FTP_RSP_ERR331)  /*** we need password ***/
      {
        snprintf(buf, sizeof(buf), "PASS %s\r\n", priv_cfg.ftp_proxy_pass);
        buf[sizeof(buf) - 1] = 0;
        ftp_control_write(docp, buf, strlen(buf));
        DEBUG_PROTOC("%s", buf);

        if (ftp_get_response(docp, NULL, TRUE) >= 400)
        {
          docp->ftp_fatal_err = TRUE;
          docp->errcode = ERR_FTP_BPROXYPASS;
          return -1;
        }
      }
    }

    snprintf(buf, sizeof(buf), "USER %s\r\n", user);
    buf[sizeof(buf) - 1] = 0;
    ftp_control_write(docp, buf, strlen(buf));
    DEBUG_PROTOC("%s", buf);

    if ((reply = ftp_get_response(docp, NULL, TRUE)) >= 400)
    {
      docp->ftp_fatal_err = TRUE;
      docp->errcode = ERR_FTP_BUSER;
      return -1;
    }

    if (reply == FTP_RSP_ERR331)  /*** we need password ***/
    {
      snprintf(buf, sizeof(buf), "PASS %s\r\n", password);
      buf[sizeof(buf) - 1] = 0;
      ftp_control_write(docp, buf, strlen(buf));
      DEBUG_PROTOC("%s", buf);

      if (ftp_get_response(docp, NULL, TRUE) >= 400)
      {
        docp->ftp_fatal_err = TRUE;
        docp->errcode = ERR_FTP_BPASS;
        return -1;
      }
    }
  }

  return 0;
}

#define CLOSE_CONTROL \
  bufio_close(docp->ftp_control); \
  docp->ftp_control = NULL;

static bufio *ftp_open_control_connection(doc * docp, char *host, int port, char *ruser, char *user, char *password)
{
  docp->errcode = ERR_NOERROR;
  docp->ftp_fatal_err = FALSE;
  docp->ftp_respc = 0;

  if (!docp->ftp_control)
  {
    if (priv_cfg.ftp_proxy && cfg.ftp_dirtyp)
    {
      if (!(docp->datasock = bufio_sock_fdopen(
              net_connect(priv_cfg.ftp_proxy, cfg.ftp_proxy_port, docp))))
      {
        if (_h_errno_ != 0)
        {
          xvherror(DBGvars(priv_cfg.ftp_proxy),
                   "net_connect(): Cannot connect to proxy @ port %d\n",
                   cfg.ftp_proxy_port);
        }
        else
        {
          xvperror(DBGvars(priv_cfg.ftp_proxy),
                   "net_connect(): Cannot connect to proxy @ port %d\n",
                   cfg.ftp_proxy_port);
        }

        docp->errcode = ERR_PROXY_CONNECT;
        return NULL;
      }

      if (http_dummy_proxy_connect(docp, host, port, priv_cfg.ftp_proxy, cfg.ftp_proxy_port))
      {
        docp->errcode = ERR_PROXY_CONNECT;
        return NULL;
      }
      docp->ftp_control = docp->datasock;
      docp->datasock = NULL;
    }
    else
    {
      if (!(docp->ftp_control = bufio_sock_fdopen(net_connect(host, port, docp))))
      {
        if (_h_errno_ != 0)
        {
          xvherror(DBGvars(host), "net_connect(): Cannot connect to host @ port %d\n",
                   port);
        }
        else
        {
          xvperror(DBGvars(host), "net_connect(): Cannot connect to host @ port %d\n",
                   port);
        }

        docp->errcode = ERR_FTP_CONNECT;
        docp->ftp_fatal_err = TRUE;
        return NULL;
      }
    }
    if (ftp_get_response(docp, NULL, TRUE) >= 400)
    {
      CLOSE_CONTROL;
      docp->ftp_fatal_err = TRUE;
      docp->errcode = ERR_FTP_CONNECT;
      return NULL;
    }
#ifdef USE_SSL
    if (docp->doc_url->type == URLT_FTPS)
    {
      char buf[2048];
      int reply;
      bufio *ssl_sock;

      strcpy(buf, "AUTH SSL\r\n");
      ftp_control_write(docp, buf, strlen(buf));
      DEBUG_PROTOC("%s", buf);

      reply = ftp_get_response(docp, NULL, TRUE);

      /*** 234 code is used by SafeGossip ***/
      /*** thank to Martijn ...           ***/
      if (reply != FTP_RSP_ERR334 && reply != FTP_RSP_ERR234)
      {
        CLOSE_CONTROL;
        docp->ftp_fatal_err = (reply == 600);
        docp->errcode = ERR_FTPS_UNSUPPORTED;
        return NULL;
      }

      ssl_sock = my_ssl_do_connect(docp, docp->ftp_control, NULL);
      if (!ssl_sock)
      {
        if (!docp->errcode)
          docp->errcode = ERR_FTPS_CONNECT;
        CLOSE_CONTROL;
        return NULL;
      }
      else
      {
        docp->ftp_control = ssl_sock;
      }
    }
#endif

    if (ftp_do_login_handshake(docp, ruser, user, password))
    {
      CLOSE_CONTROL;
      return NULL;
    }

#ifdef IP_TOS
#ifdef IPTOS_LOWDELAY
    if (docp->ftp_control)
    {
      int v = IPTOS_LOWDELAY;
      if (setsockopt(bufio_getfd(docp->ftp_control), IPPROTO_IP, IP_TOS, (char *)&v, sizeof(v)))
      {
        xperror("ftp: setsockopt TOS - LOWDELAY failed");
      }
    }
#endif
#endif
  }
#ifdef HAVE_DEBUG_FEATURES
  else
  {
    DEBUG_PROTOC("Re-using established FTP control connection\n");
  }
#endif

  return docp->ftp_control;
}

static int ftp_set_transfer_type(doc *docp, char *type)
{
  char buf[20];
  int reply;

  snprintf(buf, sizeof(buf), "TYPE %s\r\n", type);
  buf[sizeof(buf) - 1] = 0;
  ftp_control_write(docp, buf, strlen(buf));
  DEBUG_PROTOC("%s", buf);

  if ((reply = ftp_get_response(docp, NULL, TRUE)) >= 400)
  {
    CLOSE_CONTROL;
    docp->ftp_fatal_err = (reply == 600);
    docp->errcode = ERR_FTP_UNKNOWN;
    return -1;
  }

  return 0;
}

static int ftp_send_list_command(doc *docp, char *filename, bool_t nlst_or_list_dir)
{
  char buf[2148];
  int l;

  l = strlen(filename) - 1;
  if (l)
  {
    if (tl_is_dirname(filename + 1) && strcmp(filename, "//"))
      l--;
  }

  if (nlst_or_list_dir)
  {
    strcpy(buf, "LIST");
  }
  else
  {
    strcpy(buf, "NLST");
  }
  docp->ftp_dir_type = nlst_or_list_dir;

  if (cfg.ftp_list_options)
  {
    strcat(buf, " ");
    strncat(buf, cfg.ftp_list_options, sizeof(buf) - strlen(buf));
    buf[sizeof(buf) - 1] = '\0';
  }

  if (l)
  {
    strncat(buf, " ", sizeof(buf) - strlen(buf));
    buf[sizeof(buf) - 1] = '\0';
    strncat(buf, filename + 1, sizeof(buf) - strlen(buf));
    buf[sizeof(buf) - 1] = '\0';
  }

  strncat(buf, "\r\n", sizeof(buf) - strlen(buf));
  buf[sizeof(buf) - 1] = '\0';

  ftp_control_write(docp, buf, strlen(buf));
  DEBUG_PROTOC("%s", buf);

  return 0;
}

static int ftp_check_directory_existence(doc *docp, char *filename, bool_t nlst_or_list_dir)
{
  int rv = 0;

  if (nlst_or_list_dir && cfg.fix_wuftpd)
  {
    int l;
    char buf[2148];
    char *mbuf;

    l = strlen(filename) - 1;
    if (l)
    {
      if (tl_is_dirname(filename + 1) && strcmp(filename, "//"))
        l--;
    }

    strcpy(buf, "STAT -d1 ");
    if (l)
    {
      strncat(buf, filename + 1, sizeof(buf) - strlen(buf));
      buf[sizeof(buf) - 1] = '\0';
      strncat(buf, "\r\n", sizeof(buf) - strlen(buf));
      buf[sizeof(buf) - 1] = '\0';
    }
    else
    {
      /* we can assume that login directory exists */
      return 0;
    }

    ftp_control_write(docp, buf, strlen(buf));
    DEBUG_PROTOC("%s", buf);

    mbuf = NULL;
    if (ftp_get_response(docp, &mbuf, TRUE) == FTP_RSP_ERR213)
    {
      char *p;

      p = mbuf + strcspn(mbuf, "\r\n");
      p += strspn(p, "\r\n");
      if (strncmp(filename + 1, p, strcspn(p, "\r\n")))
        rv = -1;
    }

    _free(mbuf);
  }

  return rv;
}

/****************************************************************/
/* otvorenie spojenia na FTP server, prihlasenie + zahajenie    */
/* prenosu suboru / adresara                                    */
/* FIXME: Translate me!                                         */
/****************************************************************/
static bufio *ftp_connect(doc * docp, char *host, int port, char *ruser, char *user, char *password, char *filename, bool_t * is_dir)
{
  char buf[2148];
  int reply = 0;
  char *mbuf;

  docp->errcode = ERR_NOERROR;
  docp->ftp_fatal_err = FALSE;
  docp->ftp_respc = 0;

  if (!(docp->ftp_control = ftp_open_control_connection(docp, host, port, ruser, user, password)))
    return NULL;

  if (!tl_is_dirname(filename) && cfg.mode != MODE_FTPDIR)
  {
    if (docp->doc_url->extension && ((ftp_url_extension *)docp->doc_url->extension)->size > 0)
    {
      docp->totsz = ((ftp_url_extension *)docp->doc_url->extension)->size;
      reply = FTP_RSP_ERR213;
      DEBUG_PROTOC("taking stored size\n");
    }
    else
    {
      snprintf(buf, sizeof(buf), "SIZE %s\r\n", filename + 1);
      buf[sizeof(buf) - 1] = 0;
      ftp_control_write(docp, buf, strlen(buf));
      DEBUG_PROTOC("%s", buf);


      mbuf = NULL;
      if ((reply = ftp_get_response(docp, &mbuf, TRUE)) == FTP_RSP_ERR213)
      {
        int sz;

        sscanf(mbuf, "%d %d", &reply, &sz);
        docp->totsz = sz;
      }
      else
      {
        if (reply == 600)
        {
          docp->errcode = ERR_READ;
          docp->ftp_fatal_err = TRUE;
          _free(mbuf);
          return NULL;
        }
      }

      _free(mbuf);
    }

    if (reply != FTP_RSP_ERR550)  /*** file doesn't exist or not plain file  == 550 ***/
    {
      if (!cfg.always_mdtm && docp->doc_url->extension
          && ((ftp_url_extension *)docp->doc_url->extension)->time != 0)
      {
        docp->dtime = ((ftp_url_extension *)docp->doc_url->extension)->time;
        reply = FTP_RSP_ERR213;
        DEBUG_PROTOC("taking stored MDTM %d\n",
                     (int)((ftp_url_extension *)docp->doc_url->extension)->time);
      }
      else
      {
        snprintf(buf, sizeof(buf), "MDTM %s\r\n", filename + 1);
        buf[sizeof(buf) - 1] = 0;
        ftp_control_write(docp, buf, strlen(buf));
        DEBUG_PROTOC("%s", buf);

        mbuf = NULL;
        if ((reply = ftp_get_response(docp, &mbuf, TRUE)) == FTP_RSP_ERR213)
        {
          if (2 > sscanf(mbuf, "%d %2147s", &reply, buf))
          {
            xprintf(1,
                    gettext("Unexpected FTP server '213 - File status' response '%.*s'%s:"
                            " no recognizable reply code and/or date/time format.\n"),
                    (strlen(mbuf) > 80 ? 80 : strlen(mbuf)),
                    mbuf,
                    (strlen(mbuf) > 80 ? "(Truncated)" : ""));
            docp->dtime = 0;
          }
          else if (strlen(buf) == 14)
          {
            docp->dtime = time_ftp_scn(buf, FALSE);
            if (docp->dtime == (time_t)-1)
            {
              xprintf(1, gettext("Cannot decode FTP server '213 - File status' response '%s'\n"),
                      mbuf);
              docp->dtime = time_ftp_scn(buf, TRUE);
            }
          }
        }
        else
        {
          if (reply == 600)
          {
            docp->ftp_fatal_err = TRUE;
            docp->errcode = ERR_READ;
            _free(mbuf);
            return NULL;
          }
        }
        _free(mbuf);
      }

      DEBUG_PROTOC("Size: file %d, remote %d; time: file %d, remote %d\n",
                   docp->origsize,
                   docp->totsz,
                   docp->origtime,
                   docp->dtime);

      if (docp->origsize && (docp->totsz >= 0) && (docp->origsize != docp->totsz))
      {
        xprintf(1, gettext("Size differs, regeting whole\n"));
        docp->rest_pos = 0;
      }
      else if (docp->origtime && docp->dtime)
      {
        if ((difftime(docp->dtime, docp->origtime) > 0))
        {
          if (docp->rest_pos)
          {
            docp->rest_pos = 0;
            xprintf(1, gettext("Modified from last download - regeting whole\n"));
          }
        }
        else if ((cfg.mode == MODE_SYNC || cfg.mode == MODE_MIRROR) && !docp->rest_pos)
        {
          docp->errcode = ERR_FTP_ACTUAL;
          return NULL;
        }
      }

      if (docp->check_limits)
      {
        cond_info_t condp;
        int r;

        condp.level = 3;
        condp.urlnr = docp->doc_nr;
        condp.size = docp->totsz;
        condp.time = docp->dtime;
        condp.mimet = NULL;
        condp.full_tag = NULL;
        condp.params = NULL;
        condp.html_doc = NULL;
        condp.html_doc_offset = 0;
        condp.tag = NULL;
        condp.attrib = NULL;

        r = url_append_condition(docp->doc_url, &condp);

        if (!r)
        {
          switch (condp.reason)
          {
          case CONDT_MAX_SIZE:
            docp->errcode = ERR_BIGGER;
            break;

          case CONDT_MIN_SIZE:
            docp->errcode = ERR_SMALLER;
            break;

          case CONDT_NEWER_THAN:
          case CONDT_OLDER_THAN:
            docp->errcode = ERR_OUTTIME;
            break;

          case CONDT_USER_CONDITION:
            docp->errcode = ERR_SCRIPT_DISABLED;
            break;

          default:
            docp->errcode = ERR_RDISABLED;
            break;
          }
          return NULL;
        }
      }

      if (ftp_set_transfer_type(docp, "I"))
      {
        docp->ftp_fatal_err = TRUE;
        docp->errcode = ERR_FTP_DATACON;
        return NULL;
      }

      if (ftp_open_data_init(docp))
      {
        bufio_close(docp->ftp_control);
        docp->ftp_control = NULL;
        docp->errcode = ERR_FTP_DATACON;
        return NULL;
      }

      if (docp->rest_pos)
      {
        char *rsp;

        sprintf(buf, "REST %ld\r\n", (long)docp->rest_pos);
        ftp_control_write(docp, buf, strlen(buf));
        DEBUG_PROTOC("%s", buf);
        if ((reply = ftp_get_response(docp, &rsp, TRUE)) >= 400)
        {
          _free(rsp);
          if (reply == 600)
          {
            docp->ftp_fatal_err = TRUE;
            bufio_close(docp->datasock);
            docp->datasock = NULL;
            docp->errcode = ERR_READ;
            return NULL;
          }
          docp->errcode = ERR_FTP_NOREGET;
        }
        else
        {
          sprintf(buf, "%ld", (long)docp->rest_pos);
          if (!strstr(rsp + 3, buf))
          {
            xprintf(1, gettext("Warning: FTP server understands REST command, but can't handle it properly.\n"));
            docp->errcode = ERR_FTP_NOREGET;
          }
          _free(rsp);
        }
      }

      snprintf(buf, sizeof(buf), "RETR %s\r\n", filename + 1);
      buf[sizeof(buf) - 1] = 0;
      ftp_control_write(docp, buf, strlen(buf));
      DEBUG_PROTOC("%s", buf);

      /** check control connection if there is available  **/
      /** server response. This is required to properly   **/
      /** establish support active data connections, also **/
      /** in case of error                                **/
      if (!cfg.ftp_activec || tl_selectr(bufio_getfd(docp->ftp_control), 10) == 0)
      {
        if (!ftp_open_data_finish(docp))
        {
          docp->ftp_fatal_err = TRUE;
          if (!docp->errcode)
            docp->errcode = ERR_FTP_DATACON;
          return NULL;
        }
      }

      if (!((reply = ftp_get_response(docp, NULL, TRUE)) >= 400))
      {
        docp->errcode = ERR_NOERROR;

        if (!ftp_open_data_finish(docp))
        {
          docp->ftp_fatal_err = TRUE;
          if (!docp->errcode)
            docp->errcode = ERR_FTP_DATACON;
          return NULL;
        }

        return docp->datasock;
      }

      if (reply == 600)
      {
        docp->ftp_fatal_err = TRUE;
        bufio_close(docp->datasock);
        docp->datasock = NULL;
        docp->errcode = ERR_READ;
        return NULL;
      }
      docp->errcode = ERR_FTP_GET;
    }
    else
    {
      docp->errcode = ERR_FTP_GET;
    }
  }

  if (((docp->doc_url->extension
        && ((ftp_url_extension *)docp->doc_url->extension)->type == FTP_TYPE_D)
       || !docp->doc_url->extension)
      && ((docp->check_limits && cfg.condition.ftpdir) || !docp->check_limits))
  {
    int try_the_other_list_cmd;

    /*
     * The ftp_send_list_command() call will send an NLST or LIST command
     * to the FTP server.
     *
     * Some servers don't like NLST, so this code has been fitted with an
     * 'autodetect' mechanism by retrying the 'ls/dir' command by sending
     * the OTHER one of the NLST/LIST commands when the first try got us
     * a 550 response.
     */
    for (try_the_other_list_cmd = 0; try_the_other_list_cmd < 2; try_the_other_list_cmd++)
    {
      if (ftp_check_directory_existence(docp, filename, cfg.ftp_list ^ try_the_other_list_cmd))
      {
        docp->errcode = ERR_FTP_NODIR;
        docp->ftp_respc = FTP_RSP_ERR550;
        if (docp->datasock)
        {
          bufio_close(docp->datasock);
          docp->datasock = NULL;
        }
        return NULL;
      }

      if (ftp_set_transfer_type(docp, "A"))
      {
        return NULL;
      }

      if (!docp->datasock)
      {
        if (ftp_open_data_init(docp))
        {
          bufio_close(docp->ftp_control);
          docp->ftp_control = NULL;
          docp->errcode = ERR_FTP_DATACON;
          return NULL;
        }
      }

      ftp_send_list_command(docp, filename, cfg.ftp_list ^ try_the_other_list_cmd);

      /** check control connection if there is available  **/
      /** server response. This is required to properly   **/
      /** establish support active data connections, also **/
      /** in case of error                                **/
      if (!cfg.ftp_activec || tl_selectr(bufio_getfd(docp->ftp_control), 10) == 0)
      {
        if (!ftp_open_data_finish(docp))
        {
          docp->ftp_fatal_err = TRUE;
          if (!docp->errcode)
            docp->errcode = ERR_FTP_DATACON;
          return NULL;
        }
      }

      if (!((reply = ftp_get_response(docp, NULL, TRUE)) >= 400))
      {
        docp->errcode = ERR_NOERROR;
        *is_dir = TRUE;

        if (!ftp_open_data_finish(docp))
        {
          docp->ftp_fatal_err = TRUE;
          if (!docp->errcode)
            docp->errcode = ERR_FTP_DATACON;
          return NULL;
        }

        if (try_the_other_list_cmd)
        {
          xprintf(0,
            "WARNING: this FTP site (%s) does not seem to react well to your '-%sFTPlist' setting. "
            "Please try again with the '-%sFTPlist' command line option for possibly better results.\n",
            (docp->doc_url ? docp->doc_url->p.ftp.host : "???"),
            (cfg.ftp_list ? "" : "no"),
            (!cfg.ftp_list ? "" : "no"));

          /*
           * Of course, we COULD just toggle 'cfg.ftp_list' here, but that has two drawbacks:
           *
           * - the user selected a certain setting and will be confused when we change it halfway
           *   down the lane - you'll never know if this retry/autodetect code kicked in at the
           *   initial URL or only later on!
           *
           *       Worse even: imagine a valid situation where the server acted properly by sending
           *       back a 550 response code, while we were already using the 'proper' ftp_list setting
           *       for that given site, than this code would auto-modify that ftp_list setting right
           *       back to the WRONG value if the server would repond improperly to the other ls/dir
           *       command!
           *
           * - editing cfg.xxxxx variables in here implies a thread unsafety ==> for that to work
           *   *properly* we'll have to LOCK_XXX/UNLOCK_XXX guard *all* cfg.ftp_list accesses
           *   *anywhere* in the code. And that's just too much hassle for something, that's
           *   more reasonably controlled by the user him/herself anyway.
           */
        }
        return docp->datasock;
      }

      /* make sure we close the data socket, so we have to create a fresh one for the 'retry' */
      if (docp->datasock)
      {
        bufio_close(docp->datasock);
        docp->datasock = NULL;
      }

      if (reply != FTP_RSP_ERR550)
        break;
    }
    /*
     * when we get here, the FTP server returned an error response
     * for both NLST and LIST attempts
     */
    docp->ftp_fatal_err = (reply == 600);
    docp->errcode = *is_dir ? ERR_FTP_NODIR : ERR_FTP_GET;
  }
  else if (!cfg.condition.ftpdir && reply == FTP_RSP_ERR550 && *is_dir)
  {
    docp->errcode = ERR_FTP_DIRNO;
  }

  if (docp->datasock)
  {
    bufio_close(docp->datasock);
    docp->datasock = NULL;
  }

  return NULL;
}


/********************************************************/
/* pre dane FTP URL vracia deskriptor datoveho spojenia */
/* FIXME: Translate me!                                 */
/********************************************************/
bufio *ftp_get_data_socket(doc *docp)
{
  char *user, *ruser;
  char *password;
  char pom[100];
  bufio *s;
  bool_t sdir;
  char *tmp_path;

  user = url_get_user(docp->doc_url, NULL);
  password = url_get_pass(docp->doc_url, NULL);

  if (!user)
    user = "anonymous";
  ruser = user;

  if (!password)
    password = cfg.send_from ? priv_cfg.from : "pavuk@domain.xx";

  if (priv_cfg.ftp_proxy && !cfg.ftp_dirtyp)
  {
    snprintf(pom,
             sizeof(pom),
             "%s@%s %hu",
             user,
             docp->doc_url->p.ftp.host,
             docp->doc_url->p.ftp.port);
    pom[sizeof(pom) - 1] = 0;
    user = pom;
  }

  sdir = docp->doc_url->p.ftp.dir;
  tmp_path = url_decode_str(docp->doc_url->p.ftp.path, strlen(docp->doc_url->p.ftp.path));

  s = ftp_connect(docp,
                  priv_cfg.ftp_proxy && !cfg.ftp_dirtyp ?
                  priv_cfg.ftp_proxy : docp->doc_url->p.ftp.host,
                  priv_cfg.ftp_proxy && !cfg.ftp_dirtyp ?
                  priv_cfg.ftp_proxy_port : docp->doc_url->p.ftp.port,
                  ruser,
                  user,
                  password,
                  tmp_path,
                  &sdir);

  _free(tmp_path);

  if (sdir != docp->doc_url->p.ftp.dir)
  {
    docp->doc_url->p.ftp.dir = sdir;
    url_changed_filename(docp->doc_url);
  }

  return s;
}


/*******************************/
/* remove document from server */
/*******************************/
int ftp_remove(doc *docp)
{
  char *cmd;
  char *filename;
  int reply;

  if (docp->doc_url->type != URLT_FTP && docp->doc_url->type != URLT_FTPS)
    return 0;

  if (docp->doc_url->p.ftp.dir)
    return 0;

  xprintf(1, "Deleting remote document ...\n");

  filename = url_decode_str(docp->doc_url->p.ftp.path, strlen(docp->doc_url->p.ftp.path));

  cmd = tl_str_concat(DBGvars(NULL), "DELE ", filename + 1, "\r\n", NULL);
  ftp_control_write(docp, cmd, strlen(cmd));
  DEBUG_PROTOC("%s", cmd);

  _free(filename);
  _free(cmd);

  if ((reply = ftp_get_response(docp, NULL, TRUE)) >= 400)
  {
    if (reply == 600)
    {
      docp->ftp_fatal_err = TRUE;
      return -1;
    }
  }

  return 0;
}


/********************************************************/
/* z FTP adresara urobi HTML dokument                   */
/* FIXME: Translate me!                                 */
/********************************************************/
static void ftp_nlst_dir_to_html(doc *docp)
{
  char *p, *n, *res = NULL;
  char pom[8192];
  int tsize;
  bool_t last = 1;
  int ilen;

  snprintf(pom, sizeof(pom),
           "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
           "<HTML>\n<TITLE>\nDirectory of %s://%s:%hu%s\n</TITLE>\n<BODY>\n"
           "<H1 ALIGN=CENTER><B>List of FTP directory %s://%s:%hu/%s </H1><BR><UL>",
           prottable[docp->doc_url->type].urlid, docp->doc_url->p.ftp.host,
           docp->doc_url->p.ftp.port, docp->doc_url->p.ftp.path,
           prottable[docp->doc_url->type].urlid, docp->doc_url->p.ftp.host,
           docp->doc_url->p.ftp.port, docp->doc_url->p.ftp.path);
  pom[sizeof(pom) - 1] = 0;

  res = tl_strdup(pom);
  tsize = strlen(pom);

  p = docp->contents;

  while (*p)
  {
    ilen = strcspn(p, "\r\n");
    if (*(p + ilen))
      *(p + ilen) = '\0';
    else
      last = 0;

    _strtrchr(p, '\\', '/');
    n = strrchr(p, '/');
    if (n)
      n++;
    else
      n = p;

    n = url_encode_str(n, URL_PATH_UNSAFE);

    if (docp->doc_url->p.ftp.user && docp->doc_url->p.ftp.password)
    {
      snprintf(pom, sizeof(pom),
               "<LI><A HREF=\"%s://%s:%s@%s:%hu%s%s%s\">&quot;%s&quot;</A>\n",
               prottable[docp->doc_url->type].urlid,
               docp->doc_url->p.ftp.user, docp->doc_url->p.ftp.password,
               docp->doc_url->p.ftp.host, docp->doc_url->p.ftp.port,
               docp->doc_url->p.ftp.path,
               (tl_is_dirname(docp->doc_url->p.ftp.path) ? "" : "/"), n, n);
      pom[sizeof(pom) - 1] = 0;
    }
    else if (docp->doc_url->p.ftp.user)
    {
      snprintf(pom, sizeof(pom),
               "<LI><A HREF=\"%s://%s@%s:%hu%s%s%s\">&quot;%s&quot;</A>\n",
               prottable[docp->doc_url->type].urlid,
               docp->doc_url->p.ftp.user,
               docp->doc_url->p.ftp.host, docp->doc_url->p.ftp.port,
               docp->doc_url->p.ftp.path,
               (tl_is_dirname(docp->doc_url->p.ftp.path) ? "" : "/"), n, n);
      pom[sizeof(pom) - 1] = 0;
    }
    else
    {
      snprintf(pom, sizeof(pom),
               "<LI><A HREF=\"%s://%s:%hu%s%s%s\">&quot;%s&quot;</A>\n",
               prottable[docp->doc_url->type].urlid,
               docp->doc_url->p.ftp.host, docp->doc_url->p.ftp.port,
               docp->doc_url->p.ftp.path,
               (tl_is_dirname(docp->doc_url->p.ftp.path) ? "" : "/"), n, n);
      pom[sizeof(pom) - 1] = 0;
    }
    _free(n);

    tsize += strlen(pom);
    res = _realloc(res, tsize + 1);

    strcat(res, pom);
    p += ilen + last;
    p += strspn(p, "\r\n");
  }

  tsize += 22;
  res = _realloc(res, tsize + 1);

  strcat(res, "</UL>\n</BODY>\n</HTML>\n");

  _free(docp->contents);

  docp->contents = res;
  docp->size = tsize;
}


static int ftp_list_unix_perm_parse(char *str)
{
  int perm = 0;
  int i;
  char *s;

  if (strlen(str) != 10)
    return -1;

  for (s = str + 1, i = 0; i < 3; i++, s += 3)
  {
    perm <<= 3;
    if (s[0] == 'r')
      perm += 4;
    if (s[1] == 'w')
      perm += 2;
    if (s[2] == 'x')
      perm += 1;
  }
  return perm;
}


static int ftp_list_unix_perm_type(char *str)
{
  switch (str[0])
  {
  case 'd':
    return FTP_TYPE_D;

  case 'l':
    return FTP_TYPE_L;

  default:
    return FTP_TYPE_F;
  }
}


static void ftp_list_unix_get_slink(char *str, char **nm, char **ln)
{
  char *p;

  *ln = NULL;
  *nm = str;

  if ((p = strstr(str, " -> ")))
  {
    *p = '\0';
    *ln = p + 4;
  }
}


static int ftp_list_get_month_index(char *month)
{
  static const char *months[] =
  {
    "jan", "feb", "mar", "apr", "may", "jun",
    "jul", "aug", "sep", "oct", "nov", "dec",
    NULL
  };
  int i;

  if (strlen(month) != 3)
    return -1;

  /* make lowercase month */
  for (i = 0; i < 3; ++i)
  {
    month[i] = tolower(month[i]);
  }

  i = 0;
  while (months[i] != NULL)
  {
    if (strcmp(month, months[i]) == 0)
      return i + 1;

    i++;
  }

  return 0;
}


static time_t ftp_list_get_date(const char *date)
{
  int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
  char m[PATH_MAX];
  int ok = 0;

  static int our_year = 0;
  static int our_month = 0;

  char result[6 * 24 + 1];
  time_t t;

  if (our_year == 0)
  {
    time_t t;
    struct tm *ptm;

    t = time(NULL);
    LOCK_TIME;
    ptm = gmtime(&t);
    UNLOCK_TIME;

    our_year = ptm->tm_year;
    our_month = ptm->tm_mon;
  }

  if (sscanf(date, "%3[a-zA-Z] %2d %02d:%02d", m, &day, &hour, &minute) == 4)
  {
    month = ftp_list_get_month_index(m);
    if (month <= 0)
      return 0;

    if (day < 1 || day > 31)
      return 0;

    if (hour < 0 || hour > 24)
      return 0;

    if (minute < 0 || minute > 60)
      return 0;

    year = 0;
    second = 0;

    ok = 1;
  }

  if (!ok && sscanf(date, "%3[a-zA-Z] %2d %04d", m, &day, &year) == 3)
  {
    month = ftp_list_get_month_index(m);

    if (month <= 0)
      return 0;

    if (day < 1 || day > 31)
      return 0;

    if (year < 1995 || year > 2100)
      return 0;

    hour = 0;
    second = 0;
    minute = 0;

    ok = 1;
  }

  if (!ok)
    return 0;

  if (year == 0)
  {
    year = our_year;
    if ((month - 1) > our_month)
      year--;
  }

  if (year < 1900)
    year += 1900;

  sprintf(result, "%04d%02d%02d%02d%02d%02d", year, month, day, hour, minute, second);

  t = time_ftp_scn(result, FALSE);
#if 0
  xprintf(0, "Time conversion: %s == %s (%d)\n", date, result, (int)t);
#endif
  if (t == (time_t)-1)
  {
    xprintf(1, gettext("Cannot decode FTP list date '%s' (decoded output: '%s')\n"), date, result);
    t = time_ftp_scn(result, TRUE);
  }

  return t;
}


static ftp_url_extension *ftp_list_unix_parse_line(char *str, char **fname, ftp_url_extension *ret)
{
  char pom[2000];
  char name[2000];
  char attrib[2000];
  char month[2000];
  char rest[2000];
  unsigned int pi, day;
  unsigned long size;
  char *nm, *sln;

  *fname = NULL;

  if (str[0] == 'b'              /* block device */
      || str[0] == 'u'           /* character device unbuffered */
      || str[0] == 'c'           /* character device */
      || str[0] == 'p'           /* FIFO */
      || str[0] == 's')          /* socket */
  {
    /* we don't support devices, FIFOs and sockets creation */
    return NULL;
  }
  /* for SYS V style ls -l */
  else if (sscanf(str,
                  "%1999s %u %1999s %1999s %lu %1999s %u %1999s %1999[^\n\r]", attrib, &pi, pom,
                  pom, &size, month, &day, rest, name) != 9)
  {
    /* for old BSD style ls -l */
    if (sscanf(str, "%1999s %u %1999s %lu %1999s %u %1999s %1999[^\n\r]", attrib, &pi, pom, &size,
               month, &day, rest, name) != 8)
    {
      return NULL;
    }
  }
  ret->size = size;

  ret->type = ftp_list_unix_perm_type(attrib);
  ret->perm = ftp_list_unix_perm_parse(attrib);
  if (ret->perm == -1)
    return NULL;

  ftp_list_unix_get_slink(name, &nm, &sln);

  ret->slink = tl_strdup(sln);
  snprintf(pom, sizeof(pom), "%s %d %s", month, day, rest);
  pom[sizeof(pom) - 1] = 0;
  ret->time = ftp_list_get_date(pom);
  *fname = tl_strdup(nm);

  return ret;
}


static ftp_url_extension *ftp_list_epfl_parse_line(char *str, char **fname, ftp_url_extension *ret)
{
  char *p, *strtokbuf;

  *fname = NULL;

  if (*str != '+')
  {
    return NULL;
  }

  p = strchr(str, '\x09');
  if (!p)
  {
    return NULL;
  }
  *fname = tl_strdup(p + 1);
  *p = '\0';

  ret->type = FTP_TYPE_F;
  ret->size = 0;

  p = strtokc_r(str + 1, ',', &strtokbuf);

  while (p)
  {
    switch (*p)
    {
    case 'r':
      ret->type = FTP_TYPE_F;
      break;

    case '/':
      ret->type = FTP_TYPE_D;
      break;

    case 's':
      ret->size = _atoi(p + 1);
      break;
    }
    p = strtokc_r(NULL, ',', &strtokbuf);
  }
  ret->slink = NULL;
  ret->time = 0;
  ret->perm = (4 << 6) + (2 << 6) + (2 << 3) + 2;


  return ret;
}


static ftp_url_extension *ftp_list_novel_parse_line(char *str, char **fname, ftp_url_extension *ret)
{
  char pom[2000];
  char name[4000];
  char attrib[2000];
  unsigned int pi;
  unsigned long size;

  *fname = NULL;

  if (sscanf(str, "%1999s %u %1999s %lu %1999s %u %1999s %3999[^\n\r]", attrib, &pi, pom, &size,
             pom, &pi, pom, name) != 8)
  {
    return NULL;
  }
  ret->size = size;

  ret->type = *attrib == 'd' ? FTP_TYPE_D : FTP_TYPE_F;
  ret->slink = NULL;
  ret->time = 0;
  ret->perm = (4 << 6) + (2 << 6) + (2 << 3) + 2;

  *fname = tl_strdup(name);

  return ret;
}


static ftp_url_extension *ftp_list_vms_parse_line(char *str, char **fname, ftp_url_extension *ret)
{
  int l;
  char *p = str;

  *fname = NULL;

  l = strcspn(p, " ");

  ret->type = strncmp(p + l - 6, ".DIR;", 5) ? FTP_TYPE_F : FTP_TYPE_D;

  if (ret->type == FTP_TYPE_D)
    *fname = tl_strndup(p, l - 6);
  else
    *fname = tl_strndup(p, l - 2);
  lowerstr(*fname);

  ret->slink = NULL;
  ret->time = 0;
  ret->size = 0;
  ret->perm = (4 << 6) + (2 << 6) + (2 << 3) + 2;

  return ret;
}


static ftp_url_extension *ftp_list_windos_parse_line(char *str, char **fname, ftp_url_extension *ret)
{
  char pom[2000];
  char pom2[2000];
  int ti;

  *fname = NULL;

  if (sscanf(str, "%2d-%2d-%2d  %2d:%2d%*[AP]M %1999s %1999[^\n\r]", &ti, &ti, &ti, &ti, &ti, pom2,
             pom) != 7)
    return NULL;

  ret->slink = NULL;
  ret->time = 0;
  ret->perm = (4 << 6) + (2 << 6) + (2 << 3) + 2;

  if (strstr(pom2, "<DIR>"))
  {
    ret->type = FTP_TYPE_D;
    ret->size = 0;
  }
  else
  {
    ret->type = FTP_TYPE_F;
    ret->size = atoi(pom2);
  }

  *fname = tl_strdup(pom);

  return ret;
}


/********************************************************/
/* z vypisu FTP adresara urobi HTML dokument            */
/* FIXME: Translate me!                                 */
/********************************************************/
static void ftp_list_unix_dir_to_html(doc *docp)
{
  char *p, *res = NULL;
  char pom[8192];
  char urlstr[4096];
  int tsize;
  ftp_url_extension *ext = NULL;
  ftp_url_extension exta;
  char *name = NULL;
  char *tmp;
  int ilen;
  bool_t last = 1;

  enum
  {
    FTP_DT_UNIXL,
    FTP_DT_VMS,
    FTP_DT_WINDOWS
  } type;

  snprintf(pom, sizeof(pom),
           "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
           "<HTML>\n<TITLE>\nDirectory of %s://%s:%hu%s\n</TITLE>\n<BODY>\n"
           "<H1 ALIGN=CENTER><B>List of FTP directory %s://%s:%hu/%s </H1><BR><UL>",
           prottable[docp->doc_url->type].urlid,
           docp->doc_url->p.ftp.host, docp->doc_url->p.ftp.port,
           docp->doc_url->p.ftp.path, prottable[docp->doc_url->type].urlid,
           docp->doc_url->p.ftp.host, docp->doc_url->p.ftp.port, docp->doc_url->p.ftp.path);
  pom[sizeof(pom) - 1] = 0;

  res = tl_strdup(pom);
  tsize = strlen(pom);

  /*** skip first total line ***/
  if (!strncmp(docp->contents, "total ", 6))
  {
    type = FTP_DT_UNIXL;
    p = docp->contents + strcspn(docp->contents, "\r\n");
    p += strspn(p, "\r\n");
  }
  else if (!strncmp(docp->contents, "Directory ", 10)
           || !strncmp(docp->contents + strspn(docp->contents, " \r\n"), "Directory ", 10))
  {
    type = FTP_DT_VMS;
    p = docp->contents + strspn(docp->contents, " \r\n");
    p += strcspn(p, "\r\n");
    p += strspn(p, "\r\n");
  }
  else
  {
    int ti;

    if (sscanf(docp->contents, "%2d-%2d-%2d  %2d:%2d%*[AP]M %8191s %8191[^\n\r]", &ti, &ti, &ti,
               &ti, &ti, pom, pom) == 7)
    {
      type = FTP_DT_WINDOWS;
      p = docp->contents;
    }
    else
    {
      type = FTP_DT_UNIXL;
      p = docp->contents;
    }
  }

  while (*p)
  {
    ilen = strcspn(p, "\r\n");
    if (*(p + ilen))
      *(p + ilen) = '\0';
    else
      last = 0;

    if (type == FTP_DT_UNIXL)
    {
      if (*p == '+')
        ext = ftp_list_epfl_parse_line(p, &name, &exta);
      else if (*(p + 1) == '[')
        ext = ftp_list_novel_parse_line(p, &name, &exta);
      else
        ext = ftp_list_unix_parse_line(p, &name, &exta);
    }
    else if (type == FTP_DT_VMS)
    {
      if (strncmp(p, "Total of ", 9) && *p && *p != ' ')
        ext = ftp_list_vms_parse_line(p, &name, &exta);
    }
    else if (type == FTP_DT_WINDOWS)
    {
      ext = ftp_list_windos_parse_line(p, &name, &exta);
    }

    if (!ext)
    {
      xprintf(1, gettext("ERROR: unable to parse FTP list line :\n\t%s\n"), p);
      p += ilen + last;
      p += strspn(p, "\r\n");
      continue;
    }

    if (!name || !strcmp(name, ".") || !strcmp(name, ".."))
    {
      p += ilen + last;
      p += strspn(p, "\r\n");
      continue;
    }

    tmp = name;
    name = url_encode_str(name, URL_PATH_UNSAFE);
    _free(tmp);

    if (docp->doc_url->p.ftp.user && docp->doc_url->p.ftp.password)
    {
      snprintf(urlstr, sizeof(urlstr), "%s://%s:%s@%s:%hu%s%s%s%s",
               prottable[docp->doc_url->type].urlid,
               docp->doc_url->p.ftp.user,
               docp->doc_url->p.ftp.password,
               docp->doc_url->p.ftp.host,
               docp->doc_url->p.ftp.port,
               docp->doc_url->p.ftp.path,
               (tl_is_dirname(docp->doc_url->p.ftp.path) ? "" : "/"),
               name, (ext->type == FTP_TYPE_D) ? "/" : "");
      urlstr[sizeof(urlstr) - 1] = 0;
    }
    else if (docp->doc_url->p.ftp.user)
    {
      snprintf(urlstr, sizeof(urlstr), "%s://%s@%s:%hu%s%s%s%s",
               prottable[docp->doc_url->type].urlid,
               docp->doc_url->p.ftp.user,
               docp->doc_url->p.ftp.host,
               docp->doc_url->p.ftp.port,
               docp->doc_url->p.ftp.path,
               (tl_is_dirname(docp->doc_url->p.ftp.path) ? "" : "/"),
               name, (ext->type == FTP_TYPE_D) ? "/" : "");
      urlstr[sizeof(urlstr) - 1] = 0;
    }
    else
    {
      snprintf(urlstr, sizeof(urlstr), "%s://%s:%hu%s%s%s%s",
               prottable[docp->doc_url->type].urlid,
               docp->doc_url->p.ftp.host,
               docp->doc_url->p.ftp.port,
               docp->doc_url->p.ftp.path,
               (tl_is_dirname(docp->doc_url->p.ftp.path) ? "" : "/"),
               name, (ext->type == FTP_TYPE_D) ? "/" : "");
      urlstr[sizeof(urlstr) - 1] = 0;
    }
    snprintf(pom,
             sizeof(pom),
             "<LI><A PAVUKEXT=\"FTPINF %u %hu %lu %u %s\" HREF=\"%s\">&quot;%s&quot;</A> (%lub)\n",
             (unsigned int)ext->type,
             ext->perm,
             (long)ext->size,
             (unsigned int)ext->time,
             (ext->slink ? ext->slink : ""),
             urlstr,
             name,
             (long)ext->size);
    pom[sizeof(pom) - 1] = 0;

    _free(name);
    _free(ext->slink);

    tsize += strlen(pom);
    res = _realloc(res, tsize + 1);
    strcat(res, pom);

    p += ilen + last;
    p += strspn(p, "\r\n");
  }

  tsize += 22;
  res = _realloc(res, tsize + 1);

  strcat(res, "</UL>\n</BODY>\n</HTML>\n");

  _free(docp->contents);

  docp->contents = res;
  docp->size = tsize;
}


void ftp_dir_to_html(doc *docp)
{
  if (!docp->contents)
    return;

  if (!docp->ftp_dir_type)
  {
    ftp_nlst_dir_to_html(docp);
  }
  else
  {
    ftp_list_unix_dir_to_html(docp);
  }
}


void ftp_url_ext_free(ftp_url_extension *fext)
{
  _free(fext->slink);
  _free(fext);
}


ftp_url_extension *ftp_url_ext_new(int type, int perm, ssize_t size, char *slink, time_t time)
{
  ftp_url_extension *rv;

  rv = _malloc(sizeof(*rv));

  rv->perm = perm;
  rv->size = size;
  rv->type = type;
  rv->slink = slink;
  rv->time = time;

  return rv;
}


ftp_url_extension *ftp_url_ext_dup(ftp_url_extension *ext)
{
  return ftp_url_ext_new(ext->type, ext->perm, ext->size, tl_strdup(ext->slink), ext->time);
}


ftp_url_extension *ftp_parse_ftpinf_ext(char *str)
{
  char pom[8192];
  unsigned int t, p, s, tt;
  int num;

  if (strncmp(str, "FTPINF", 6))
    return NULL;

  num = sscanf(str, "FTPINF %u %u %u %u %8191[^\n\r]", &t, &p, &s, &tt, pom);

  if (num < 4)
    return NULL;

  return ftp_url_ext_new(t, p, s, (num == 5) ? tl_strdup(pom) : NULL, tt);
}


int ftp_make_symlink(url *urlp)        /* FIXME: Security */
{
  char pom[PATH_MAX];
  char *frompath, *topath;
  char *p;
  ftp_url_extension *ext = (ftp_url_extension *)urlp->extension;
  int rv;

  frompath = tl_strdup(url_to_filename(urlp, TRUE));

  if (!cfg.preserve_links)
  {
    if (ext->slink[0] == '/')
    {
      p = urlp->p.ftp.path;
      urlp->p.ftp.path = ext->slink;
      topath = tl_strdup(url_get_local_name_real(urlp, NULL, FALSE));
      urlp->p.ftp.path = p;
    }
    else
    {
      strcpy(pom, urlp->p.ftp.path);
      p = strrchr(pom, '/');
      if (p)
        *(p + 1) = '\0';
      strcat(pom, ext->slink);

      p = urlp->p.ftp.path;
      urlp->p.ftp.path = pom;
      topath = tl_strdup(url_get_local_name_real(urlp, NULL, FALSE));
      urlp->p.ftp.path = p;
    }
    p = topath;
    topath = get_relative_path(frompath, p);
    _free(p);
  }
  else
    topath = NULL;

  /* don't make links to directory index files */
  /* but rather to directories                 */
  if (topath && tl_is_dirname(ext->slink))
  {
    p = tl_get_basename(topath);
    if (p > topath)
      *(p) = '\0';
  }

  xprintf(1, gettext("Making symlink \"%s\" to \"%s\"\n"), frompath, topath ? topath : ext->slink);

  if (makealldirs(frompath))
    xperror1("Cannot create directory chain for file", mk_native(frompath));

  if (!tl_access(frompath, F_OK))
  {
    if (tl_unlink(frompath))
      xperror1("Cannot unlink/delete file", mk_native(frompath));
  }

  rv = tl_symlink(topath ? topath : ext->slink, frompath);

  if (rv == -1)
  {
    xperror2("ftp_make_symlink() topath failure",
             mk_native(topath ? topath : ext->slink), mk_native(frompath));
  }

  _free(topath);
  _free(frompath);

  return rv;
}


void ftp_handshake_info_free(ftp_handshake_info *fhi)
{
  _free(fhi->host);

  for ( ; fhi->infos; fhi->infos = dllist_remove_entry(fhi->infos, fhi->infos))
  {
    ftp_handshake_info_data *d = (ftp_handshake_info_data *)fhi->infos->data;
    _free(d->cmd);
    _free(d);
  }
}


ftp_handshake_info *ftp_handshake_info_parse(char *host, char *str)
{
  ftp_handshake_info *rv = _malloc(sizeof(*rv));
  ftp_handshake_info_data *d;
  char *p;
  char **array;
  int i, ok;

  rv->infos = NULL;
  p = strchr(host, ':');
  if (p)
  {
    rv->host = tl_strndup(host, p - host);
    rv->port = _atoi(p + 1);
    if (!rv->port)
      rv->port = DEFAULT_FTP_PORT;
  }
  else
  {
    rv->host = tl_strdup(host);
    rv->port = DEFAULT_FTP_PORT;
  }

  array = tl_str_split(str, "\\");

  d = NULL;
  ok = TRUE;
  if (array)
  {
    for (i = 0; array[i]; i++)
    {
      if (!ok)
      {
        _free(array[i]);
        continue;
      }

      if (!d)
      {
        d = _malloc(sizeof(*d));
        d->cmd = array[i];
        d->response = 0;
        array[i] = NULL;
      }
      else
      {
        d->response = _atoi(array[i]);
        if (!d->response && errno == ERANGE)
          ok = FALSE;
        _free(array[i]);
        rv->infos = dllist_append(rv->infos, (dllist_t)d);
        d = NULL;
      }
    }
    if (d)
    {
      _free(d->cmd);
      _free(d);
      ok = FALSE;
    }

    _free(array);
  }
  else
  {
    ok = FALSE;
  }

  if (!ok)
  {
    ftp_handshake_info_free(rv);
    rv = NULL;
  }

  return rv;
}


ftp_handshake_info *ftp_handshake_info_dup(ftp_handshake_info *fhi)
{
  dllist *ptr;
  ftp_handshake_info *rv = _malloc(sizeof(*rv));

  rv->host = tl_strdup(fhi->host);
  rv->port = fhi->port;
  rv->infos = NULL;

  for (ptr = fhi->infos; ptr; ptr = ptr->next)
  {
    ftp_handshake_info_data *d1, *d2;

    d1 = (ftp_handshake_info_data *)ptr->data;

    d2 = _malloc(sizeof(*d2));
    d2->response = d1->response;
    d2->cmd = tl_strdup(d1->cmd);

    rv->infos = dllist_append(rv->infos, (dllist_t)d2);
  }

  return rv;
}


char *ftp_handshake_info_data_dump(ftp_handshake_info *fhi)
{
  ftp_handshake_info_data *d;
  dllist *ptr;
  char pom[512];
  char *rv = NULL;

  if (fhi->infos)
  {
    d = (ftp_handshake_info_data *)fhi->infos->data;
    snprintf(pom, sizeof(pom), "%s\\%d", d->cmd, d->response);
    pom[sizeof(pom) - 1] = 0;
    rv = tl_strdup(pom);

    for (ptr = fhi->infos->next; ptr; ptr = ptr->next)
    {
      d = (ftp_handshake_info_data *)ptr->data;
      snprintf(pom, sizeof(pom), "\\%s\\%d", d->cmd, d->response);
      pom[sizeof(pom) - 1] = 0;
      rv = tl_str_append(rv, pom);
    }
  }
  return rv;
}


static ftp_handshake_info *ftp_handshake_info_data_find(char *host, int port)
{
  dllist *ptr;

  for (ptr = priv_cfg.ftp_login_hs; ptr; ptr = ptr->next)
  {
    ftp_handshake_info *fhi = (ftp_handshake_info *)ptr->data;

    if ((fhi->port == port || !host[0]) && !strcmp(fhi->host, host))
      return fhi;
  }

  return NULL;
}


static char *ftp_handshake_expand_cmd(char *cmd, doc *docp, char *user, char *password)        /* FIXME: Security */
{
  char ocmd[2048];
  char *p, *op;

  p = cmd;
  op = ocmd;
  *op = '\0';

  while (*p)
  {
    if (*p == '%')
    {
      p++;
      switch (*p)
      {
      case 'u':
        strcpy(op, user);
        break;

      case 'U':
        strcpy(op, priv_cfg.ftp_proxy_user);
        break;

      case 'p':
        strcpy(op, password);
        break;

      case 'P':
        strcpy(op, priv_cfg.ftp_proxy_pass);
        break;

      case 'h':
        strcpy(op, docp->doc_url->p.ftp.host);
        break;

      case 's':
        sprintf(op, "%hu", docp->doc_url->p.ftp.port);
        break;

      default:
        *op = '%';
        op++;
        *op = *p;
        op++;
        *op = '\0';
        break;
      }
      p++;
      while (*op)
        op++;
    }
    else
    {
      *op = *p;
      op++;
      p++;
      *op = '\0';
    }
  }
  strcat(ocmd, "\r\n");

  return tl_strdup(ocmd);
}


static int ftp_do_login_handshake_cust(ftp_handshake_info * fhi, doc * docp, char *user, char *password)
{
  dllist *ptr;

  for (ptr = fhi->infos; ptr; ptr = ptr->next)
  {
    ftp_handshake_info_data *d = (ftp_handshake_info_data *)ptr->data;
    char *cmd;
    int response;

    cmd = ftp_handshake_expand_cmd(d->cmd, docp, user, password);
    ftp_control_write(docp, cmd, strlen(cmd));
    DEBUG_PROTOC("%s", cmd);
    _free(cmd);

    response = ftp_get_response(docp, NULL, TRUE);

    if (response != d->response)
      return -1;
  }

  return 0;
}

