/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "recurse.h"
#include "http.h"
#include "ftp.h"
#include "update_links.h"
#include "mode.h"
#include "url.h"
#include "dns.h"
#include "ainterface.h"
#include "times.h"
#include "schedule.h"
#include "dlhash_tools.h"
#include "nscache.h"
#include "log.h"
#include "authinfo.h"
#include "cookie.h"
#include "net.h"
#include "gui_api.h"
#include "gui.h"
#include "myssl.h"


_config_struct_t cfg;

#ifdef WIN32
/*
 * read string value from HKEY_LOCAL_MACHINE
 */
char *read_lmachine_registry_val(char *path, char *var)
{
  HKEY hKey;
  char rpath[2048];
  DWORD sz = sizeof(rpath);
  DWORD type;
  char *rv = NULL;

  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    return NULL;

  if(RegQueryValueEx(hKey, var, NULL, &type, (LPBYTE) rpath, &sz) == ERROR_SUCCESS)
    rv = tl_strdup(rpath);

  RegCloseKey(hKey);
  return rv;
}

#endif

#undef pavuk_get_program_name
char *pavuk_get_program_name(DBGdeclvoid(void))
{
  char *rv = tld_strdup(DBGpass(cfg.prg_path));
  char *p;

  p = strrchr(rv, '/');
  if(p)
    memmove(rv, p + 1, strlen(p + 1) + 1);
  p = strrchr(rv, '\\');
  if(p)
    memmove(rv, p + 1, strlen(p + 1) + 1);

  return rv;
}


static char *pavuk_get_install_path(DBGdeclvoid(void))
{
  char *rv = NULL;
#if defined(WIN32) || defined(__WIN32)
  rv = read_lmachine_registry_val("Software\\Stefan Ondrejicka\\Pavuk", "Install Path");

  if(rv)
  {
    char *p = rv;
    rv = cvt_win32_to_unix_path(p, FALSE);
    _free(p);
  }
  else
    rv = tld_strdup(DBGpass("/cygdrive/c"));
#else
#ifdef INSTALL_PREFIX
  rv = tld_strdup(DBGpass(INSTALL_PREFIX));
#else
  rv = tld_strdup(DBGpass("/usr/local"));
#endif
#endif

  return rv;
}
#define pavuk_get_install_path()    pavuk_get_install_path(DBGvoid())


static RETSIGTYPE pavuk_alarmsig(int signum)
{
  xprintf(0, gettext("ALARM signal received. Aborting application.\n"));
  DEBUG_DEVEL("cfg.stop = TRUE @ %s, line #%d\n", __FILE__, __LINE__);

  xprintf(0, gettext("Program has been forcefully terminated\n"));
#if defined(I_FACE)
  cfg.xi_face = FALSE;
#endif
  cfg.rbreak = TRUE;
  cfg.stop = TRUE;
  errno = EINTR;
  exit(10);
#if !defined(RETSIGTYPE_IS_VOID)
  return 1;
#endif
}

static RETSIGTYPE pavuk_termsig(int signum)
{
  xprintf(0, gettext("TERM signal received. Aborting application.\n"));
  DEBUG_DEVEL("cfg.stop = TRUE @ %s, line #%d\n", __FILE__, __LINE__);

#ifdef SIGALRM
  tl_signal(SIGALRM, pavuk_alarmsig);
#endif
  xprintf(0, gettext("TERM signal catched\n"));
#if defined(I_FACE)
  cfg.xi_face = FALSE;
#endif
  cfg.rbreak = TRUE;
  cfg.stop = TRUE;
  errno = EINTR;
#ifdef SIGALRM
  alarm(1);
#else
  errno = EINTR;
  tl_sleep(1);
  exit(10);
#endif
#if !defined(RETSIGTYPE_IS_VOID)
  return 1;
#endif
}



