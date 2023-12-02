/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _config_h_
#define _config_h_

#ifdef __GNUC__
/* Enable GNU extensions in sources  */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#ifndef __USE_GNU
#define __USE_GNU 1
#endif
#endif

/* [i_a] */
#if defined(_MSC_VER) && (defined(WIN32) || defined(_WIN32))
/* Microsoft Visual C - native Win32 port/build */
#include "../ac-config.Win32.h"
#else
#include "../ac-config.h"
#endif






/*
   special debugging routines; used in conjunction with memalloc debugging libs
   (e.g. crtdbg debug support on Win32/MSVC) to track down memory leakage due to
   heap memory allocations which are not freed before program termination.
 */
#if defined(DEBUG)

#define DBGdecl(arg)        const char *sourcecode_file, int sourcecode_line, arg
#define DBGpass(arg)        sourcecode_file, sourcecode_line, arg
#define DBGvars(arg)        __FILE__, __LINE__, arg
#define DBGvoid()           __FILE__, __LINE__
#define DBGdeclvoid(void)   const char *sourcecode_file, int sourcecode_line

#else

#define DBGdecl(arg)	    arg
#define DBGpass(arg)        arg
#define DBGvars(arg)    	arg
#define DBGvoid()           /**/
#define DBGdeclvoid(void)   void

#endif /* DEBUG */




#if defined(HAVE_LLSEEK) || defined(HAVE_LSEEK64)
#define _LARGEFILE64_SOURCE     /* see 'man lseek64' */
#endif


#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif

#ifdef __CYGWIN__
#include <sys/cygwin.h>
#endif

#if defined(__CYGWIN__) && (defined(WIN32) || defined(__WIN32))
#include <windows.h>
#else
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_FLOCK
#include <sys/file.h>
#endif

#ifdef HAVE_FNMATCH
#include <fnmatch.h>
#else
#include "fnmatch.h"
#endif

#ifdef HAVE_FSTATVFS
#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#else
#ifdef HAVE_FSTATFS
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#endif
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_TERMIOS
#include <termios.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <assert.h>
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <errno.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <limits.h>
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_PROCESS_H
#include <process.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UTIME_H
#include <utime.h>
#endif
#ifdef HAVE_SYS_UTIME_H
#include <sys/utime.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif
#ifdef HAVE_PROCESS_H
#include <process.h>
#endif

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#else
#if defined(WIN32) || defined(__WIN32)
#include "dirent-win32.h"
#endif
#endif
#ifdef HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#ifdef HAVE_NDIR_H
#include <ndir.h>
#endif


#if defined(WIN32) || defined(__WIN32)

#if defined(DEBUG)
#define _CRTDBG_MAP_ALLOC 1
#endif
#include <crtdbg.h>

#endif


#if !defined(HAVE_TYPE_SIGHANDLER_T)
typedef RETSIGTYPE(*sighandler_t) (int);
#endif


#if defined(PTW32_VERSION)
extern long pthread_t2long(pthread_t t);
#else
#define pthread_t2long(t)    ((long)(t))
#endif






#if !defined(__CYGWIN__) && (defined(WIN32) || defined(__WIN32))

extern int tl_rename(const char *p1, const char *p2);
extern int tl_stat(const char *p, struct stat *s);
extern int tl_lstat(const char *p, struct stat *s);
extern int tl_link(const char *p1, const char *p2);
extern int tl_symlink(const char *p1, const char *p2);
extern int tl_unlink(const char *p);
extern int tl_open(const char *p, int flags, int mode);
extern int tl_access(const char *p, int flags);
extern int tl_mkdir(const char *p, int rights);
extern int tl_rmdir(const char *p);
extern DIR *tl_opendir(const char *p);
extern FILE *tl_fopen(const char *p, const char *mode);

extern const char *mk_native(const char *path);
extern void set_mk_native_pool(int threadcount);
extern void cleanup_native_pool(void);

#else

#define tl_rename(p1, p2)         rename(p1, p2)
#define tl_stat(p, s)             stat(p, s)
#define tl_lstat(p, s)            lstat(p, s)
#define tl_link(p1, p2)           link(p1, p2)
#define tl_symlink(p1, p2)        symlink(p1, p2)
#define tl_unlink(p)              unlink(p)
#define tl_open(p, flags, mode)   open(p, flags, mode)
#define tl_access(p, flags)       access(p, flags)
#define tl_mkdir(p, rights)       mkdir(p, rights)
#define tl_rmdir(p)               rmdir(p)
#define tl_opendir(p)             opendir(p)
#define tl_fopen(p, mode)         fopen(p, mode)

#define mk_native(path)           path
#define set_mk_native_pool(size)  (void)(size)
#define cleanup_native_pool() /**/

#endif







/*
   here can you specify characters,
   which are unsafe in file paths.

   FIXME: these are unsafe FileSYSTEM characters,
   i.e. we don't take proper care of Win32 path
   specifications, which use '\\' next to '/'
   (latest Windows also seems to support '/' out
   of the box, but doesn't return this path delimiter
   itself though).
 */
#if defined(__CYGWIN__) || defined(WIN32) || defined(__WIN32)
#define FS_UNSAFE_CHARACTERS "\\:*?\"<>|&^%"
#else
#define FS_UNSAFE_CHARACTERS "*?\"<>|"
#endif




/*
   use as '#if DEBUG_DEADLOCK' instead of '#ifdef DEBUG_DEADLOCK'!
   This allows you to predefine this one or set a default if not yet
   specified.
*/
#ifndef DEBUG_DEADLOCK
#if defined(DEBUG)
#define DEBUG_DEADLOCK 0
#else
#define DEBUG_DEADLOCK 0
#endif
#endif


#define DUMP_SECTION_SEPSTR   "\n"          \
  "##############################################################\n"



