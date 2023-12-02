/*
   additions for replay web attack mode (hammer_mode 1)
 */

#ifndef _HAMMER_MODE_1_H_
#define _HAMMER_MODE_1_H_ 1

#ifdef INCLUDE_CHUNKY_DoS_FEATURES

#define HAMMER_DUMB_REPEAT              0
#define HAMMER_RECORD_AND_REPLAY        1

typedef struct
{
  protocol type;
  char *host_name;
  int host_port;
  struct sockaddr *local_addr;
  int local_addr_len;
  struct sockaddr *dest_addr;
  int dest_addr_len;
  int socket_family;
  int socket_proto;

  int ref_count;

  char *req_mime_header;
  size_t req_mime_header_len;
  char *req_data;
  size_t req_data_len;

  char *sent_to_server;         /* byte block sent to server as request */
  size_t sent_size;             /* total size of byte block */
  size_t header_sent_size;      /* size of initial (header) part of byte block */

  int expected_response_code;   /* 200 OK, or anything else. sanity check */
  bool_t expect_persistent;     /* assume the next item may be got by keeping the persistent connection */
  bool_t is_page_starter;
  char *url4display;
} activity_record;


extern int hammerflag_level_parse(const char *source_str, unsigned long *dst);
extern char *hammerflag_level_construct(unsigned long level);


extern activity_record *new_activityrecord(void);
extern void free_deep_activityrecord(activity_record * p);
extern void dlfree_func_act_rec_queue(void *key_data);

extern activity_record *get_registered_activityrecord(activity_record * parent, url * urlp);
extern void dump_actrec_list(void);
extern void clean_actrec_list(void);
extern void replay_the_recording(void);
extern void fixup_record(activity_record * rec);

#endif

#endif
