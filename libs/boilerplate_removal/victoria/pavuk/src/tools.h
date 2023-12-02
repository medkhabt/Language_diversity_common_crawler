/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _pv_tools_h_
#define _pv_tools_h_

#include "dllist.h"

typedef unsigned char bool_t;



#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define TL_MAX(x,y)             ((x) > (y) ? (x) : (y))
#define TL_MIN(x,y)             ((x) < (y) ? (x) : (y))
#define TL_BETWEEN(c,a,b)       ((c) >= (a) && (c) <= (b))

#define NUM_ELEM(x)     ((unsigned int) (sizeof(x) / sizeof(x[0])))


/*
   special debugging routines; used in conjunction with memalloc debugging libs
   (e.g. crtdbg debug support on Win32/MSVC)
 */
#if defined(DEBUG)

extern void *tld_malloc(const char *sourcefile, int sourceline, size_t len);
extern void *tld_realloc(const char *sourcefile, int sourceline, void *ptr, size_t len);
extern void tld_free(const char *sourcefile, int sourceline, void *ptr);
extern char *tld_strdup(const char *sourcefile, int sourceline, const char *s);
extern char *tld_strndup(const char *sourcefile, int sourceline, const char *s, int l);

#define _malloc(s)                      tld_malloc(__FILE__, __LINE__, s)
#define _realloc(p, s)                  tld_realloc(__FILE__, __LINE__, p, s)
#define _free(ptr)                      do { if (ptr) { tld_free(__FILE__, __LINE__, (void *)(ptr)); ptr = NULL; } } while (0)

#define tl_strdup(s)                    tld_strdup(__FILE__, __LINE__, s)
#define tl_strndup(s, l)                tld_strndup(__FILE__, __LINE__, s, l)

#else

#define tld_malloc(len)                 _malloc(len)
#define tld_realloc(ptr, len)           _realloc(ptr, len)
#define tld_free(ptr)                   _free(ptr)
#define tld_strdup(s)                   tl_strdup(s)
#define tld_strndup(s, l)               tl_strndup(s, l)


extern void *_malloc(size_t len);
extern void *_realloc(void *ptr, size_t len);

#define _free(ptr)                              do { if (ptr) { free((void *)(ptr)); ptr = NULL; } } while (0)

extern char *tl_strdup(const char *);
extern char *tl_strndup(const char *, int);

#endif /* DEBUG */


extern char **tl_str_split(const char *, const char *);
extern dllist *tl_numlist_split(char *, char *);
extern void tl_strv_free(char **);
extern int tl_strv_find(char **, char *);
extern int tl_strv_length(char **);
extern void tl_strv_sort(char **);
extern char *tl_str_append(DBGdecl(char *str1), const char *str2);
#define tl_str_append(str1, str2)        tl_str_append(DBGvars(str1), str2)
extern char *tl_str_nappend(char *str1, const char *str2, int n);
extern char *tl_str_concat(DBGdecl(char *str1), ...);
extern char *tl_str_printf(char *str1, const char *fmt, ...);

extern char *tl_data_concat_data(int *tlen, char *tdata, int len, const char *data);
extern char *tl_data_concat_str(DBGdecl(int *len), char *data, ...);

extern void tl_msleep(unsigned int ms, bool_t forced);
extern void tl_sleep(unsigned int);

extern char *tl_adjust_filename(char *);
extern int tl_filename_needs_adjust(const char *path);
extern int tl_is_dirname(const char *path);
extern const char *tl_get_extension(const char *);
extern char *tl_get_basename(char *);

extern int tl_flock(int *fd, const char *filename, int opt, int b_lock);

extern int tl_mkstemp(char *);

extern char *tl_load_text_file(const char *filename);
extern int tl_save_text_file(const char *filename, const char *content, int length);

extern char *tl_hexdump_content(char *dst, size_t dstsize, const void *content, size_t content_length);
extern char *tl_asciidump_content(char *dst, size_t dstsize, const char *content, size_t content_length);

