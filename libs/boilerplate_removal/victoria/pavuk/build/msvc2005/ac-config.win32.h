/* ac-config.h.  Generated from ac-config.h.in by configure.  */
/* ac-config.h.in.  Generated from configure.in by autoheader.  */

/* Define to 1 if the `closedir' function returns void instead of `int'. */
#undef CLOSEDIR_VOID

/* Berkeley DB 1.8x comatibility */
#undef DB_COMPATIBILITY_API

/* Define if you want to compile with extra debug development checks */
#if defined(_DEBUG) || defined(DEBUG) /* [i_a] this is done in the MSVC2005 project: different builds available */
#undef DEBUG 
#define DEBUG 1
#else
#undef DEBUG 
#endif

/* largest data type */
#define DLLISTTYPE void *

/* path to EGD/PRNGD socket */
/* #undef EGD_SOCKET_NAME */

/* NTLM authorization support */
#define ENABLE_NTLM 1

/* getpgrp() need PID parameter */
/* #undef GETPGRP_NEED_PID */

/* gettext intl stuffs */
#undef GETTEXT_NLS

/* with GTK+ GUI */
#undef GTK_FACE

/* Define to 1 if you have the `alarm' function. */
#undef HAVE_ALARM

/* Define to 1 if you have the <arpa/inet.h> header file. */
#undef HAVE_ARPA_INET_H

/* Berkeley DB 1.8x */
#undef HAVE_BDB_18x

/* BSD REs */
/* #undef HAVE_BSD_REGEX */

/* have _nl_msg_cat_cntr in libintl */
#define HAVE_CAT_CNTR 1

/* Define to 1 if you have the `chsize' function. */
/* #undef HAVE_CHSIZE */

/* Define to 1 if you have the <ctype.h> header file. */
#define HAVE_CTYPE_H 1

/* have db185.h */
#undef HAVE_DB185_H

/* have db1/db.h */
/* #undef HAVE_DB1_H */

/* have db2/db185.h */
/* #undef HAVE_DB2_DB185_H */

/* have db3/db185.h */
/* #undef HAVE_DB3_DB185_H */

/* have db4/db185.h */
/* #undef HAVE_DB4_DB185_H */

/* Define if you want to compile with debugging options */
#define HAVE_DEBUG_FEATURES 1

/* Define to 1 if you have the <direct.h> header file. */
#define HAVE_DIRECT_H 1

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#undef HAVE_DIRENT_H

/* dprintf */
#undef HAVE_DPRINTF

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* fcntl() file locking */
#undef HAVE_FCNTL_LOCK

/* have flock() in libc */
#undef HAVE_FLOCK

/* Define to 1 if your system has a working POSIX `fnmatch' function. */
#undef HAVE_FNMATCH

/* Define to 1 if you have the `fork' function. */
#undef HAVE_FORK

/* Define to 1 if you have the `freeaddrinfo' function. */
#define HAVE_FREEADDRINFO 1

/* Define to 1 if you have the `fstatfs' function. */
#undef HAVE_FSTATFS

/* Define to 1 if you have the `fstatvfs' function. */
#undef HAVE_FSTATVFS

/* Define to 1 if you have the `ftruncate' function. */
#undef HAVE_FTRUNCATE

/* Define to 1 if you have the `gai_strerror' function. */
#undef HAVE_GAI_STRERROR

/* gcrypt support for NTLM */
/* #undef HAVE_GCRYPT */

/* Define to 1 if you have the `getaddrinfo' function. */
#define HAVE_GETADDRINFO 1

/* Define to 1 if you have the `getcwd' function. */
#define HAVE_GETCWD 1 /* MSVC2005 */

/* threadsafe gethostbyname_r function */
#undef HAVE_GETHOSTBYNAME_R

/* Define to 1 if you have the `getpid' function. */
#define HAVE_GETPID 1 /* MSVC2005 */

/* Define to 1 if you have the `gettimeofday' function. */
#undef HAVE_GETTIMEOFDAY

/* Define to 1 if you have the `getuid' function. */
#undef HAVE_GETUID

/* have tm_gmtoff inside struct tm */
#undef HAVE_GMTOFF

/* GNU REs */
/* #undef HAVE_GNU_REGEX */

/* IPv6 support */
#undef HAVE_INET6

/* Define to 1 if you have the `inet_ntop' function. */
#define HAVE_INET_NTOP 1

/* Define to 1 if you have the `inet_pton' function. */
#define HAVE_INET_PTON 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <io.h> header file. */
#define HAVE_IO_H 1

/* Define to 1 if you have the <jsapi.h> header file. */
#define HAVE_JSAPI_H 1

/* Define to 1 if you have the `db' library (-ldb). */
#undef HAVE_LIBDB

/* Define to 1 if you have the `link' function. */
#undef HAVE_LINK

