/***************************************************************************/  
/*    This code is part of WWW grabber called pavuk                        */ 
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */ 
/*    Distributed under GPL 2 or later                                     */ 
/***************************************************************************/ 
  
#ifndef _mime_h_
#define _mime_h_
  
#include "doc.h"
extern const char *mime_get_type_ext(const char *mime_type);
extern char *get_mime_n_param_val_str(char *mime_type, char *src, int occurrences_to_skip);
extern int mime_ext_matches_type(const char *extension_to_test, const char *mime_type, const char *opt_mime_type_ext);

#define get_mime_param_val_str(param_id, mimet) \
  get_mime_n_param_val_str(param_id, mimet, 0) 
#endif
  
