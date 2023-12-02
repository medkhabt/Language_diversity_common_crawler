/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "dns.h"
#include "gui_api.h"
#include "absio.h"
#include "net.h"
#include "doc.h"
#include "http.h"

#if defined(WIN32) || defined(__WIN32)

int WinSockErr2errno(int err)
{
  switch (err)
  {
  case 0:
    return 0;

  default:
    return EFAULT;

  case WSAEWOULDBLOCK:
    return EWOULDBLOCK;

  case WSAEINPROGRESS:
    return EINPROGRESS;

  case WSAEALREADY:
    return EALREADY;

  case WSAENOTSOCK:
    return ENOTSOCK;

  case WSAEDESTADDRREQ:
    return EDESTADDRREQ;

  case WSAEMSGSIZE:
    return EMSGSIZE;

  case WSAEPROTOTYPE:
    return EPROTOTYPE;

  case WSAENOPROTOOPT:
    return ENOPROTOOPT;

  case WSAEPROTONOSUPPORT:
    return EPROTONOSUPPORT;

  case WSAESOCKTNOSUPPORT:
    return ESOCKTNOSUPPORT;

  case WSAEOPNOTSUPP:
    return EOPNOTSUPP;

  case WSAEPFNOSUPPORT:
    return EPFNOSUPPORT;

  case WSAEAFNOSUPPORT:
    return EAFNOSUPPORT;

  case WSAEADDRINUSE:
    return EADDRINUSE;

  case WSAEADDRNOTAVAIL:
    return EADDRNOTAVAIL;

  case WSAENETDOWN:
    return ENETDOWN;

  case WSAENETUNREACH:
    return ENETUNREACH;

  case WSAENETRESET:
    return ENETRESET;

  case WSAECONNABORTED:
    return ECONNABORTED;

  case WSAECONNRESET:
    return ECONNRESET;

  case WSAENOBUFS:
    return ENOBUFS;

  case WSAEISCONN:
    return EISCONN;

  case WSAENOTCONN:
    return ENOTCONN;

  case WSAESHUTDOWN:
    return ESHUTDOWN;

  case WSAETOOMANYREFS:
    return ETOOMANYREFS;

  case WSAETIMEDOUT:
    return ETIMEDOUT;

  case WSAECONNREFUSED:
    return ECONNREFUSED;

  case WSAELOOP:
    return ELOOP;

  case WSAENAMETOOLONG:
    return ENAMETOOLONG;

  case WSAEHOSTDOWN:
    return EHOSTDOWN;

  case WSAEHOSTUNREACH:
    return EHOSTUNREACH;

  case WSAENOTEMPTY:
    return ENOTEMPTY;

  case WSAEPROCLIM:
    return EPROCLIM;

  case WSAEUSERS:
    return EUSERS;

  case WSAEDQUOT:
    return EDQUOT;

  case WSAESTALE:
    return ESTALE;

  case WSAEREMOTE:
    return EREMOTE;
  }
}

int net_host_to_in_addr(char *hostname, abs_addr * haddr)
{
  int len;

  if(dns_gethostbyname(hostname, &len, haddr->addr, &haddr->family))
    return -1;

  return 0;
}



