/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _have_mt_h_
#define _have_mt_h_

#ifdef HAVE_MT

#define MT_STACK_SIZE 256000

extern pthread_key_t _mt_key_main_thread;
#define MT_IS_MAIN_THREAD() (pthread_getspecific(_mt_key_main_thread))

extern pthread_mutex_t _mt_urlstack_lock;
extern pthread_mutex_t _mt_urlhash_lock;
extern pthread_mutex_t _mt_filehash_lock;
extern pthread_mutex_t _mt_cookies_lock;
extern pthread_mutex_t _mt_authinfo_lock;
extern pthread_mutex_t _mt_dns_lock;
extern pthread_mutex_t _mt_dcnt_lock;
extern pthread_mutex_t _mt_time_lock;
extern pthread_mutex_t _mt_ghbn_lock;
extern pthread_mutex_t _mt_getlfname_lock;
extern pthread_mutex_t _mt_proxy_lock;
extern pthread_mutex_t _mt_output_lock;
extern pthread_mutex_t _mt_dirr_lock;
extern pthread_mutex_t _mt_tcnt_lock;
extern pthread_mutex_t _mt_gtktree_lock;
extern pthread_mutex_t _mt_gtkstatus_lock;
extern pthread_mutex_t _mt_gtklog_lock;
extern pthread_mutex_t _mt_rejcnt_lock;
extern pthread_mutex_t _mt_failcnt_lock;
extern pthread_mutex_t _mt_gcfg_lock;
extern pthread_mutex_t _mt_nscache_lock;
extern pthread_mutex_t _mt_robots_lock;
extern pthread_mutex_t _mt_recact_lock;
extern pthread_mutex_t _mt_taghash_lock;
extern pthread_mutex_t _mt_jsres_lock;
extern pthread_mutex_t _mt_inet_ntoa_lock;
extern pthread_mutex_t _mt_mozjs_lock;
extern pthread_mutex_t _mt_ssl_map_lock;

