/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _debugl_h_
#define _debugl_h_

#define DLV1     (1U << 0)
#define DLV2     (1U << 1)
#define DLV3     (1U << 2)
#define DLV4     (1U << 3)
#define DLV5     (1U << 4)
#define DLV6     (1U << 5)
#define DLV7     (1U << 6)
#define DLV8     (1U << 7)
#define DLV9     (1U << 8)
#define DLV10    (1U << 9)
#define DLV11    (1U << 10)
#define DLV12    (1U << 11)
#define DLV13    (1U << 12)
#define DLV14    (1U << 13)
#define DLV15    (1U << 14)
#define DLV16    (1U << 15)
#define DLV17    (1U << 16)
#define DLV18    (1U << 17)
#define DLV19    (1U << 18)
#define DLV20    (1U << 19)
#define DLV21    (1U << 20)
#define DLV22    (1U << 21)
#define DLV23    (1U << 22)
#define DLV24    (1U << 23)
#define DLV25    (1U << 24)
#define DLV26    (1U << 25)
#define DLV27    (1U << 26)
#define DLV28    (1U << 27)
#define DLV29    (1U << 28)
#define DLV30    (1U << 29)
#define DLV31    (1U << 30)
#define DLV32    (1U << 31)


#define DEBUG_HTML_BITMASK        DLV1  
#define DEBUG_PROTOS_BITMASK      DLV2  
#define DEBUG_PROTOC_BITMASK      DLV3  
#define DEBUG_PROCS_BITMASK       DLV4  
#define DEBUG_PROCE_BITMASK       DLV4  
#define DEBUG_LOCKS_BITMASK       DLV5  
#define DEBUG_NET_BITMASK         DLV6  
#define DEBUG_MISC_BITMASK        DLV7  
#define DEBUG_USER_BITMASK        DLV8  
#define DEBUG_MTLOCK_BITMASK      DLV9  
#define DEBUG_MTTHR_BITMASK       DLV10 
#define DEBUG_PROTOD_BITMASK      DLV11 
#define DEBUG_LIMITS_BITMASK      DLV12 
#define DEBUG_SSL_BITMASK         DLV13 
#define DEBUG_HAMMER_BITMASK      DLV14 
#define DEBUG_TRACE_BITMASK       DLV15 
#define DEBUG_DEVEL_BITMASK       DLV16 
#define DEBUG_BUFIO_BITMASK       DLV17 
#define DEBUG_COOKIE_BITMASK      DLV18 
#define DEBUG_HTMLFORM_BITMASK    DLV19 
#define DEBUG_RULES_BITMASK       DLV20 

#ifdef HAVE_DEBUG_FEATURES
#ifdef __GNUC__
/*** for HTML debug messages ***/
#define DEBUG_HTML(s...)        if(cfg.debug && (DLV1  & cfg.debug_level)) xdebug(DLV1 , "[HTML]    ", ## s)
/*** for server responses ***/
#define DEBUG_PROTOS(s...)      if(cfg.debug && (DLV2  & cfg.debug_level)) xdebug(DLV2 , "[PROTOS]  ", ## s)
/*** for client requests ***/
#define DEBUG_PROTOC(s...)      if(cfg.debug && (DLV3  & cfg.debug_level)) xdebug(DLV3 , "[PROTOC]  ", ## s)
/*** function start message ***/
#define DEBUG_PROCS(s)          if(cfg.debug && (DLV4  & cfg.debug_level)) xdebug(DLV4 , "[PROCS]   ", "calling - %s\n", s)
/*** function exit message ***/
#define DEBUG_PROCE(s)          if(cfg.debug && (DLV4  & cfg.debug_level)) xdebug(DLV4 , "[PROCE]   ", "exiting - %s\n", s)
/*** locking ***/
#define DEBUG_LOCKS(s...)       if(cfg.debug && (DLV5  & cfg.debug_level)) xdebug(DLV5 , "[LOCKS]   ", ## s)
/*** network stuff ***/
#define DEBUG_NET(s...)         if(cfg.debug && (DLV6  & cfg.debug_level)) xdebug(DLV6 , "[NET]     ", ## s)
/*** some misc debug messages ***/
#define DEBUG_MISC(s...)        if(cfg.debug && (DLV7  & cfg.debug_level)) xdebug(DLV7 , "[MISC]    ", ## s)
/*** user level infos ***/
#define DEBUG_USER(s...)        if(cfg.debug && (DLV8  & cfg.debug_level)) xdebug(DLV8 , "[USER]    ", ## s)
/*** multithreading ***/
#define DEBUG_MTLOCK(s...)      if(cfg.debug && (DLV9  & cfg.debug_level)) xdebug(DLV9 , "[MTLOCK]  ", ## s)
#define DEBUG_MTTHR(s...)       if(cfg.debug && (DLV10 & cfg.debug_level)) xdebug(DLV10, "[MTTHR]   ", ## s)
#define DEBUG_PROTOD(s...)      if(cfg.debug && (DLV11 & cfg.debug_level)) xdebug(DLV11, "[PROTOD]  ", ## s)
#define DEBUG_LIMITS(s...)      if(cfg.debug && (DLV12 & cfg.debug_level)) xdebug(DLV12, "[LIMITS]  ", ## s)
#define DEBUG_SSL(s...)         if(cfg.debug && (DLV13 & cfg.debug_level)) xdebug(DLV13, "[SSL]     ", ## s)
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
#define DEBUG_HAMMER(s...)      if(cfg.debug && (DLV14 & cfg.debug_level)) xdebug(DLV14, "[HAMMER]  ", ## s)
#endif
/*** detailed execution analysis ***/
#define DEBUG_TRACE(s...)       if(cfg.debug && (DLV15 & cfg.debug_level)) xdebug(DLV15, "[TRACE]   ", ## s)
#define DEBUG_DEVEL(s...)       if(cfg.debug && (DLV16 & cfg.debug_level)) xdebug(DLV16, "[DEV]     ", ## s)
#define DEBUG_BUFIO(s...)       if(cfg.debug && (DLV17 & cfg.debug_level)) xdebug(DLV17, "[BUFIO]   ", ## s)
#define DEBUG_COOKIE(s...)      if(cfg.debug && (DLV18 & cfg.debug_level)) xdebug(DLV18, "[COOKIE]  ", ## s)
#define DEBUG_HTMLFORM(s...)    if(cfg.debug && (DLV19 & cfg.debug_level)) xdebug(DLV19, "[HTMLFORM]", ## s)
#define DEBUG_RULES(s...)       if(cfg.debug && (DLV20 & cfg.debug_level)) xdebug(DLV20, "[RULES]   ", ## s)
#else
extern void DEBUG_HTML(char *, ...);
extern void DEBUG_PROTOS(char *, ...);
extern void DEBUG_PROTOC(char *, ...);
extern void DEBUG_PROCS(char *);
extern void DEBUG_PROCE(char *);
extern void DEBUG_LOCKS(char *, ...);
extern void DEBUG_NET(char *, ...);
extern void DEBUG_MISC(char *, ...);
extern void DEBUG_USER(char *, ...);
extern void DEBUG_MTLOCK(char *, ...);
extern void DEBUG_MTTHR(char *, ...);
extern void DEBUG_PROTOD(char *, ...);
extern void DEBUG_LIMITS(char *, ...);
extern void DEBUG_SSL(char *, ...);
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
extern void DEBUG_HAMMER(char *, ...);
#endif
extern void DEBUG_TRACE(char *, ...);
extern void DEBUG_DEVEL(char *, ...);
extern void DEBUG_BUFIO(char *, ...);
extern void DEBUG_COOKIE(char *, ...);
extern void DEBUG_HTMLFORM(char *, ...);
extern void DEBUG_RULES(char *, ...);
#endif

