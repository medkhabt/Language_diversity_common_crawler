/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _log_h_
#define _log_h_

#include "doc.h"
#include "url.h"



typedef struct
{
  struct timeval start_time;
  struct timeval end_time;

  int transmitted_request_count;
  int failed_request_count;     /* total number of errors while hammering this page */
  int failed_socket_count;
  int failed_connect_count;     /* total number of connect errors */
  int failed_tx_count;          /* total number of transmit errors */
  int failed_response_count;    /* number of server-side failures (guestimate!) while ... etc ... */
  int unexpected_response_count;        /* number of failed response MIME code comparisons while ... */
  int bad_content_count;

  int page_size;                /* number of URLs/requests in page */

  struct timeval total_wait_time;
  struct timeval total_timeout_time;
  struct timeval total_connect_timeout_time;
  struct timeval total_write_timeout_time;
  struct timeval total_read_timeout_time;
} page_statistics_t;

typedef struct
{
  struct timeval start_time;
  struct timeval end_time;

  int transmitted_request_count;
  int failed_request_count;     /* total number of side errors while hammering this page */
  int failed_socket_count;
  int failed_connect_count;     /* total number of connect errors */
  int failed_tx_count;          /* total number of transmit errors */
  int failed_response_count;    /* number of server-side failures (guestimate!) while ... etc ... */
  int unexpected_response_count;        /* number of failed response MIME code comparisons while ... */
  int bad_content_count;

  int recording_size;           /* number of records in replay recording */
  int run_count;
  int page_count;

  struct timeval total_run_time;
  struct timeval total_page_time;
  struct timeval total_wait_time;
  struct timeval total_timeout_time;
  struct timeval total_connect_timeout_time;
  struct timeval total_write_timeout_time;
  struct timeval total_read_timeout_time;
} summary_statistics_t;




extern void log_start(const char *message);
extern void log_end(void);
extern void log_str(const char *message);
extern void short_log(doc * docp, url * urlp, const char *urlstr);
extern void short_log_header(const char *msgstr);
extern void time_log(doc *, int mode, const char *urlstr);
extern void time_log_header(int mode, const char *str);

extern void init_page_stats(page_statistics_t * d);
extern void init_summary_stats(summary_statistics_t * d);



#endif