/*****************************************************************/
/* otvori spojenie na dany port servera  s (IP, DNS) adresou   */
/* FIXME: Translate me!                                          */
/*****************************************************************/
static int net_connect_helper(char *hostname, int port_no, doc * docp)
{
  struct sockaddr *addr;
  struct sockaddr_storage saddr;
  abs_addr haddr;
  SOCKET sock;
  int rv, l, sas, tcp;
  u_long val;

  int actual_port = port_no;
  char *actual_hostname = hostname;
  http_proxy *hp;

  if(docp->doc_url->type == URLT_HTTPS && cfg.transparent_ssl_proxy)
    hp = cfg.transparent_ssl_proxy;
  else
    hp = cfg.transparent_proxy;

  /*
   * Check for transparent redirect
   */
  if(hp)
  {
    actual_hostname = hp->addr;
    actual_port = hp->port;
  }

  set_h_errno(0);

  if(net_host_to_in_addr(actual_hostname, &haddr))
    return -2;

  gettimeofday(&docp->dns_time, NULL);

  _Xt_Serve;

  tcp = dns_getprotoid("tcp");

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  if(docp->actrec_ref)
  {
    activity_record *rec = docp->actrec_ref;

    DEBUG_DEVEL("set host connect info #1: %d [%d/%s]\n", docp->doc_url->type, __LINE__, __FILE__);
    rec->type = docp->doc_url->type;
    rec->host_name = tl_strdup(actual_hostname);
    rec->host_port = actual_port;
    rec->socket_family = haddr.family;
    rec->socket_proto = tcp;
  }
#endif

  if((sock = socket(haddr.family, SOCK_STREAM, tcp)) == INVALID_SOCKET)
  {
    errno = WinSockErr2errno(WSAGetLastError());
    xperror("socket()");
    return -1;
  }

  val = 1;
  if(SOCKET_ERROR == ioctlsocket(sock, FIONBIO, &val))
  {
    errno = WinSockErr2errno(WSAGetLastError());
    xperror("fcntl() - F_SETFL");
    closesocket(sock);
    return -1;
  }

  if(cfg.local_ip)
  {
    struct sockaddr_storage sladdr;
    struct sockaddr *laddr;

    laddr = dns_setup_sockaddr(&cfg.local_ip_addr, 0, &sladdr, &sas);

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
    if(docp->actrec_ref)
    {
      activity_record *rec = docp->actrec_ref;

      DEBUG_DEVEL("set host connect local bind info [%d/%s]\n", __LINE__, __FILE__);
      rec->local_addr = _malloc(sas);
      memcpy(rec->local_addr, laddr, sas);
      rec->local_addr_len = sas;
    }
#endif

    if(bind(sock, laddr, sas) == SOCKET_ERROR)
    {
      errno = WinSockErr2errno(WSAGetLastError());
      xperror("bind() - local");
      closesocket(sock);
      return -1;
    }
  }

  addr = dns_setup_sockaddr(&haddr, actual_port, &saddr, &sas);

#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  if(docp->actrec_ref)
  {
    activity_record *rec = docp->actrec_ref;

    DEBUG_DEVEL("set host connect dest addr info [%d/%s]\n", __LINE__, __FILE__);
    rec->dest_addr = _malloc(sas);
    memcpy(rec->dest_addr, addr, sas);
    rec->dest_addr_len = sas;
  }
#endif

  rv = connect(sock, addr, sas);
  errno = WinSockErr2errno(WSAGetLastError());
  if(rv == SOCKET_ERROR && (errno != EINPROGRESS) && (errno != EISCONN) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
  {
    closesocket(sock);
    return -1;
  }


#if defined(I_FACE) && !defined(HAVE_MT)
  if(cfg.xi_face)
  {
    if(rv == SOCKET_ERROR && (errno == EINPROGRESS))
    {
      rv = gui_wait_io(sock, FALSE);
      if(!rv)
      {
        DEBUG_NET("Async connect - connected\n");
      }

      if(cfg.rbreak || cfg.stop || rv)
      {
        closesocket(sock);
        return -1;
      }
    }
  }
  else
#endif /* !HAVE_MT */
  {
    if(rv == SOCKET_ERROR && (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK))
    {
      while(!cfg.stop && !cfg.rbreak && (rv = tl_selectw((int) sock, (int) cfg.conn_timeout)) == -1 && (errno == EINTR));
      if(rv <= 0)
      {
        if(rv == 0)
          errno = ETIMEDOUT;
        return -1;
      }
    }
  }

  l = sizeof(rv);
  if(getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *) &rv, &l) || rv)
  {
    closesocket(sock);
    errno = WinSockErr2errno(rv);
    return -1;
  }

  DEBUG_NET("Successfully connected to %s,%d\n", hostname, port_no);

  return (int) sock;
}

int net_connect(char *hostname, int port_no, doc * docp)
{
  int rc = net_connect_helper(hostname, port_no, docp);
  /* if net_connect_helper() returned -2, this was dns
   * failure. If -1, it was connect() failure
   */
  if(rc == -2)
  {
    DEBUG_NET("DNS failure for %s,%d\n", hostname, port_no);
    rc = -1;
  }
  else
  {
    gettimeofday(&docp->connect_time, NULL);
  }
  return rc;
}