typedef struct
{
  unsigned long id;
  char *option;
  char *label;
} debug_level_type;

extern debug_level_type cfg_debug_levels[];

extern int debug_level_parse(const char *source_str, unsigned long *dst);
extern char *debug_level_construct(unsigned long level);

#else /* !HAVE_DEBUG_FEATURES */
#ifdef __GNUC__
#define DEBUG_HTML(s...)
#define DEBUG_PROTOS(s...)
#define DEBUG_PROTOC(s...)
#define DEBUG_URL(s...)
#define DEBUG_PROCS(s...)
#define DEBUG_PROCE(s...)
#define DEBUG_LOCKS(s...)
#define DEBUG_NET(s...)
#define DEBUG_MISC(s...)
#define DEBUG_USER(s...)
#define DEBUG_MTLOCK(s...)
#define DEBUG_MTTHR(s...)
#define DEBUG_PROTOD(s...)
#define DEBUG_LIMITS(s...)
#define DEBUG_SSL(s...)
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
#define DEBUG_HAMMER(s...)
#endif
#define DEBUG_TRACE(s...)
#define DEBUG_DEVEL(s...)
#define DEBUG_BUFIO(s...)
#define DEBUG_COOKIE(s...)
#define DEBUG_HTMLFORM(s...)
#define DEBUG_RULES(s...)
#else
/*
   [i_a] allow the compiler to discard the arguments, as they'll now be located in a line like

   (void)("msg", arg1, arg2);

   which will be discarded by a smart compiler. UNLESS there are function calls in the arg list,
   of course: these functions may exhibit side effects. Nevertheless, this is the best we can do
   for Mr. Compiler
 */
#define DEBUG_HTML                  (void)
#define DEBUG_PROTOS                (void)
#define DEBUG_PROTOC                (void)
#define DEBUG_URL                   (void)
#define DEBUG_PROCS                 (void)
#define DEBUG_PROCE                 (void)
#define DEBUG_LOCKS                 (void)
#define DEBUG_NET                   (void)
#define DEBUG_MISC                  (void)
#define DEBUG_USER                  (void)
#define DEBUG_MTLOCK                (void)
#define DEBUG_MTTHR                 (void)
#define DEBUG_PROTOD                (void)
#define DEBUG_LIMITS                (void)
#define DEBUG_SSL                   (void)
#ifdef INCLUDE_CHUNKY_DoS_FEATURES
#define DEBUG_HAMMER                (void)
#endif
#define DEBUG_TRACE                 (void)
#define DEBUG_DEVEL                 (void)
#define DEBUG_BUFIO                 (void)
#define DEBUG_COOKIE                (void)
#define DEBUG_HTMLFORM              (void)
#define DEBUG_RULES                 (void)
#endif
#endif

#endif