#if defined(_MSC_VER) && (defined(WIN32) || defined(_WIN32))
void __cdecl inv_param_handler(const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, unsigned int i1, uintptr_t p1)
{
}
#endif


static void init_values(int argc, char **argv)
{
  char *d;
#ifdef HAVE_GETUID
  uid_t uid;
#endif
  time_t i__time = time(NULL);  /* __time is defined by several C RTLs out there; make sure this variable does not collide with those defs */
  char pom[PATH_MAX];
  struct hostent *hp = NULL;
  /* struct passwd *pwstruct; */
#ifdef GETTEXT_NLS
  int i;
#endif
  memset(&cfg, 0, sizeof(cfg));
#ifdef I_FACE
  memset(&gui_cfg, 0, sizeof(gui_cfg));
#endif

#if defined(_MSC_VER) && (defined(WIN32) || defined(_WIN32))
  {
    int tmp;

    /* Get the current bits */
    tmp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

#if 0
    /* Clear the upper 16 bits and OR in the desired freqency */
    tmp = (tmp & 0x0000FFFF) | _CRTDBG_CHECK_EVERY_16_DF;
#endif
    tmp |= _CRTDBG_LEAK_CHECK_DF;

    /* Set the new bits */
    _CrtSetDbgFlag(tmp);

	_set_invalid_parameter_handler(inv_param_handler);
  }
#endif

#if defined(PTW32_STATIC_LIB)
  pthread_win32_process_attach_np();
  pthread_win32_thread_attach_np();
#endif

#ifdef HAVE_WINSOCK2_H
  {
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);
    if(err != 0)
    {
      xperror("Could not find a usable Winsock DLL");
    }

    /* Confirm that the WinSock DLL supports 2.2. */
    /* Note that if the DLL supports versions greater    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* requested.                                        */
    if(LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
      WSACleanup();
      xperror("Could not find a usable Winsock v2.2 (or higher) DLL");
    }

    /* The WinSock DLL is acceptable. Proceed. */
  }
#endif

  cfg.prg_path = argv[0];
  cfg.install_path = pavuk_get_install_path();

  cfg.dump_fd = bufio_new(TRUE, 1024000);
  cfg.dump_urlfd = bufio_new(TRUE, 1024000);
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  cfg.rec_act_dump_fd = bufio_new(TRUE, 1024000);
#endif

  cfg.logfile = bufio_new(TRUE, 1024000);
  cfg.short_logfile = bufio_new(TRUE, 1024000);
  cfg.time_logfile = bufio_new(TRUE, 1024000);
  cfg.time_logfile4sum = bufio_new(TRUE, 1024000);

  cfg.save_scn = bufio_new(TRUE, 10000);
  cfg.dump_cmd_fname = bufio_new(TRUE, 10000);

#ifdef HAVE_MT
  mt_init();
#endif

  init_locale_env();

#ifdef GTK_FACE
  cfg.path_to_home = tl_strdup(g_get_home_dir());
#endif
  if(!cfg.path_to_home)
    cfg.path_to_home = tl_strdup(getenv("HOME"));
  if(!cfg.path_to_home)
    cfg.path_to_home = tl_strdup("/tmp/");

#ifdef HAVE_TZSET
  tzset();
#endif

#ifdef I_FACE
  cfg.done = FALSE;
#endif
  cfg.urlstack = NULL;
  cfg.urls_in_dir = NULL;
  cfg.total_cnt = 0;
  cfg.time = new_tm(localtime(&i__time));
  cfg.time->tm_year += 1900;
  cfg.fail_cnt = 0;
  cfg.docnr = 0;

  cfg.url_hash_tbl = NULL;
  cfg.fn_hash_tbl = NULL;
  cfg.last_used_proxy_node = NULL;

  cfg_setup_default();

#ifdef GETTEXT_NLS
#ifdef GETTEXT_DEFAULT_CATALOG_DIR
  cfg.msgcatd = tl_strdup(GETTEXT_DEFAULT_CATALOG_DIR);