extern void strip_nl(char *);
extern void omit_chars(char *str, const char *chars);
extern char *upperstr(char *);
extern char *lowerstr(char *);
extern int makealldirs(const char *);
extern char *get_abs_file_path(DBGdecl(const char *));
#define get_abs_file_path(p)        get_abs_file_path(DBGvars(p))
extern char *get_abs_file_path_oss(DBGdecl(const char *));
#define get_abs_file_path_oss(p)    get_abs_file_path_oss(DBGvars(p))
extern char *get_relative_path(char *, char *);
extern bool_t is_in_list(const char *, const char **);
extern bool_t is_in_dllist(const char *, dllist *);
extern bool_t is_in_pattern_list(const char *, const char **);
extern bool_t is_in_pattern_dllist(const char *, dllist *);
extern char *get_1qstr(const char *str);
extern char *strtokc_r(char *str, int chr, char **save);
extern const char *strfindnchr(const char *str, int chr, int n);
extern const char *strrfindnchr(const char *str, int chr, int n);
extern int str_cnt_chr(const char *s, int c);
extern bool_t file_is_html(const char *fn);
extern bool_t ext_is_html(const char *fn);
extern bool_t ext_is_gzipped_file(const char *fn);      /* [i_a] */
extern long int _atoi(const char *);
extern long int _atoi_with_size(const char *str, bool_t decimal_or_binary_sizes);
extern double _atof(const char *);
extern char *_strtrchr(char *str, int cfrom, int cto);
extern void xprintf(unsigned int, const char *, ...);
extern void xvaprintf(unsigned int, const char *, va_list *);
extern void xdebug(unsigned int, const char *, const char *, ...);
extern void xvadebug(unsigned int, const char *, va_list *);
#if defined(DEBUG)
#define xperror(s)              _xperror(__FILE__, __LINE__, s)
#define xperror1(s, a)          _xperror1(__FILE__, __LINE__, s, a)
#define xperror2(s, a1, a2)     _xperror2(__FILE__, __LINE__, s, a1, a2)
#define xherror(s)              _xherror(__FILE__, __LINE__, s)
extern void _xperror(const char *sourcefilename, int lineno, const char *msg);
extern void _xperror1(const char *sourcefilename, int lineno, const char *msg, const char *arg1);
extern void _xperror2(const char *sourcefilename, int lineno, const char *msg, const char *arg1, const char *arg2);
extern void _xherror(const char *sourcefilename, int lineno, const char *msg);
extern void xvherror(const char *sourcefilename, int lineno, const char *filename, const char *str, ...);
extern void xvperror(const char *sourcefilename, int lineno, const char *filename, const char *str, ...);
#else
extern void xperror(const char *);
extern void xperror1(const char *msg, const char *arg1);
extern void xperror2(const char *msg, const char *arg1, const char *arg2);
extern void xherror(const char *);
extern void xvherror(const char *filename, const char *str, ...);
extern void xvperror(const char *filename, const char *str, ...);
#endif
extern void print_progress_style(int where, const char *special_msg);   /* 0: regular; 1..N: hammer sections */
extern int unlink_recursive(char *);
extern unsigned int hash_func(const char *str, int num);
extern int str_is_in_list(int casesensitive, const char *str, ...);
extern int copy_fd_to_file(int, char *);
extern char *escape_str(const char *str, const char *unsafe);
extern char *tl_mktempfname(const char *fn, const char *prefix);


#ifdef HAVE_MT
extern RETSIGTYPE pavuk_sigintthr(int nr);
extern RETSIGTYPE pavuk_sigquitthr(int nr);
#endif
extern RETSIGTYPE pavuk_intsig(int signum);
extern RETSIGTYPE pavuk_quitsig(int signum);
#ifdef SIGXFSZ
extern RETSIGTYPE pavuk_filesizesig(int signum);
extern RETSIGTYPE pavuk_sigfilesizethr(int nr);
#endif
extern void tl_signal(int signum, sighandler_t handler);



#if (defined(WIN32) || defined(__WIN32))
extern char *cvt_win32_to_unix_path(const char *filepath, bool_t keep_relative);
extern char *cvt_unix_to_win32_path(DBGdecl(const char *));
#define cvt_unix_to_win32_path(p)       cvt_unix_to_win32_path(DBGvars(p))
extern int tl_win32_system(const char *);
#define tl_system(cmd)  tl_win32_system(cmd)
#else
#define tl_system(cmd)  system(cmd)
#endif