int net_bindport(abs_addr * haddr, int port_min, int port_max)
{
  SOCKET sock;
  int port;
  int n, tcp;
  struct sockaddr *addr;
  struct sockaddr_storage saddr;
  char pom[512];

  addr = (struct sockaddr *) &saddr;

  tcp = dns_getprotoid("tcp");

  if((sock = socket(haddr->family, SOCK_STREAM, tcp)) == -1)
  {
    errno = WinSockErr2errno(WSAGetLastError());
    xperror("socket");
    return -1;
  }

  n = 1;
  if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &n, sizeof(n)))
  {
    errno = WinSockErr2errno(WSAGetLastError());
    xperror("net_bind: setsockopt - SO_REUSEADDR");
  }

  if(port_min < 0 || port_max < 0)
  {
    port = 0;

    addr = dns_setup_sockaddr(haddr, port, &saddr, &n);

    if(bind(sock, addr, n) != 0)
    {
      errno = WinSockErr2errno(WSAGetLastError());
      snprintf(pom, sizeof(pom), "bind to port(%d)", port);
      xperror(pom);
      closesocket(sock);
      return -1;
    }
  }
  else
  {
    /*
       pro: take a port out of the specified range
     */
    int count = 50;
    double port_range = port_max - port_min + 1;
    int success = 0;

    DEBUG_NET("trying ports in range %d - %d\n", port_min, port_max);
    while(count >= 0)
    {
      if(count == 0)
      {
        port = 0;
        DEBUG_NET("trying system assigned port\n");
      }
      else
      {
        port = port_min + (int) (port_range * rand() / (RAND_MAX + 1.0));
        ASSERT(port >= port_min && port <= port_max);
      }

      addr = dns_setup_sockaddr(haddr, port, &saddr, &n);

      if(bind(sock, addr, n) == 0)
      {
        success = 1;
        break;
      }

      DEBUG_NET("trying new port : port %d failed\n", port);

      count--;
    }

    if(!success)
    {
      errno = WinSockErr2errno(WSAGetLastError());
      snprintf(pom, sizeof(pom), "bind to ports (%d-%d)", port_min, port_max);
      xperror(pom);
      closesocket(sock);
      return -1;
    }
  }

  if(getsockname(sock, addr, &n))
  {
    errno = WinSockErr2errno(WSAGetLastError());
    xperror("getsockname");
    closesocket(sock);
    return -1;
  }

  DEBUG_NET("BINDING to port : %d\n", dns_get_sockaddr_port(addr));

  if(listen(sock, 1) == -1)
  {
    errno = WinSockErr2errno(WSAGetLastError());
    xperror("listen");
    closesocket(sock);
    return -1;
  }

  return (int) sock;
}

int net_accept(int sock)
{
  struct sockaddr *caller;
  struct sockaddr_storage scaller;
  int p;
  SOCKET rsock = INVALID_SOCKET;
  int rv;
  u_long val;

  ASSERT(rsock < 0);
  val = 1;
  if(SOCKET_ERROR == ioctlsocket(sock, FIONBIO, &val))
  {
    errno = WinSockErr2errno(WSAGetLastError());
    xperror("fcntl() - F_SETFL");
    closesocket(sock);
    return -1;
  }

  caller = (struct sockaddr *) &scaller;
  p = sizeof(scaller);

  rsock = accept(sock, caller, &p);
  errno = WinSockErr2errno(WSAGetLastError());
  if((rsock < 0) && (errno != EWOULDBLOCK) && (errno != EAGAIN))
  {
    closesocket(sock);
    return -1;
  }

#if defined(I_FACE) && !defined(HAVE_MT)
  if(cfg.xi_face)
  {
    if((rsock < 0) && (errno == EWOULDBLOCK || errno == EAGAIN))
    {
      rv = gui_wait_io(sock, TRUE);
      if(rv || cfg.rbreak || cfg.stop)
      {
        closesocket(sock);
        return -1;
      }
    }
  }
  else
#endif /* !HAVE_MT */
  {
    if((rsock < 0) && (errno == EWOULDBLOCK || errno == EAGAIN))
    {
      while((rv = tl_selectr(sock, (int) cfg.conn_timeout)) == -1 && errno == EINTR);
      if(rv <= 0)
      {
        if(rv == 0)
          errno = ETIMEDOUT;
        closesocket(sock);
        return -1;
      }
    }
  }

  if((rsock < 0) && ((rsock = accept(sock, caller, &p)) == -1))
  {
    errno = WinSockErr2errno(WSAGetLastError());
    closesocket(sock);
    return -1;
  }

#ifdef HAVE_DEBUG_FEATURES
  if(cfg.verbose)
  {
    char *ip = dns_get_sockaddr_ip(caller);
    DEBUG_NET("ACCEPTING connection from: %s:%d\n", ip, dns_get_sockaddr_port(caller));
    _free(ip);
  }
#endif

  return (int) rsock;
}



/* [i_a] apr/2007 */
int sock_maybe_in_fdset(fd_set * set, int sock)
{
  int i;

  if(sock < 0)
    return FALSE;

  /* if set isn't filled up yet, this socket might fit in there. */
  if(set->fd_count < FD_SETSIZE)
    return TRUE;
  /* otherwise it MUST be in there */
  for(i = 0; i < set->fd_count; i++)
  {
    if(set->fd_array[i] == sock)
      return TRUE;
  }

  return FALSE;
}




#endif