/* Define to 1 if you have the `llseek' function. */
#undef HAVE_LLSEEK

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if you support file names longer than 14 characters. */
#define HAVE_LONG_FILE_NAMES 1

/* Define to 1 if you have the `lseek64' function. */
#undef HAVE_LSEEK64

/* Define to 1 if you have the `lstat' function. */
#undef HAVE_LSTAT

/* have libmcrypt */
/* #undef HAVE_MCRYPT */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mkdir' function. */
#undef HAVE_MKDIR

/* Define to 1 if you have the `mkstemp' function. */
#undef HAVE_MKSTEMP

/* with JavaScript bindings */
#define HAVE_MOZJS 1
#define XP_WIN  1    /* make sure the JavaScript library compiles in Win32 mode too! */

/* multithreading support */
#define HAVE_MT 1

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
#undef HAVE_NDIR_H

/* Define to 1 if you have the <netdb.h> header file. */
#undef HAVE_NETDB_H

/* Define to 1 if you have the <netinet/in.h> header file. */
#undef HAVE_NETINET_IN_H

/* Define if you have cygwin with new paths style */
#define HAVE_NEWSTYLE_CYGWIN 1   /* [i_a] fixup apr/2007 - native Win32 port hack to ensure the drive:/ prefix doesn't need to be fixed EVERYWHERE, leading to code chaos */

/* Define to 1 if you have the <nss.h> header file. */
/* #undef HAVE_NSS_H */

/* libcrypto from OpenSSL contains MD4 cipher */
#define HAVE_OPENSSL_MD4 1

/* Define to 1 if you have the `pclose' function. */
#undef HAVE_PCLOSE

/* PCRE REs */
/* #undef HAVE_PCRE_REGEX */

/* Define to 1 if you have the `pipe' function. */
#undef HAVE_PIPE

/* Define to 1 if you have the `popen' function. */
#undef HAVE_POPEN

/* POSIX REs */
#undef HAVE_POSIX_REGEX

/* Define to 1 if you have the <prio.h> header file. */
/* #undef HAVE_PRIO_H */

/* Define to 1 if you have the <process.h> header file. */
#define HAVE_PROCESS_H 1

/* Define to 1 if you have the `pthread_equal' function. */
#define HAVE_PTHREAD_EQUAL 1

/* Define to 1 if you have the <pthread.h> header file. */
#define HAVE_PTHREAD_H 1

/* Define to 1 if you have the `pthread_sigmask' function. */
/* #undef HAVE_PTHREAD_SIGMASK */

/* Define to 1 if you have the <pwd.h> header file. */
#undef HAVE_PWD_H

/* OpenSSL lib contains EGD/PRNGD support */
#define HAVE_RAND_EGD 1

/* REs support */
#define HAVE_REGEX 1

/* Define to 1 if you have the <regex.h> header file. */
#define HAVE_REGEX_H 1

/* Define to 1 if you have the `rmdir' function. */
#undef HAVE_RMDIR

/* Define to 1 if you have the `setenv' function. */
#undef HAVE_SETENV

/* Define to 1 if you have the `sigaction' function. */
#undef HAVE_SIGACTION

/* Define to 1 if you have the `sigaddset' function. */
#undef HAVE_SIGADDSET

/* Define to 1 if you have the `sigemptyset' function. */
#undef HAVE_SIGEMPTYSET

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if you have the `sleep' function. */
#undef HAVE_SLEEP

/* Define to 1 if you have the <smime.h> header file. */
/* #undef HAVE_SMIME_H */

/* Define to 1 if you have the `snprintf' function. */
#undef HAVE_SNPRINTF

/* have declared struct sockaddr_storage */
#define HAVE_SOCKADDR_STORAGE 1

/* <socks.h> */
/* #undef HAVE_SOCKS_H */

/* Define to 1 if you have the <ssl.h> header file. */
#define HAVE_SSL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#undef HAVE_STRCASECMP

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strncasecmp' function. */
#undef HAVE_STRNCASECMP

/* Define to 1 if you have the `symlink' function. */
#undef HAVE_SYMLINK

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_DIR_H */

/* Define to 1 if you have the <sys/file.h> header file. */
#undef HAVE_SYS_FILE_H

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#undef HAVE_SYS_IOCTL_H

/* Define to 1 if you have the <sys/mount.h> header file. */
#define HAVE_SYS_MOUNT_H 1

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_NDIR_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#undef HAVE_SYS_PARAM_H

/* Define to 1 if you have the <sys/socket.h> header file. */
#undef HAVE_SYS_SOCKET_H

/* Define to 1 if you have the <sys/statfs.h> header file. */
#define HAVE_SYS_STATFS_H 1