#endif
#if defined(__CYGWIN__) || defined(WIN32) || defined(__WIN32)
  _free(cfg.msgcatd);
  cfg.msgcatd = tl_str_concat(DBGvars(NULL), cfg.install_path, "/share/locale", NULL);
#endif
#else
  cfg.language = "C";
#endif

  _INIT_NLS;

#ifdef SOCKS
  SOCKSinit(argv[0]);
#endif

  if(!(d = getenv("USER")))
  {
#ifdef HAVE_GETUID
    struct passwd *pwstruct;

    uid = getuid();
    if((pwstruct = getpwuid(uid)))
    {
      d = tl_strdup(pwstruct->pw_name);
    }
    else
#endif
    {
      d = tl_strdup("[anon]");
    }
  }
  else
  {
    d = tl_strdup(d);
  }

  if(gethostname(pom, sizeof(pom)))
  {
#ifdef HAVE_WINSOCK2_H
    errno = WinSockErr2errno(WSAGetLastError());
#endif
    xperror("gethostname");
  }
  else
  {
    cfg.local_host = tl_strdup(pom);
    hp = gethostbyname(pom);
  }

  if(hp)
  {
    if(d)
      snprintf(pom, sizeof(pom), "%s@%s", d, hp->h_name);
    else
      snprintf(pom, sizeof(pom), "pavuk@%s", hp->h_name);
    pom[sizeof(pom) - 1] = 0;
  }
  else
  {
    if(d)
      snprintf(pom, sizeof(pom), "%s@unknown.sk", d);
    else
      strncpy(pom, "pavuk@unknown.sk", sizeof(pom));
    pom[sizeof(pom) - 1] = 0;
  }

  _free(d);
  cfg.from = tl_strdup(pom);

  cfg_load_setup();

  cfg.stop = FALSE;
  cfg.rbreak = FALSE;

#ifdef I_FACE
  cfg.xi_face = FALSE;

  for(i = 1; i < argc; i++)
  {
    /*** load preferences ***/
    if(!strcasecmp(argv[i], "-prefs"))
    {
      cfg.use_prefs = TRUE;
    }
    else if(!strcasecmp(argv[i], "-noprefs"))
    {
      cfg.use_prefs = FALSE;
    }
    /**** we want to run GUI ****/
    else if(!strcasecmp(argv[i], "-X"))
    {
     /**** we want to run GUI ****/
      cfg.xi_face = TRUE;
    }
  }
  if(cfg.use_prefs && cfg.xi_face)
  {
    if(cfg_load_pref())
    {
      /* FIXME: handle error condition? */
#if 0
      xprintf(0, gettext("ERROR: failed to load configuration preferences\n"));
      exit(PAVUK_EXIT_CFG_ERR);
#endif
    }
  }
#endif

  _INIT_NLS;

#ifdef GETTEXT_NLS
/*** these parameters have to be resolved before each other ***/
  for(i = 1; i < argc; i++)
  {
    if(!strcasecmp(argv[i], "-msgcat"))
    {
      i++;
      if(i < argc)
      {
        cfg.msgcatd = tl_strdup(argv[i]);
      }
      else
      {
        xprintf(0, gettext("Not enough number of parameters \"-msgcat\"\n"));
        usage();
      }
    }
    if(!strcasecmp(argv[i], "-language"))
    {
      i++;
      if(i < argc)
      {
        cfg.language = tl_strdup(argv[i]);
      }
      else
      {
        xprintf(0, gettext("Not enough number of parameters \"-language\"\n"));
        usage();
      }
    }
  }
#endif

  _INIT_NLS;

  /*
   * [i_a] make sure generic cleanup is activated just before we start
   * loading (possibly bad) commandline arguments / scenarios
   */
  atexit(pavuk_do_at_exit);

  cfg_setup_cmdln(argc, argv);

