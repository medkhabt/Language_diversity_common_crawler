/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/


#include "config.h"

#include "dns.h"
#include "log.h"
#include "htmlparser.h"
#include "robots.h"
#include "ainterface.h"
#include "jsbind.h"
#include "myssl.h"

void pavuk_do_at_exit(void)
{
#if defined(__CYGWIN__) || defined(WIN32) || defined(__WIN32)
  int ewait;
#endif

#if defined(I_FACE) && !defined(HAVE_MT)
  dns_server_kill();
#endif
#ifdef HAVE_MOZJS
  pjs_destroy();
#endif
#ifdef USE_SSL
  my_ssl_cleanup();
#endif
  robots_do_cleanup();
  html_parser_do_cleanup();

#if defined(HAVE_MT) && defined(I_FACE)
  if(!cfg.xi_face)
#endif
  {
    free_all();
  }

#if 0
  ASSERT(cfg.url_hash_tbl);
  ASSERT(cfg.fn_hash_tbl);
#endif

  if(cfg.url_hash_tbl)
  {
    dlhash_free(cfg.url_hash_tbl);
    cfg.url_hash_tbl = NULL;
  }
  if(cfg.fn_hash_tbl)
  {
    dlhash_free(cfg.fn_hash_tbl);
    cfg.fn_hash_tbl = NULL;
  }

  ASSERT(!cfg.url_hash_tbl);
  ASSERT(!cfg.fn_hash_tbl);

  dns_free_tab();
  /* cfg_free_params(); [i_a] */
  while(cfg.request)
  {
    url_info_free((url_info *) cfg.request->data);
    cfg.request->data = NULL;
    cfg.request = dllist_remove_entry(cfg.request, cfg.request);
  }

  cleanup_cookies();

  ASSERT(!cfg.urlstack);
  ASSERT(!cfg.urls_in_dir);

  _free(cfg.time);
  _free(cfg.local_host);
  _free(cfg.path_to_home);
  _free(cfg.install_path);

  log_end();
#if defined(__CYGWIN__) || defined(WIN32) || defined(__WIN32)
  ewait = cfg.wait_on_exit;     /* [i_a] fix: this parameter will be destroyed by the next call! */
#endif
  cfg_free_params();            /* <-- will change the bufio items! */
  bufio_close(cfg.save_scn);
  cfg.save_scn = NULL;
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  bufio_close(cfg.rec_act_dump_fd);
  cfg.rec_act_dump_fd = NULL;
#endif
  bufio_close(cfg.dump_cmd_fname);
  cfg.dump_cmd_fname = NULL;
  bufio_close(cfg.dump_fd);
  cfg.dump_fd = NULL;
  bufio_close(cfg.dump_urlfd);
  cfg.dump_urlfd = NULL;
  bufio_close(cfg.logfile);
  cfg.logfile = NULL;
  bufio_close(cfg.short_logfile);
  cfg.short_logfile = NULL;
  bufio_close(cfg.time_logfile);
  cfg.time_logfile = NULL;
  bufio_close(cfg.time_logfile4sum);
  cfg.time_logfile4sum = NULL;

  free_mime_types();

  cleanup_native_pool();

  cfg_cmdln_cleanup();

#ifdef HAVE_WINSOCK2_H
  WSACleanup();
#endif

#if defined(PTW32_STATIC_LIB)
  pthread_win32_thread_detach_np();
  pthread_win32_process_detach_np();
#endif

#if defined(__CYGWIN__) || defined(WIN32) || defined(__WIN32)
  if(isatty(0) && ewait)
  {
    printf(gettext("press any key to exit\n"));
    getc(stdin);
  }
#endif
}