#define LOCK_CFG_URLSTACK       mt_pthread_mutex_lock(&_mt_urlstack_lock, "urlstack", __LINE__, __FILE__)
#define UNLOCK_CFG_URLSTACK     mt_pthread_mutex_unlock(&_mt_urlstack_lock, "urlstack", __LINE__, __FILE__)
#define LOCK_CFG_URLHASH        mt_pthread_mutex_lock(&_mt_urlhash_lock, "urlhash", __LINE__, __FILE__)
#define UNLOCK_CFG_URLHASH      mt_pthread_mutex_unlock(&_mt_urlhash_lock, "urlhash", __LINE__, __FILE__)
#define LOCK_CFG_FILEHASH       mt_pthread_mutex_lock(&_mt_filehash_lock, "filehash", __LINE__, __FILE__)
#define UNLOCK_CFG_FILEHASH     mt_pthread_mutex_unlock(&_mt_filehash_lock, "filehash", __LINE__, __FILE__)
#define LOCK_COOKIES            mt_pthread_mutex_lock(&_mt_cookies_lock, "cookies", __LINE__, __FILE__)
#define UNLOCK_COOKIES          mt_pthread_mutex_unlock(&_mt_cookies_lock, "cookies", __LINE__, __FILE__)
#define LOCK_AUTHINFO           mt_pthread_mutex_lock(&_mt_authinfo_lock, "authinfo", __LINE__, __FILE__)
#define UNLOCK_AUTHINFO         mt_pthread_mutex_unlock(&_mt_authinfo_lock, "authinfo", __LINE__, __FILE__)
#define LOCK_GETLFNAME          mt_pthread_mutex_lock(&_mt_getlfname_lock, "getlfname", __LINE__, __FILE__)
#define UNLOCK_GETLFNAME        mt_pthread_mutex_unlock(&_mt_getlfname_lock, "getlfname", __LINE__, __FILE__)
#define LOCK_DNS                mt_pthread_mutex_lock(&_mt_dns_lock, "dns", __LINE__, __FILE__)
#define UNLOCK_DNS              mt_pthread_mutex_unlock(&_mt_dns_lock, "dns", __LINE__, __FILE__)
#define LOCK_SSL_MAP            mt_pthread_mutex_lock(&_mt_ssl_map_lock, "ssl_map", __LINE__, __FILE__)
#define UNLOCK_SSL_MAP          mt_pthread_mutex_unlock(&_mt_ssl_map_lock, "ssl_map", __LINE__, __FILE__)
#define LOCK_DCNT               mt_pthread_mutex_lock(&_mt_dcnt_lock, "dcnt", __LINE__, __FILE__)
#define UNLOCK_DCNT             mt_pthread_mutex_unlock(&_mt_dcnt_lock, "dcnt", __LINE__, __FILE__)
#define LOCK_TIME               mt_pthread_mutex_lock(&_mt_time_lock, "time", __LINE__, __FILE__)
#define UNLOCK_TIME             mt_pthread_mutex_unlock(&_mt_time_lock, "time", __LINE__, __FILE__)
#define LOCK_GHBN               mt_pthread_mutex_lock(&_mt_ghbn_lock, "ghbn", __LINE__, __FILE__)
#define UNLOCK_GHBN             mt_pthread_mutex_unlock(&_mt_ghbn_lock, "ghbn", __LINE__, __FILE__)
#define LOCK_OUTPUT             mt_pthread_mutex_lock(&_mt_output_lock, "output", __LINE__, __FILE__)
#define UNLOCK_OUTPUT           mt_pthread_mutex_unlock(&_mt_output_lock, "output", __LINE__, __FILE__)
#define LOCK_PROXY              mt_pthread_mutex_lock(&_mt_proxy_lock, "proxy", __LINE__, __FILE__)
#define UNLOCK_PROXY            mt_pthread_mutex_unlock(&_mt_proxy_lock, "proxy", __LINE__, __FILE__)
#define LOCK_DIRR               mt_pthread_mutex_lock(&_mt_dirr_lock, "dirr", __LINE__, __FILE__)
#define UNLOCK_DIRR             mt_pthread_mutex_unlock(&_mt_dirr_lock, "dirr", __LINE__, __FILE__)
#define LOCK_TCNT               mt_pthread_mutex_lock(&_mt_tcnt_lock, "tcnt", __LINE__, __FILE__)
#define UNLOCK_TCNT             mt_pthread_mutex_unlock(&_mt_tcnt_lock, "tcnt", __LINE__, __FILE__)
#define LOCK_GTKTREE            mt_pthread_mutex_lock(&_mt_gtktree_lock, "gtktree", __LINE__, __FILE__)
#define UNLOCK_GTKTREE          mt_pthread_mutex_unlock(&_mt_gtktree_lock, "gtktree", __LINE__, __FILE__)
#define LOCK_GTKSTATUS          mt_pthread_mutex_lock(&_mt_gtkstatus_lock, "gtkstatus", __LINE__, __FILE__)
#define UNLOCK_GTKSTATUS        mt_pthread_mutex_unlock(&_mt_gtkstatus_lock, "gtkstatus", __LINE__, __FILE__)
#define LOCK_GTKLOG             mt_pthread_mutex_lock(&_mt_gtklog_lock, "gtklog", __LINE__, __FILE__)
#define UNLOCK_GTKLOG           mt_pthread_mutex_unlock(&_mt_gtklog_lock, "gtklog", __LINE__, __FILE__)
#define LOCK_REJCNT             mt_pthread_mutex_lock(&_mt_rejcnt_lock, "rejcnt", __LINE__, __FILE__)
#define UNLOCK_REJCNT           mt_pthread_mutex_unlock(&_mt_rejcnt_lock, "rejcnt", __LINE__, __FILE__)
#define LOCK_FAILCNT            mt_pthread_mutex_lock(&_mt_failcnt_lock, "failcnt", __LINE__, __FILE__)
#define UNLOCK_FAILCNT          mt_pthread_mutex_unlock(&_mt_failcnt_lock, "failcnt", __LINE__, __FILE__)
#define LOCK_URL(lurl)          mt_pthread_mutex_lock(&(lurl)->lock, "url", __LINE__, __FILE__)
#define UNLOCK_URL(lurl)        mt_pthread_mutex_unlock(&(lurl)->lock, "url", __LINE__, __FILE__)
#define LOCK_GCFG               mt_pthread_mutex_lock(&_mt_gcfg_lock, "gcfg", __LINE__, __FILE__)
#define UNLOCK_GCFG             mt_pthread_mutex_unlock(&_mt_gcfg_lock, "gcfg", __LINE__, __FILE__)
#define LOCK_NSCACHE            mt_pthread_mutex_lock(&_mt_nscache_lock, "nscache", __LINE__, __FILE__)
#define UNLOCK_NSCACHE          mt_pthread_mutex_unlock(&_mt_nscache_lock, "nscache", __LINE__, __FILE__)
#define LOCK_ROBOTS             mt_pthread_mutex_lock(&_mt_robots_lock, "robots", __LINE__, __FILE__)
#define UNLOCK_ROBOTS           mt_pthread_mutex_unlock(&_mt_robots_lock, "robots", __LINE__, __FILE__)
#define LOCK_RECACT             mt_pthread_mutex_lock(&_mt_recact_lock, "rec_act", __LINE__, __FILE__)
#define UNLOCK_RECACT           mt_pthread_mutex_unlock(&_mt_recact_lock, "rec_act", __LINE__, __FILE__)
#define LOCK_TAG_HASH           mt_pthread_mutex_lock(&_mt_taghash_lock, "tag_hash", __LINE__, __FILE__)
#define UNLOCK_TAG_HASH         mt_pthread_mutex_unlock(&_mt_taghash_lock, "tag_hash", __LINE__, __FILE__)
#define LOCK_INETNTOA           mt_pthread_mutex_lock(&_mt_inet_ntoa_lock, "inet_ntoa", __LINE__, __FILE__)
#define UNLOCK_INETNTOA         mt_pthread_mutex_unlock(&_mt_inet_ntoa_lock, "inet_ntoa", __LINE__, __FILE__)
#define LOCK_MOZJS              mt_pthread_mutex_lock(&_mt_mozjs_lock, "mozjs", __LINE__, __FILE__)
#define UNLOCK_MOZJS            mt_pthread_mutex_unlock(&_mt_mozjs_lock, "mozjs", __LINE__, __FILE__)