#ifndef HAVE_SETENV
extern int tl_setenv(const char *, const char *, int);
#define setenv          tl_setenv
#define unsetenv(var)   tl_setenv(var, "", 1)
#elif !defined(HAVE_UNSETENV)
#define unsetenv(var)   setenv(var, "", 1)
#endif

/*
 * As the Linux flock(2) manpage says: flock() does not work across NFS
 * shares, so we _prefer_ to use fcntl() which _does_, as pavuk may
 * be storing spidered files on an NFS-shared filesystem!
 *
 * Hence we check for both, then prioritize in the source code here.
 * (See also 'configure.in')
 */
#if defined(HAVE_FCNTL_LOCK)

extern int tl_funlock(int);

#define _flock(fd, fname, opt, blk)     tl_flock(&fd, fname, opt, blk)
#define _funlock(fd)                    tl_funlock(fd)

#elif defined(HAVE_FLOCK) || (defined(WIN32) || defined(_WIN32))

#if defined(HAVE_SYS_FILE_H)
#include <sys/file.h>
#endif

#define _flock(fd, fname, opt, blk)     tl_flock(&fd, fname, opt, blk)
#define _funlock(fd)                    flock(fd, LOCK_UN)

#else

extern int report_unsup_locking(void);

#define _flock(fd, fname, opt, blk)     report_unsup_locking()
#define _funlock(fd)                    report_unsup_locking()

#endif

#define new_string      tl_strdup
#define new_n_string    tl_strndup



#define tl_ascii_isalnum(c) \
        (TL_BETWEEN((c),'a','z') || TL_BETWEEN((c),'A','Z') || \
         TL_BETWEEN((c),'0','9'))

#define tl_ascii_isalpha(c) \
        (TL_BETWEEN((c),'a','z') || TL_BETWEEN((c),'A','Z'))

#define tl_ascii_isascii(c) \
        (TL_BETWEEN((c),0,127))

#define tl_ascii_isblank(c) \
        (((c) == ' ') || ((c) == '\t'))

#define tl_ascii_iscntrl(c) \
        (TL_BETWEEN((c),0,31) || ((c) == 127))

#define tl_ascii_isdigit(c) \
        (TL_BETWEEN((c),'0','9'))

#define tl_ascii_isgraph(c) \
        (TL_BETWEEN((c),33,126))

#define tl_ascii_islower(c) \
        (TL_BETWEEN((c),'a','z'))

#define tl_ascii_isprint(c) \
        (TL_BETWEEN((c),32,126))

#define tl_ascii_ispunct(c) \
        (TL_BETWEEN((c),33,47) || TL_BETWEEN((c),58,64) || \
         TL_BETWEEN((c),91,96) || TL_BETWEEN((c),123,126))

#define tl_ascii_isspace(c) \
        (((c) == ' ') || ((c) == '\f') || ((c) == '\n') || ((c) == '\r') || \
         ((c) == '\t') || ((c) == '\v'))

#define tl_ascii_isupper(c) \
        (TL_BETWEEN((c),'A','Z'))

#define tl_ascii_isxdigit(c) \
        (TL_BETWEEN((c),'0','9') || TL_BETWEEN((c),'a','f') || \
         TL_BETWEEN((c),'A','F'))

#define tl_ascii_tolower(c) \
        (tl_ascii_isupper(c) ? ('a' + ((c) - 'A')) : (c))

#define tl_ascii_toupper(c) \
        (tl_ascii_islower(c) ? ('A' + ((c) - 'a')) : (c))


/* return TRUE if last character in string str matches c */
#define tl_strendchr(str, c) \
        (((str) && *(str)) \
         ? (str)[strlen(str) - 1] == (c) \
         : FALSE)


/* for hexadicimal encoding */
#define HEXASC2HEXNR(x)                                         \
        (TL_BETWEEN(x,'0','9') ? ((x) - '0') :                  \
         TL_BETWEEN(x,'a','f') ? ((x) - 'a' + 10) :             \
         ((x) - 'A' + 10))

#define HEX2CHAR(str)                                           \
        ((HEXASC2HEXNR((str)[0]) << 4)                          \
         + HEXASC2HEXNR((str)[1]))


#endif