#if defined(__CYGWIN__) || defined(WIN32) || defined(__WIN32)
#define PAVUK_LOCK_FILENAME         "_._lock"
#define PAVUK_INFO_DIRNAME          "_.pavuk_info"
#define PAVUK_FNAME_PREFIX          "_.in_"
#define PAVUK_TEMPFNAME_PREFIX      "_._"
#define PAVUK_CONFIG_FILENAME       "pavukrc"
#define PAVUK_PREFERENCES_FILENAME  "_.pavuk_prefs"
#define PAVUK_REMIND_DB_FILENAME    "_.pavuk_remind_db"
#define PAVUK_GTK_KEYS_FILENAME     "_.pavuk_keys"
#define PAVUK_CFGFNAME_PREFIX       "_."
#else
#define PAVUK_LOCK_FILENAME         "._lock"
#define PAVUK_INFO_DIRNAME          ".pavuk_info"
#define PAVUK_FNAME_PREFIX          ".in_"
#define PAVUK_TEMPFNAME_PREFIX      "._"
#define PAVUK_CONFIG_FILENAME       "pavukrc"
#define PAVUK_PREFERENCES_FILENAME  ".pavuk_prefs"
#define PAVUK_REMIND_DB_FILENAME    ".pavuk_remind_db"
#define PAVUK_GTK_KEYS_FILENAME     ".pavuk_keys"
#define PAVUK_CFGFNAME_PREFIX       "."
#endif


#ifndef HAVE_SNPRINTF
extern int snprintf(char *buf, size_t buflen, const char *msg, ...);
#endif
#ifndef HAVE_VSNPRINTF
extern int vsnprintf(char *buf, size_t buflen, const char *msg, va_list args);
#endif
#if 0
#ifndef HAVE_DPRINTF
extern int dprintf(int fd, const char *format, ...);
#endif
#endif

#ifndef HAVE_STRCASECMP
extern int strcasecmp(const char *s1, const char *s2);
#endif
#ifndef HAVE_STRNCASECMP
extern int strncasecmp(const char *s1, const char *s2, size_t len);
#endif

#ifndef HAVE_LSTAT
extern int lstat(const char *fname, struct stat *s);
#endif

#if defined(WIN32) || defined(__WIN32)
#ifndef S_ISDIR
#define S_ISDIR(var)    ((var & _S_IFDIR) != 0)
#endif
#ifndef S_IRUSR
#define S_IRUSR     _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR     _S_IWRITE
#endif
#ifndef S_IRWXU
#define S_IRWXU     (_S_IREAD | _S_IWRITE)
#endif
#ifndef S_IRGRP
#define S_IRGRP     _S_IREAD
#endif
#ifndef S_IWGRP
#define S_IWGRP     _S_IWRITE
#endif
#ifndef S_IROTH
#define S_IROTH     _S_IREAD
#endif
#ifndef S_IWOTH
#define S_IWOTH     _S_IWRITE
#endif
#ifndef S_IXGRP
#define S_IXGRP     0
#endif
#ifndef S_IXOTH
#define S_IXOTH     0
#endif


#define mkdir port32_mkdir
#define rmdir port32_rmdir


#ifndef EWOULDBLOCK
#define EWOULDBLOCK             WSAEWOULDBLOCK
#endif
#ifndef EINPROGRESS
#define EINPROGRESS             WSAEINPROGRESS
#endif
#ifndef EALREADY
#define EALREADY                WSAEALREADY
#endif
#ifndef ENOTSOCK
#define ENOTSOCK                WSAENOTSOCK
#endif
#ifndef EDESTADDRREQ
#define EDESTADDRREQ            WSAEDESTADDRREQ
#endif
#ifndef EMSGSIZE
#define EMSGSIZE                WSAEMSGSIZE
#endif
#ifndef EPROTOTYPE
#define EPROTOTYPE              WSAEPROTOTYPE
#endif
#ifndef ENOPROTOOPT
#define ENOPROTOOPT             WSAENOPROTOOPT
#endif
#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT         WSAEPROTONOSUPPORT
#endif
#ifndef ESOCKTNOSUPPORT
#define ESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
#endif
#ifndef EOPNOTSUPP
#define EOPNOTSUPP              WSAEOPNOTSUPP
#endif
#ifndef EPFNOSUPPORT
#define EPFNOSUPPORT            WSAEPFNOSUPPORT
#endif
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT            WSAEAFNOSUPPORT
#endif
#ifndef EADDRINUSE
#define EADDRINUSE              WSAEADDRINUSE
#endif
#ifndef EADDRNOTAVAIL
#define EADDRNOTAVAIL           WSAEADDRNOTAVAIL
#endif
#ifndef ENETDOWN
#define ENETDOWN                WSAENETDOWN
#endif
#ifndef ENETUNREACH
#define ENETUNREACH             WSAENETUNREACH
#endif
#ifndef ENETRESET
#define ENETRESET               WSAENETRESET
#endif
#ifndef ECONNABORTED
#define ECONNABORTED            WSAECONNABORTED
#endif
#ifndef ECONNRESET
#define ECONNRESET              WSAECONNRESET
#endif
#ifndef ENOBUFS
#define ENOBUFS                 WSAENOBUFS
#endif
#ifndef EISCONN
#define EISCONN                 WSAEISCONN
#endif
#ifndef ENOTCONN
#define ENOTCONN                WSAENOTCONN
#endif
#ifndef ESHUTDOWN
#define ESHUTDOWN               WSAESHUTDOWN
#endif
#ifndef ETOOMANYREFS
#define ETOOMANYREFS            WSAETOOMANYREFS
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT               WSAETIMEDOUT
#endif
#ifndef ECONNREFUSED
#define ECONNREFUSED            WSAECONNREFUSED
#endif
#ifndef ELOOP
#define ELOOP                   WSAELOOP
#endif
#ifndef ENAMETOOLONG
#define ENAMETOOLONG            WSAENAMETOOLONG
#endif
#ifndef EHOSTDOWN
#define EHOSTDOWN               WSAEHOSTDOWN
#endif
#ifndef EHOSTUNREACH
#define EHOSTUNREACH            WSAEHOSTUNREACH
#endif
#ifndef ENOTEMPTY
#define ENOTEMPTY               WSAENOTEMPTY
#endif
#ifndef EPROCLIM
#define EPROCLIM                WSAEPROCLIM
#endif
#ifndef EUSERS
#define EUSERS                  WSAEUSERS
#endif
#ifndef EDQUOT
#define EDQUOT                  WSAEDQUOT
#endif
#ifndef ESTALE
#define ESTALE                  WSAESTALE
#endif
#ifndef EREMOTE
#define EREMOTE                 WSAEREMOTE
#endif


