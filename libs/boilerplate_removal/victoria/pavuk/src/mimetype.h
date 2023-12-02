/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _mimetype_h_
#define _mimetype_h_

struct mime_type_ext
{
  const char *mimet;
  const char *ext;
};

extern const char **get_mimetypes_collection(void);
extern struct mime_type_ext *get_mime_type_ext_collection(void);
extern int cfg_load_mime_types(const char *filename);
extern void init_mime_types(void);
extern void free_mime_types(void);

#endif