#if defined(I_FACE)
  /**** if requested, create GUI ****/
  if(cfg.xi_face)
  {
#if defined I_FACE && !defined HAVE_MT
    dns_serv_start();
#endif
    gui_start(&argc, argv);
  }
#endif

#ifdef GETTEXT_NLS
  /* [i_a] only use default if not already set from commandline */
  if(!cfg.language)
  {
    cfg.language = tl_strdup(getenv("LC_MESSAGES"));
  }
#endif

  /* dump_fd/dump_urlfd handle checking moved inside cfg_setup_cmdln() */

  cfg.url_hash_tbl = dlhash_new(cfg.hash_size, url_key_func, url_hash_func, dllist_url_compare);
  dlhash_set_free_func(cfg.url_hash_tbl, url_free_func, NULL);
  cfg.fn_hash_tbl = dlhash_new(cfg.hash_size, fn_key_func, str_hash_func, str_comp_func);
  dlhash_set_free_func(cfg.fn_hash_tbl, NULL, NULL);

  memset(&cfg.local_ip_addr, '\0', sizeof(cfg.local_ip_addr));
  if(cfg.local_ip && net_host_to_in_addr(cfg.local_ip, &cfg.local_ip_addr))
  {
    xherror(cfg.local_ip);
  }

  if(cfg.cache_dir)
  {
    d = cfg.cache_dir;
    cfg.cache_dir = get_abs_file_path_oss(cfg.cache_dir);
    _free(d);
  }
  else
  {
    getcwd(pom, sizeof(pom));
#if defined(WIN32) || defined(__WIN32)
    cfg.cache_dir = cvt_win32_to_unix_path(pom, TRUE);
#else
    cfg.cache_dir = tl_strdup(pom);
#endif
  }

  if(cfg.subdir)
  {
    d = cfg.subdir;
    cfg.subdir = get_abs_file_path_oss(cfg.subdir);
    _free(d);
    if(tl_is_dirname(cfg.subdir))
      *(cfg.subdir + strlen(cfg.subdir) - 1) = '\0';
  }

  cfg.prev_mode = cfg.mode;

  /* [i_a] added - cfg_dump_cmd() was unused but useful */
  cfg_dump_cmd(&cfg.dump_cmd_fname);
  ASSERT(cfg.dump_cmd_fname ? !bufio_is_open(cfg.dump_cmd_fname) : TRUE);

  cfg_dump(&cfg.save_scn);
  ASSERT(cfg.save_scn ? !bufio_is_open(cfg.save_scn) : TRUE);

  if(cfg.cookie_file)
    cookie_read_file(cfg.cookie_file);

  if(cfg.auth_file)
    authinfo_load(cfg.auth_file);

  log_start("[commandline]");

  if(!cfg.sched_cmd)
    cfg.sched_cmd = tl_strdup(AT_CMD);

  if(cfg.schtime)
  {
    _free(cfg.time);
    cfg.time = new_tm(localtime(&cfg.schtime));
  }

  if(!cfg.index_name)
    cfg.index_name = tl_strdup("_._.html");

#ifdef USE_SSL
  my_ssl_init_once();
#endif

  if(cfg.bgmode)
  {
#ifdef HAVE_FORK
    pid_t ppid;

    ppid = fork();
    if(ppid < 0)
    {
      xperror("fork");
      xprintf(1, gettext("Unable to fork pavuk to background - running in foreground\n"));
    }
    else if(ppid != 0)
    {
      xprintf(0, gettext("Pavuk will run at backround as PID %d\n"), (int) ppid);
      exit(PAVUK_EXIT_CFG_ERR);
    }
#endif
#if defined(__CYGWIN__) || defined(WIN32) || defined(__WIN32)
    FreeConsole();
#endif
  }
}