/*
   see also tools.c:xdebug() for a similar situation caused by log/debug
   functions calling themselves recursively (though indirectly), hence causing
   deadlock situations in a multithreaded environment.
 */
#define TRYLOCK_LOG             mt_pthread_mutex_trylock(&_mt_log_lock, "log", __LINE__, __FILE__)

#define _h_errno_ (*((int *)pthread_getspecific(cfg.herrno_key)))



extern int mt_pthread_mutex_lock(pthread_mutex_t *, char *, int line, const char *filename);
extern int mt_pthread_mutex_trylock(pthread_mutex_t * mutex, char *id, int line, const char *filename);
extern int mt_pthread_mutex_unlock(pthread_mutex_t *, char *, int line, const char *filename);

typedef struct _mt_semaphore
{
  long v;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} mt_semaphore;

extern int mt_semaphore_init(mt_semaphore *);
extern int mt_semaphore_initv(mt_semaphore *, int v_startvalue);
extern int mt_semaphore_destroy(mt_semaphore *);
extern long mt_semaphore_up(mt_semaphore *);
extern long mt_semaphore_down(mt_semaphore *);
extern long mt_semaphore_timed_down(mt_semaphore *, int);
extern long mt_semaphore_timed_wait(mt_semaphore *, int);
extern long mt_semaphore_decrement(mt_semaphore *);

#else

#define MT_IS_MAIN_THREAD() TRUE

#define LOCK_CFG_URLSTACK
#define UNLOCK_CFG_URLSTACK
#define LOCK_CFG_URLHASH
#define UNLOCK_CFG_URLHASH
#define LOCK_CFG_FILEHASH
#define UNLOCK_CFG_FILEHASH
#define LOCK_COOKIES
#define UNLOCK_COOKIES
#define LOCK_AUTHINFO
#define UNLOCK_AUTHINFO
#define LOCK_DNS
#define UNLOCK_DNS
#define LOCK_SSL_MAP
#define UNLOCK_SSL_MAP
#define LOCK_GETLFNAME
#define UNLOCK_GETLFNAME
#define LOCK_DCNT
#define UNLOCK_DCNT
#define LOCK_TIME
#define UNLOCK_TIME
#define LOCK_OUTPUT
#define UNLOCK_OUTPUT
#define LOCK_GHBN
#define UNLOCK_GHBN
#define LOCK_PROXY
#define UNLOCK_PROXY
#define LOCK_DIRR
#define UNLOCK_DIRR
#define LOCK_TCNT
#define UNLOCK_TCNT
#define LOCK_GTKTREE
#define UNLOCK_GTKTREE
#define LOCK_GTKSTATUS
#define UNLOCK_GTKSTATUS
#define LOCK_GTKLOG
#define UNLOCK_GTKLOG
#define LOCK_REJCNT
#define UNLOCK_REJCNT
#define LOCK_FAILCNT
#define UNLOCK_FAILCNT
#define LOCK_URL(lurl)
#define UNLOCK_URL(lurl)
#define LOCK_GCFG
#define UNLOCK_GCFG
#define LOCK_NSCACHE
#define UNLOCK_NSCACHE
#define LOCK_ROBOTS
#define UNLOCK_ROBOTS
#define LOCK_RECACT
#define UNLOCK_RECACT
#define LOCK_TAG_HASH
#define UNLOCK_TAG_HASH
#define LOCK_INETNTOA
#define UNLOCK_INETNTOA
#define LOCK_MOZJS
#define UNLOCK_MOZJS

#define TRYLOCK_LOG

#define _h_errno_ h_errno

#endif

extern void mt_init(void);

#endif