#ifndef ENOSUP
#define ENOSUP          EBADF
#endif


/* shutdown() constants */
#ifndef SHUT_RDWR
#define SHUT_RDWR   SD_BOTH
#endif

#ifndef SHUT_RD
#define SHUT_RD   SD_RECEIVE
#endif

#ifndef SHUT_WR
#define SHUT_WR   SD_SEND
#endif



#ifndef WAIT_CHILD
#define WAIT_CHILD   _WAIT_CHILD
#endif



#endif


#ifdef DONT_HAVE_PID_T          /* [i_a] MSVC 2003/2005 fix: */
#undef pid_t
#if defined(_INTPTR_T_DEFINED)  /* MSVC.NET */
#define pid_t     intptr_t
#else
#define pid_t     int
#endif
#endif



#ifndef HAVE_GETPID
extern pid_t getpid(void);
#endif


#ifndef HAVE__SET_ERRNO
extern void _set_errno(int errcode);
#endif





#ifndef F_OK
#define F_OK   0x00             /* defines mode for _access() / access() */
#endif
#ifndef R_OK
#define R_OK   0x04             /* defines mode for _access() / access() */
#endif
#ifndef W_OK
#define W_OK   0x02             /* defines mode for _access() / access() */
#endif
#ifndef X_OK
#define X_OK   0x00             /* defines mode for _access() / access() */
#endif



/* For flock() emulation */
#ifndef HAVE_FLOCK

#ifndef LOCK_SH
#define LOCK_SH 1
#endif
#ifndef LOCK_EX
#define LOCK_EX 2
#endif
#ifndef LOCK_NB
#define LOCK_NB 4
#endif
#ifndef LOCK_UN
#define LOCK_UN 8
#endif

extern int flock(int fd, int op);
#endif



#ifndef HAVE_GETTIMEOFDAY
struct timeval;
struct timezone;
extern int gettimeofday(struct timeval *tv, struct timezone *tz);
#endif

#ifndef HAVE_MKDIR
extern int mkdir(const char *path, int rights);
#endif

#ifndef HAVE_RMDIR
extern int rmdir(const char *path);
#endif

#ifndef HAVE_SYMLINK
extern int symlink(const char *to_path, const char *from_path);
#endif

#ifndef HAVE_LINK
extern int link(const char *to_path, const char *from_path);
#endif

#ifndef HAVE_SLEEP
extern void sleep(int seconds);
#endif

#ifndef HAVE_FTRUNCATE
extern int ftruncate(int filehandle, size_t size);
#endif

#ifndef HAVE_GETCWD
extern char *getcwd(char *buf, int max_len);
#endif


#ifdef HAVE_WINSOCK2_H
extern int WinSockErr2errno(int err);
#endif

/* return program filename derived from argv[0]; path is stripped from filename */
extern char *pavuk_get_program_name(DBGdeclvoid(void));
#define pavuk_get_program_name()    pavuk_get_program_name(DBGvoid())





typedef DLLISTTYPE dllist_t;


#define NeedFunctionPrototypes 1

extern void set_h_errno(int ecode);

#ifdef NEED_DECLARE_H_ERRNO
extern int h_errno;
#endif

#if !defined(HAVE_LONG_FILE_NAMES) && defined(__GNUC__)
#warning "This program can't run successfuly on machine without long filenames support"
#endif

#include "mode.h"

#if defined(I_FACE) && !defined(HAVE_MT)
#define _Xt_Serve       gui_loop_serve()
#define _Xt_EscLoop     gui_loop_escape()
#define _Xt_ServeLoop   gui_loop_do()
#else
#define _Xt_Serve
#define _Xt_EscLoop
#define _Xt_ServeLoop
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef O_SHORT_LIVED
#ifndef _O_SHORT_LIVED
#define O_SHORT_LIVED 0
#else
#define O_SHORT_LIVED _O_SHORT_LIVED
#endif
#endif

#include <limits.h>
#ifndef PATH_MAX
#ifdef FILENAME_MAX
#define PATH_MAX        FILENAME_MAX
#elif defined(MAX_PATH)
#define PATH_MAX        MAX_PATH
#else
#define PATH_MAX        2048
#endif
#endif

#ifndef NAME_MAX
#ifdef MAXNAMLEN
#define NAME_MAX        MAXNAMLEN
#else
#define NAME_MAX 256
#endif
#endif

/********************************************************/
/* the folowing is to workaround systems which defines  */
/* unreal limits on filenames                           */
/********************************************************/
#if defined(HAVE_LONG_FILE_NAMES) && NAME_MAX < 16
#undef NAME_MAX
#define NAME_MAX 256
#endif

#if defined(HAVE_LONG_FILE_NAMES) && NAME_MAX < 1024
#undef PATH_MAX
#define PATH_MAX 2048
#endif

#ifndef INT_MAX
#define INT_MAX         2147483647
#endif
#ifndef USHRT_MAX
#define USHRT_MAX       65535
#endif

#ifdef SOCKS

#ifdef HAVE_SOCKS_H
#include <socks.h>
#else

int SOCKSinit(char *);
#define connect         Rconnect
int Rconnect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
#define bind            Rbind
int Rbind(int sockfd, struct sockaddr *my_addr, socklen_t addrlen);
#define accept          Raccept
int Raccept(int s, struct sockaddr *addr, socklen_t * addrlen);
#define listen          Rlisten
int Rlisten(int s, int backlog);
#define select          Rselect
int Rselect(int n, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, struct timeval *timeout);
#define gethostbyname   Rgethostbyname
struct hostent *Rgethostbyname(const char *name);
#define getsockname     Rgetsockname
int Rgetsockname(int s, struct sockaddr *name, socklen_t * namelen);

#endif /* HAVE_SOCKS_H */

#endif /* SOCKS */





#ifndef TIMEVAL_TO_TIMESPEC
#define TIMEVAL_TO_TIMESPEC(tv, ts) {                   \
            (ts)->tv_sec = (tv)->tv_sec;                \
            (ts)->tv_nsec = (tv)->tv_usec * 1000; }