static void read_urls(char *filename)
{
  bufio *fd;
  char lnbuf[BUFIO_ADVISED_READLN_BUFSIZE];
  int n;
  bool_t isstdin;

  isstdin = !strcmp(cfg.urls_file, "-");

  DEBUG_MISC(gettext("reading URLs from file - %s\n"), filename);
  if(isstdin)
    fd = bufio_fdopen(0);
  else
    fd = bufio_open(filename, O_BINARY | O_RDONLY);
  if(!fd)
  {
    xperror(mk_native(filename));
    return;
  }

  while((n = bufio_readln(fd, lnbuf, sizeof(lnbuf))) > 0)
  {
    strip_nl(lnbuf);
    if(!strcmp(lnbuf, "."))
      break;

    if(lnbuf[0])
    {
      url_info *ui;

      /*
         heuristics: see if we can decode the line in '-request' style,
         if not, assume it is an old-style url with no frills.
       */
      ui = url_info_parse(lnbuf, TRUE);
      if(!ui)
      {
        ui = url_info_new(lnbuf);
      }
      ASSERT(ui != NULL);
      cfg.request = dllist_append(cfg.request, (dllist_t) ui);
    }
  }

  if(n < 0)
    xperror("reading URLs from file");

  if(isstdin)
  {
    bufio_free(fd);
  }
  else
  {
    bufio_close(fd);
  }
  fd = NULL;
}

int main(int argc, char **argv)
{
  time_t i__time = time(NULL);

  init_values(argc, argv);

  /* [i_a] moved atexit into init_values so generic cleanup code gets a chance even when scenario to load is bad */

  /*
     pro: We do not fully trust pavuk to stop working after the timeout
     expired, so we order the OS to send us an ALARM signal one minute
     after the timeout expires. If the program still runs at that time
     it will be aborted once the signal strikes home.
   */
  if(cfg.max_time > 0)
  {
#ifdef HAVE_ALARM
    alarm((int) ((cfg.max_time + 1) * 60));
#else
    xprintf(1, gettext("pavuk '-max_time' feature not supported on this platform.\n"));
    exit(PAVUK_EXIT_CFG_ERR);
#endif
  }

  if(cfg.urls_file)
  {
    read_urls(cfg.urls_file);
    _free(cfg.urls_file);
  }

  /*
     pro: Set seed for random generator; needed to find a port for
     active ftp.
   */
  srand((int) time(NULL) ^ getpid());

  tl_signal(SIGINT, pavuk_intsig);
  tl_signal(SIGTERM, pavuk_termsig);
#ifdef SIGALRM
  tl_signal(SIGALRM, pavuk_alarmsig);
#endif
#ifdef SIGPIPE
  tl_signal(SIGPIPE, SIG_IGN);
#endif

/**** spustenie algoritmu alebo rozhrania ****/
/**** FIXME: Translate me!                ****/
#if defined(I_FACE)
  if(cfg.xi_face)
  {
    cfg.prev_mode = cfg.mode;
    cfg.mode_started = FALSE;

    gui_main();
  }
  else
#endif
  {
#ifdef SIGQUIT
    tl_signal(SIGQUIT, pavuk_quitsig);
#endif
#ifdef SIGXFSZ
    tl_signal(SIGXFSZ, pavuk_filesizesig);
#endif

    if(cfg.schtime)
    {
      cfg.schtime = (time_t) 0;
      if(at_schedule())
      {
        xprintf(0, gettext("Error scheduling\n"));
      }
    }
    else
    {
      if(cfg.reschedh)
      {
        i__time += 3600 * cfg.reschedh;
        _free(cfg.time);
        cfg.time = new_tm(localtime(&i__time));
        at_schedule();
      }
      absi_restart();
    }
  }

  /* log_start(NULL); -- [i_a] handled in at_exit() handler */
  /* pavuk_do_at_exit(); */

  return cfg.fail_cnt ? PAVUK_EXIT_DOC_ERR : PAVUK_EXIT_OK;
}