/* Define to 1 if you have the <sys/statvfs.h> header file. */
#undef HAVE_SYS_STATVFS_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#undef HAVE_SYS_TIME_H

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/utime.h> header file. */
/* #undef HAVE_SYS_UTIME_H */

/* Define to 1 if you have the <sys/vfs.h> header file. */
#undef HAVE_SYS_VFS_H

/* Define to 1 if you have the <sys/wait.h> header file. */
#undef HAVE_SYS_WAIT_H

/* Define if you have tcgetpgrp() */
#undef HAVE_TERMIOS

/* TRE regular expression library */
#define HAVE_TRE_REGEX 1

/* Define to 1 if the system has the type `sighandler_t'. */
#undef HAVE_TYPE_SIGHANDLER_T

/* have timezone function tzset() */
#define HAVE_TZSET 1

/* Define to 1 if you have the <unistd.h> header file. */
#undef HAVE_UNISTD_H

/* Define to 1 if you have the `unsetenv' function. */
#undef HAVE_UNSETENV

/* Define to 1 if you have the `usleep' function. */
#undef HAVE_USLEEP

/* Define to 1 if you have the `utime' function. */
#undef HAVE_UTIME

/* Define to 1 if you have the <utime.h> header file. */
#undef HAVE_UTIME_H

/* SYSV 8 REs */
/* #undef HAVE_V8_REGEX */

/* SYSV 8 RE exports regsub */
/* #undef HAVE_V8_REGSUB */

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to 1 if you have the `waitpid' function. */
#undef HAVE_WAITPID

/* Define to 1 if you have the <windows.h> header file. */
#define HAVE_WINDOWS_H 1

/* Define to 1 if you have the <winsock2.h> header file. */
#define HAVE_WINSOCK2_H 1

/* have Xmu library */
#undef HAVE_XMU

/* have libz */
#define HAVE_ZLIB 1

/* Define to 1 if you have the <zlib.h> header file. */
#define HAVE_ZLIB_H 1

/* Define to 1 if you have the `_chsize' function. */
#define HAVE__CHSIZE 1

/* Define to 1 if you have the `_mkdir' function. */
#define HAVE__MKDIR 1

/* Define to 1 if you have the `_rmdir' function. */
#define HAVE__RMDIR 1

/* Define to 1 if you have the `_set_errno' function. */
#define HAVE__SET_ERRNO 1

/* Define to 1 if you have the `_snprintf' function. */
#define HAVE__SNPRINTF 1

/* Define to 1 if you have the `_vsnprintf' function. */
#define HAVE__VSNPRINTF 1

/* Set host type */
#define HOSTTYPE "Win32"

/* Define if you want to compile with chunky/hammer mode (DoS) features. */
#define INCLUDE_CHUNKY_DoS_FEATURES 1

/* with GUI */
#undef I_FACE

/* Define to 1 if `major', `minor', and `makedev' are declared in <mkdev.h>.
   */
/* #undef MAJOR_IN_MKDEV */

/* Define to 1 if `major', `minor', and `makedev' are declared in
   <sysmacros.h>. */
/* #undef MAJOR_IN_SYSMACROS */

/* libc doesn't export h_errno variable */
/* #undef NEED_DECLARE_H_ERRNO */

/* improper alignment of NTLM structures */
#undef NTLM_UNPACKED_STRUCT

/* have new OpenSSL not old SSLeay libs */
#define OPENSSL 1

/* Name of package */
#define PACKAGE "pavuk"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "pavuk"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "pavuk 0.9.36cvs"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "pavuk"

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.9.36cvs"

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Define to 1 if sighandler_t has a 'void' return type, i.e. when RETSIGTYPE
   is defined to 'void'. */
#define RETSIGTYPE_IS_VOID 1

/* revision number of software */
#define REVISION "2007-10-13T06:31"

/* Socks proxy support */
#undef SOCKS

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#undef TIME_WITH_SYS_TIME

/* want SSL support */
#define USE_SSL 1

/* use Mozilla NSS3 for SSL support */
/* #undef USE_SSL_IMPL_NSS */

/* use OpenSSL for SSL support */
#define USE_SSL_IMPL_OPENSSL 1

/* Version number of package */
#define VERSION "0.9.36cvs"

/* have new OpenSSL with TLS1 support */
#define WITH_SSL_TLS1 1

/* with preview dialog for HTML tree */
#undef WITH_TREE

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* Define to 1 if the X Window System is missing or not being used. */
#define X_DISPLAY_MISSING 1

/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef gid_t */

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `int' if <sys/types.h> does not define. */
/* MSVC hack: */
/* #undef pid_t */
#define DONT_HAVE_PID_T

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to `int' if <sys/types.h> does not define. */
#define socklen_t int

/* Define to `long' if <sys/types.h> does not define. */
#define ssize_t long

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef uid_t */