#endif

/* zero out a timer */
#ifndef timerclear
#define timerclear(tvp)         (tvp)->tv_sec = (tvp)->tv_usec = 0
#endif

/* is timer non-zero? */
#ifndef timerisset
#define timerisset(tvp)         ((tvp)->tv_sec || (tvp)->tv_usec)
#endif

/* add tvp and uvp and store in vvp */
#ifndef timeradd
#define timeradd(tvp, uvp, vvp)                                       \
        do {                                                          \
                (vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;        \
                (vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;     \
                if ((vvp)->tv_usec >= 1000000) {                      \
                        (vvp)->tv_sec++;                              \
                        (vvp)->tv_usec -= 1000000;                    \
                }                                                     \
        } while (0)
#endif

/* subtract uvp from tvp and store in vvp */
#ifndef timersub
#define timersub(tvp, uvp, vvp)                                         \
        do {                                                            \
                (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;          \
                (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;       \
                if ((vvp)->tv_usec < 0) {                               \
                        (vvp)->tv_sec--;                                \
                        (vvp)->tv_usec += 1000000;                      \
                }                                                       \
        } while (0)
#endif

/* compare tvp and uvp using cmp */
#ifndef timercmp
#define timercmp(tvp, uvp, cmp)                         \
        (((tvp)->tv_sec == (uvp)->tv_sec) ?             \
        ((tvp)->tv_usec cmp (uvp)->tv_usec) :           \
        ((tvp)->tv_sec cmp (uvp)->tv_sec))
#endif

/* multiply tvp by x and store in uvp */
#define timermul(tvp, uvp, x)                                           \
        do {                                                            \
                (uvp)->tv_sec = (tvp)->tv_sec * x;                      \
                (uvp)->tv_usec = (tvp)->tv_usec * x;                    \
                while((uvp)->tv_usec > 1000000) {                       \
                        (uvp)->tv_sec++;                                \
                        (uvp)->tv_usec -= 1000000;                      \
                }                                                       \
        } while(0)


/* device tvp by x.  store in tvp */
#define timerdiv2(tvp, x)                                               \
        do {                                                            \
                (tvp)->tv_sec = (tvp)->tv_sec / x;                      \
                (tvp)->tv_usec = (tvp)->tv_usec / x;                    \
        } while(0)

#ifndef timeradd2
#define timeradd2(dst, src)                           \
  timeradd(dst, src, dst)
#endif

#ifndef timersub2
#define timersub2(dst, src)                           \
  timersub(dst, src, dst)
#endif




#undef VERIFY
#undef ASSERT
#undef ASSERT_IS_ENABLED

#ifdef DEBUG
#ifndef NDEBUG
/* make sure the assert() macro does NOT resolve to a nil operation for VERIFY() here! */
#define VERIFY(expr)    assert(expr)
#define ASSERT(expr)    assert(expr)
#define ASSERT_IS_ENABLED
#else
#define VERIFY(expr)    (void)(expr)
#define ASSERT(expr) /**/
#endif
#else
#define VERIFY(expr)    (void)(expr)
#define ASSERT(expr) /**/
#endif
#include "mt.h"
#include "debugl.h"
#include "condition.h"
#include "nls.h"
#include "dllist.h"
#include "dlhash.h"
#include "http.h"
#include "dns.h"
#include "mimetype.h"

#define HASH_TBL_SIZE   233

  typedef enum
{
  SSTRAT_DO_LEVEL_ORDER,
  SSTRAT_DO_LEVEL_ORDER_INLINE_FIRST,
  SSTRAT_DO_PRE_ORDER,
  SSTRAT_DO_PRE_ORDER_INLINE_FIRST,
  SSTRAT_LAST
} strategy;

#if 0
//typedef struct
//{
//  int fd;                       /* file handle */
//  char *filename;               /* [optional] filename. NULL if not available */
//  bool_t append;                /* [optional] TRUE if file should be opened in APPEND mode */
//} fd_param;
#endif

/*
   WARNING: all integer value types which are configured through command options
            MUST be 'long' or 'unsigned long' type to prevent run-time failures
            due to options overwriting adjacent options too due to incorrect type
            size (sizeof(int) is not necessarily == sizeof(long)
 */
typedef struct
{
/*** CMDline parameters ***/
  char *default_prefix;         /*** default prefix used in filename_to_url when nondefault tree layout is used ***/
  char *info_dir;               /*** directory where are info files stored instead of regulary to -cdir ***/
  char *urls_file;              /*** urls will be read from this file ***/
  char *cookie_file;            /*** cookie file in NS format ***/
  char *auth_file;              /*** file for authentification informations ***/
  bufio *save_scn;              /*** name for scenario saving ***/
  char *scndir;                 /*** directory where scenarios are stored ***/
  char *scenario;               /*** scenario file to load ***/
  char *subdir;                 /*** subdirectory of cache_dir to focus  ***/
  char *cache_dir;              /*** dir where your local tree is located ***/
  bufio *logfile;               /*** logging file ***/
  bufio *short_logfile;         /*** newstyle log file name ***/
  bufio *time_logfile;          /*** time log file name ***/
  bufio *time_logfile4sum;      /*** time summary log file name ***/
  bool_t sdemo_mode;            /*** sdemo compaible output ***/
  bool_t noencode;              /*** don't do RFC 2396 character escaping ***/
  char *time_relative;          /*** what timings are relative to ***/
  http_proxy *transparent_proxy;     /*** list of transparent proxy servers ***/
  http_proxy *transparent_ssl_proxy; /*** list of transparent SSL proxy servers ***/
  char *remind_cmd;             /*** command for reminding changed URLS ***/
  char *sched_cmd;              /*** scheduling command ***/
  char *post_cmd;               /*** command for post downloading processing of files ***/
  char *stats_file;             /*** status file ***/

  /* [i_a] */
  bool_t ignore_RFC2616_chunk_bug;   /*** ignore IIS 5/6 erroneous RFC2616 'chunked' mode download handling ***/
  bool_t report_url_with_err;   /*** include URL in error reports ***/
  bool_t verbose;               /*** ? output messages ***/
  long progress_mode;           /*** progress reporting mode ***/
  bool_t no_disc_io;            /*** deny any disc I/O, except logging ***/
  bool_t try_all_forms;         /*** try to fetch even forms unknown to .formdata */
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  bool_t log_hammer_action;     /*** log all hammer activity too! ***/
#ifdef HAVE_MT
  long hammer_threadcount;      /*** number of threads to use for hammer replay attack ***/
#endif
  long hammer_mode;             /*** hammer mode: 0 = repeat; 1 = record & burst ***/
  long hammer_flags;            /*** hammer options within a mode ***/
  long hammer_endcount;         /*** number of times to repeat requests; negative # = infinite ***/
  double hammer_ease_time;      /*** [msec] delay between hammer requests (mode 1 and beyond) ***/
  double hammer_read_timeout;   /*** [msec] delay for hammer read action (mode 1 and beyond) ***/
  bufio *rec_act_dump_fd;       /*** filedescriptor for dumping recorded activity ***/
  dllist *rec_act_queue;        /*** queue used for recording activity for future replay ***/
  long hammer_cycle;            /* 0...hammer_endcount: counts the actual hammer repeat cycle */
#endif
  bufio *dump_cmd_fname;        /*** name for cmd saving ***/
  char *dump_cmd_dir;           /*** directory for dumps ***/
  bool_t disable_dns;           /*** do NOT perform DNS host lookups as these will probably hang/fail */
  const char **bad_content;           /*** list of content strings which signal failed pages ***/
  char *test_id;                /*** specify a test ID for logging ***/
  const char **page_suffix;
  bool_t allow_page_suffix;
  /* [/i_a] */
  bufio *dump_fd;               /*** number of filedescriptor for dumping documents ***/
  bufio *dump_urlfd;            /*** number of filedescriptor for dumping all URLs ***/
  long hash_size;               /*** hash tables size ***/
  long trans_quota;             /*** transfer quota ***/
  long file_quota;              /*** file size quota ***/
  long fs_quota;                /*** filesystem quota ***/
  long bufsize;                 /*** size of read buffer ***/
  long base_level;              /*** base tree level from which is built local file tree ***/
  long nretry;                  /*** number of retries on error ***/
  long nreget;                  /*** number of regets ***/
  long nredir;                  /*** max number of redirections ***/
  long ddays;                   /*** delay in days for document reload ***/
  long rollback;                /*** size to go back when reget-ing ***/
  long sleep;                   /*** sleeptime between transfers ***/
  long cookies_max;             /*** maximal number of cookies ***/
  long reschedh;                /*** number of hours for rescheduling ***/
  double max_time;              /*** maximal time for run ***/
  pavuk_mode mode;              /*** working mode ***/
  struct tm *time;              /*** when to schedule execution time ***/
  time_t schtime;               /*** cmdln param for scheduling time ***/
  strategy scheduling_strategy;
                                /*** url downloading scheduling strategy ***/

  char *ftp_proxy_pass;         /*** password for access to FTP proxy ***/
  char *ftp_proxy_user;         /*** username for access to FTP proxy ***/
  char *http_proxy_pass;        /*** password for access to HTTP proxy ***/
  char *http_proxy_user;        /*** username for access to HTTP proxy ***/
  long proxy_auth_scheme;       /*** authorization scheme for proxy ***/
  char *ftp_proxy;              /*** FTP proxy server ***/
  long ftp_proxy_port;
  bool_t ftp_via_http;          /*** gatewaying FTP through HTTP proxy ***/
  bool_t ftp_dirtyp;            /*** dirty FTP proxying via CONNECT request to HTTP proxy ***/
  long active_ftp_min_port;     /*** minimum port for active ftp ***/
  long active_ftp_max_port;     /*** minimum port for active ftp ***/

  char *gopher_proxy;           /*** Gopher proxy ***/
  long gopher_proxy_port;
  bool_t gopher_via_http;       /*** gatewaying Gopher trough HTTP proxy ***/
  char *name_auth;              /*** meno pouzivatela pri HTTP autentifikacii ***/
  char *passwd_auth;            /*** password for HTTP authorization ***/
  long auth_scheme;             /*** authorization scheme 1- user, 2- Basic, 3- Digest ***/

  bool_t rsleep;                /*** randomize sleep period up to -sleep ***/
  bool_t hack_add_index;        /*** add also directories of all files to queue ***/
  bool_t post_update;           /*** update in parent documents only curently downloaded URLs ***/
  bool_t fix_wuftpd;            /*** use STAT -d to test existence of directory
                                     when using LIST, because wuftpd does not raise
                                     error when listing non existing directory ***/
  bool_t retrieve_slink;        /*** retrieve symlinks like regular files ***/
  bool_t dump_resp;             /*** when -dumpfd is used, dump also HTTP response header ***/
  bool_t dump_req;              /*** when -dumpfd is used, dump also HTTP request ***/
  bool_t dump_after;            /*** when -dumpfd is used, dump document just
                                     after successfull download and after
                                     processing of HTML documents ***/
#if defined I_FACE
  bool_t xi_face;               /*** requested GUI interface ***/
#endif
  bool_t singlepage;            /*** download single HTML page with all inline objecs ***/
  bool_t unique_doc;            /*** always try to generate unique names for documents ***/
  bool_t del_after;             /*** delete transfered files after successfull transfer ***/
  bool_t use_http11;            /*** enable using of HTTP/1.1 protocol version ***/
  bool_t gen_logname;           /*** generate nummbered log names when original locked ***/
  bool_t send_if_range;         /*** send If-Range: header in HTTP request ***/
  bool_t auto_referer;          /*** send own URL as referer with starting URLs ***/
  bool_t referer;               /*** referer field for requests ***/
  bool_t persistent;            /*** use persistent connections ***/
  bool_t all_to_remote;         /*** change all links to remote and don't do any further changes to it ***/
  bool_t sel_to_local;          /*** change links, which acomplish limits, to local immediately ***/
  bool_t all_to_local;          /*** change all links to local immediately ***/
  bool_t enable_info;           /*** enable using and creating info files ***/
  bool_t enable_js;             /*** enable javascript ***/
  bool_t bgmode;                /*** run at background ***/
  bool_t store_index;           /*** store directory URLS as index files ***/
  bool_t send_from;             /*** send From: header in request ***/
  bool_t check_size;            /*** some broken HTTP servers send wrong Content-Length: header ***/
  bool_t htdig;                 /*** to dump HTTP response - used by htDig ***/
  bool_t send_cookies;          /*** send available cookies in HTTP request ***/
  bool_t recv_cookies;          /*** receive cookie infos ***/
  bool_t update_cookies;        /*** update cookie file ***/
  bool_t cookie_check_domain;   /*** check if server sets cookie for own domain ***/
  bool_t preserve_time;         /*** preserve time of remote document ***/
  bool_t preserve_perm;         /*** preserve permisions of remote document ***/
  bool_t preserve_links;        /*** preserve absolute symlinks ***/
  bool_t quiet;                 /*** ? output messages ***/
  bool_t auth_reuse_nonce;      /*** reuse HTTP Digest authorization nonce ***/
  bool_t auth_reuse_proxy_nonce;/*** reuse HTTP proxy Digest authorization nonce ***/
  bool_t show_time;             /*** show start and end time of download ? */
  bool_t remove_old;            /*** remove old documents (when not occurrs on remote site) ***/
  bool_t remove_before_store;   /*** pro: remove document before storing it ***/
  bool_t always_mdtm;           /*** always use MDTM: no cached values ***/
  bool_t progress;              /*** show retrieving progress when on console ***/
  bool_t ftp_activec;           /*** use active FTP data connection instead of passive ***/
  bool_t ftp_list;              /*** retrieve FTP directory with LIST cmd insted of NLST ***/
  bool_t ftp_html;              /*** process HTML files downloaded over FTP protocol ***/
  bool_t cache;                 /*** disallow caching of HTTP documents (on proxy cache) ***/
  bool_t rewrite_links;         /*** indikacia ci ma maju byt odkazy v HTML dokumentoch prepisovane ***/
  bool_t freget;                /*** force reget whole file when server doesn't support reget ***/
  bool_t use_enc;               /*** indikacia ci sa ma pouzivat gzip/compress kodovanie pri prenose ***/
  bool_t read_css;              /*** fetch objects refed in css ***/
  char *ftp_list_options;       /*** aditional options to FTP LIST/NLST commands ***/
  char *auth_ntlm_domain;       /*** domain name for NTLM authorization ***/
  char *auth_proxy_ntlm_domain; /*** domain name for NTLM proxy authorization ***/
  char *local_ip;               /*** address for local network interface ***/
  char *index_name;             /*** name of directory index file instead of _._.html ***/
  char *store_name;             /*** filename of document transfered with -mode singlepage ***/
  char *from;                   /*** HTTP request field From: or anonymous FTP password ***/
  char *identity;               /*** User-agent: field contents ***/
  const char **accept_lang;           /*** list of preffered languages ***/
  const char **accept_chars;          /*** list of preffered character sets ***/
  const char **cookies_disabled_domains;
                                /*** domains from which cookies are not acceptable ***/
  const char **dont_touch_url_pattern;/*** to allow preserve some URLs in the original form ***/

  cond condition;               /*** structure which contains all limiting conditions ***/
  dllist *request;              /*** list of urls entered by user ***/
  dllist *formdata;             /*** data for forms found during document tree traversing ***/
  dllist *lfnames;              /*** list of filename conversion rules ***/
  dllist *http_headers;         /*** list of additional HTTP headers ***/
  dllist *http_proxy;           /*** list of HTTP proxy servers ***/
  dllist *ftp_login_hs;         /*** list for -ftp_login_handshake ***/

  char *tr_del_chr;             /*** set of characters to delete while doing name transformation ***/
  char *tr_str_s1;              /*** strfrom in transformation ***/
  char *tr_str_s2;              /*** strto in transformation ***/
  char *tr_chr_s1;              /*** setfrom in transformation ***/
  char *tr_chr_s2;              /*** setto in transformation ***/

  double maxrate;               /*** maximal transfer rate ***/
  double minrate;               /*** minimal transfer rate ***/
  double conn_timeout;          /*** timeout for network communication: connection timeout ***/
  double read_timeout;          /*** timeout for network communication: data read I/O timeout ***/
  double write_timeout;         /*** timeout for network communication: data write I/O timeout ***/

#ifdef HAVE_MOZJS
  char *js_script_file;         /*** file which contains JavaScript script with functions declarations ***/
#endif

#ifdef HAVE_BDB_18x
  char *ns_cache_dir;           /*** directory for Netscape cache ***/
#endif
  char *moz_cache_dir;          /*** directory for Mozilla cache ***/

#ifdef HAVE_MT
  long nthr;                    /*** configured number of running threads ***/
  bool_t immessages;            /*** print messages immediatly when produced not just when it is safe ***/
#endif

#if defined(__CYGWIN__) || defined(WIN32) || defined(__WIN32)
  bool_t ie_cache;              /*** possibily load files from MSIE cache directory */
  bool_t wait_on_exit;          /*** this option is for WIN32 CLI version ***/
#endif

#ifdef HAVE_TERMIOS
  bool_t tccheck;               /*** checking of we are at foreground ***/
#endif

#ifdef HAVE_REGEX
  dllist *js_patterns;          /*** matching patterns for JS URLs ***/
  dllist *js_transform;         /*** matching patterns for JS with transform **/
  dllist *advert_res;           /*** list of RE-s for advertisement banners ***/
  bool_t remove_adv;            /*** enable / disable advertisement banners ***/

  dllist *dont_touch_url_rpattern;
  dllist *dont_touch_tag_rpattern;
                                /*** to allow preserve some URLs in the original form ***/
#endif

#ifdef HAVE_DEBUG_FEATURES
  bool_t debug;                 /*** debug mode on/off ***/
  unsigned long debug_level;    /*** debug level ***/
#endif

#ifdef USE_SSL
  long ssl_version;             /*** ssl2/ssl3/ssl23/tls1 version of ssl_client_method() ***/
  char *ssl_proxy;              /*** SSL tuneling proxy ***/
  long ssl_proxy_port;
  char *ssl_cipher_list;
  char *ssl_cert_passwd;
  bool_t unique_sslid;          /*** use unique SSL IDs with each SSL connection ***/
#ifdef USE_SSL_IMPL_OPENSSL
  char *ssl_cert_file;
  char *ssl_key_file;
  char *egd_socket;             /*** path to EGD socket ***/
#endif
#ifdef USE_SSL_IMPL_NSS
  char *nss_cert_dir;           /*** certDir for Netscape NSS ***/
  bool_t nss_accept_unknown_cert;        /*** don't care much about certificates ***/
  bool_t nss_domestic_policy;
#endif
#endif

  char *language;               /*** language for LC_MESSAGES ***/

#ifdef GETTEXT_NLS
  char *msgcatd;                /*** explicit message catalog directory ***/
#endif

#ifdef I_FACE
  char *fontname;               /*** default font used in interface ***/
  long xlogsize;                /*** max number of lines in LOG widget ***/
  bool_t log_autoscroll;        /*** autoscroll of log window ***/
  bool_t run_iface;             /*** if immediately run download after start of pavuk in GUI interface ***/
  bool_t use_prefs;             /*** store & load prefernces from ~/.pavuk_prefs file ***/

                                /*** alternative icons for GUI ***/
  char *bt_icon_cfg;
  char *bt_icon_cfg_s;
  char *bt_icon_lim;
  char *bt_icon_lim_s;
  char *bt_icon_gobg;
  char *bt_icon_gobg_s;
  char *bt_icon_rest;
  char *bt_icon_rest_s;
  char *bt_icon_cont;
  char *bt_icon_cont_s;
  char *bt_icon_stop;
  char *bt_icon_stop_s;
  char *bt_icon_brk;
  char *bt_icon_brk_s;
  char *bt_icon_exit;
  char *bt_icon_exit_s;
  char *bt_icon_mtb;
  char *bt_icon_mtb_s;

#ifdef WITH_TREE
  char *browser;                /*** command to execute your preffered browser ***/
#endif
#endif

/*** GLOBALdata ***/
  abs_addr local_ip_addr;       /*** numeric address for local network interface ***/
  time_t start_time;            /*** start time of downloading ***/
  struct timeval hr_start_time; /*** high-resolution start time of downloading ***/
  long trans_size;              /*** transfered size in session ***/
  char *path_to_home;
  char *local_host;             /*** hostname of local machine ***/
  long fail_cnt;                /*** counter for failed transfers ---> return code of pavuk ***/
  char *prg_path;               /*** path to pavuk executable == argv[0] ***/
  char *install_path;           /*** pavuk install path especialy used in win32 version ***/
  long total_cnt;               /*** total number of URLs in queue  ***/
  long process_cnt;             /*** number of already processed documents ***/
  long reject_cnt;              /*** number of rejected URLs ***/
  pavuk_mode prev_mode;         /*** previous active mode ***/
  bool_t mode_started;          /*** mode startup finisched ***/
  bool_t volatile rbreak;       /*** immediately stop transfer ***/
  bool_t volatile stop;         /*** stop after this document will be processed ***/

  dllist *urlstack;             /*** list of URLs in processing queue ***/
  dllist *urls_in_dir;          /*** list of URLs extracted from mirroring
                                     directory, for checking for nonexistent
                                     document removal ***/
  dlhash *url_hash_tbl;         /*** hash table for better performance URL lookup ***/
  dlhash *fn_hash_tbl;          /*** hash table for better performance filename lookup ***/
  dllist *last_used_proxy_node; /*** pointer to last used proxy node ***/
  long docnr;                   /*** current number of document ***/

#ifdef HAVE_MT
  time_t timestamp;
  time_t cfg_changed;           /*** timestamp for cfg struct last change ***/
  pthread_key_t currdoc_key;
  pthread_key_t herrno_key;
  pthread_key_t thrnr_key;
  pthread_key_t privcfg_key;
  mt_semaphore nrunning_sem;    /*** counting sema, keeps track of number of active threads ***/
  mt_semaphore urlstack_sem;    /*** counting sema, keeps track of the number of URLs available in the URLstack/queue ***/
  pthread_t mainthread;
  pthread_t *allthreads;
  long allthreadsnr;
#endif

#ifdef I_FACE
  bool_t done;                  /*** was done startup ? ***/
  bool_t processing;            /*** some URL is actualy in processing ***/
#endif

  long threadnr;                /* 0...hammer_threadcount: identifies the thread when accessed in priv_cfg */

  const char **mime_type_collection; /*** A collection of MIME types ***/
  struct mime_type_ext *mime_type_ext_collection; /*** A collection of (preferred) filename extensions and matching MIME types ***/
} _config_struct_t;

extern _config_struct_t cfg;

#if defined(HAVE_MT) && defined(I_FACE)

#if 0
/********************************************************************/
/* this structure contains corresponding field form _config_struct  */
/* structure. when I don't want to use mutex(es) for locking of     */
/* config structure when running multiple downloading threads, I    */
/* I have to make copy of dynamicaly created config parameters to   */
/* prevent segfaults when changing configuration from GUI           */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/* !!!!!!!!!! Not realy used, only to know which fields !!!!!!!!!!! */
/* !!!!!!!!!! are used from private copy                !!!!!!!!!!! */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/********************************************************************/
typedef struct
{
  time_t timestamp;
  char *default_prefix;
  char *info_dir;
  char *subdir;
  char *cache_dir;
  char *post_cmd;
  char *http_proxy_pass;
  char *http_proxy_user;
  char *ftp_proxy_pass;
  char *ftp_proxy_user;
  char *ftp_proxy;
  char *gopher_proxy;
  char *name_auth;
  char *passwd_auth;
  char *index_name;
  char *store_name;
  char *from;
  char *identity;
  char *auth_ntlm_domain;
  char *auth_proxy_ntlm_domain;
  char *ftp_list_options;

  char **accept_lang;
  char **accept_chars;
  char **cookies_disabled_domains;
  char **dont_touch_url_pattern;

  cond condition;
  dllist *formdata;
  dllist *lfnames;
  dllist *http_headers;
  dllist *js_patterns;
  dllist *js_transform;
  dllist *ftp_login_hs;

  char *tr_del_chr;
  char *tr_str_s1;
  char *tr_str_s2;
  char *tr_chr_s1;
  char *tr_chr_s2;

#ifdef HAVE_REGEX
  dllist *advert_res;
  dllist *js_patterns;
  dllist *js_transform;
  dllist *dont_touch_url_rpattern;
  dllist *dont_touch_tag_rpattern;
#endif

#ifdef HAVE_MOZJS
  char *js_script_file;
#endif

#ifdef HAVE_BDB_18x
  char *ns_cache_dir;
  char *moz_cache_dir;
#endif

#ifdef USE_SSL
  char *ssl_proxy;
  char *ssl_cipher_list;
  char *ssl_cert_file;
  char *ssl_key_file;
  char *ssl_cert_passwd;
  char *egd_socket;
#endif
} _config_struct_priv_t;
#endif

#define _config_struct_priv_t _config_struct_t

extern void privcfg_make_copy(_config_struct_priv_t *);
extern void privcfg_free(_config_struct_priv_t *);

#define priv_cfg (*((_config_struct_priv_t *)pthread_getspecific(cfg.privcfg_key)))
#define _MT_CFGSTAMP    cfg.cfg_changed = time(NULL)
#else
#define priv_cfg cfg
#define _MT_CFGSTAMP
#endif

typedef enum
{
  PARAM_FNAME,                  /* filename -- stored in 'bufio' struct   */
  PARAM_FD,                     /* file handle (+ optional filename) -- stored in 'bufio' struct   */
  PARAM_NUM,                    /* integer number                       */
  PARAM_BYTE_NUM,               /* integer number for memory sizes      */
  PARAM_PBOOL,                  /* positive boolean                     */
  PARAM_NBOOL,                  /* negative boolean                     */
  PARAM_STR,                    /* single string                        */
  PARAM_PASS,                   /* password string                      */
  PARAM_STRLIST,                /* comma separated list of strings      */
  PARAM_STRLIST_EX,             /* comma separated list of strings      */
  PARAM_CONN,                   /* connection - host[:port]             */
  PARAM_AUTHSCH,                /* authorization scheme - 1/2/3         */
  PARAM_MODE,                   /* operation mode - mode.c              */
  PARAM_PATH,                   /* file/dir path                        */
  PARAM_PATH_NC,                /* file/dir path - do not complete      */
  PARAM_TIME,                   /* time string - YYYY.MM.DD.HH:mm       */
  PARAM_HTMLTAG,                /* HTML tags specification              */
  PARAM_TWO_QSTR,               /* two quoted strings                   */
  PARAM_DOUBLE,                 /* double number (default values are 'long' fixed point integer values in thousandths of a unit) */
  PARAM_LFNAME,                 /* for -fnrules option                  */
  PARAM_RE,                     /* list of regular expressions          */
  PARAM_USTRAT,                 /* url strategy - -strategy           */
  PARAM_SSLVER,                 /* ssl version - ssl23/ssl2/ssl3/tls1   */
  PARAM_HTTPHDR,                /* additional HTTP header               */
  PARAM_DEBUGL,                 /* debug level - debugl.c               */
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
  PARAM_HAMMERFLAG,             /* hammerflag value - hammer_mode.c     */
#endif
  PARAM_REQUEST,                /* extended request specification       */
  PARAM_PROXY,                  /* proxy specification - host:port      */
  PARAM_TRANSPARENT,            /* proxy specification - host:port      */
  PARAM_FUNC,                   /* exec function for this param type; default_val == # of expected arguments   */
  PARAM_JSTRANS,                /* for -js_transform option             */
  PARAM_NUMLIST,                /* list of integer numbers -[ad]port    */
  PARAM_FTPHS,                  /* for FTP -ftp_login_handshake         */
  PARAM_TAGPAT,                 /* for HTML tag patterns                */
  PARAM_PORT_RANGE              /* for TCP/IP port ranges               */
} par_type_t;

/* this is to support parameters of foreign libraries (like gtk) */
#define PARAM_FOREIGN           (1 << 29)

/* this is for marking option as unsupported in current compile time      */
/* configuration. This will allow to accept unsupported option on         */
/* commandline just throwing warning instead of throwing error and exit.  */
#define PARAM_UNSUPPORTED       (1 << 30)

/* [i_a] 64-bit compiler support */
typedef union _ptr_or_long_container_t
{
  long l;
  void *p;
} ptr_or_long_container_t;

typedef struct _cfg_param
{
  /*
     Example (cobbled together from options.h):
     short_cmd:         "h"
     long_cmd:          "-prefs"
     par_entry:         "UsePreferences:"
     type:              PARAM_FUNC
     default_val:       { 0 }
     val_adr:           usage  --  &cfg.use_prefs
     mdefault_val:      { TRUE }
     mval_adr:          &cfg.condition.allow_site
     help:              gettext_nop("bla bla bla")
   */
  char *short_cmd;
  char *long_cmd;
  char *par_entry;
  par_type_t type;
  ptr_or_long_container_t default_val;
  void *val_adr;
  ptr_or_long_container_t mdefault_val;
  void *mval_adr;
  char *help;
} cfg_param_t;

extern char *get_strategy_label(strategy);
extern void usage(void);
extern void usage_short(cfg_param_t *param);
extern void show_targeted_usage(cfg_param_t *param);
extern void cfg_setup_default(void);
extern void cfg_set_all_to_default(void);
extern void cfg_setup_cmdln(int, char **);
extern void cfg_cmdln_cleanup(void);
extern int cfg_load(const char *);
extern void cfg_load_setup(void);
extern int cfg_dump(bufio **);
extern int cfg_dump_cmd(bufio **);
extern int cfg_dump_cmd_fd(bufio *);
extern void cfg_free_params(void);
extern int cfg_dump_pref(void);
extern int cfg_load_pref(void);
extern int cfg_get_num_params(cfg_param_t * param, int opt_type);

extern void pavuk_do_at_exit(void);

#define PAVUK_EXIT_OK           0       /* everything goes as expected  */
#define PAVUK_EXIT_CFG_ERR      1       /* configuration error          */
#define PAVUK_EXIT_DOC_ERR      2       /* some of documents failed     */

#endif
