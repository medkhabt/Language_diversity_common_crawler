/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "schedule.h"
#include "mode.h"

/************************************************/
/* naplanovanie vykonania programu v danom case */
/* pri aktualnej konfiguracii                   */
/* FIXME: Translate me!                         */
/************************************************/
int at_schedule(void)
{
  int tfd;
  char *p, *op;
  char pcmd[10 * PATH_MAX];
  char cmd[PATH_MAX];
  char tmp[PATH_MAX];
  char tform[3] = "%s";
#ifdef I_FACE
  bool_t xi_save;
#endif
  strcpy(tmp, "pavuk_schedule.tmp.XXXXXX");
  tfd = tl_mkstemp(tmp);
  if(tfd < 0)
  {
    xperror1("tl_mkstemp", tmp);
    return -1;
  }

  strcpy(cmd, cfg.sched_cmd ? cfg.sched_cmd : AT_CMD);

  p = cmd;
  op = pcmd;
  *op = '\0';

  while(*p)
  {
    if(*p == '%')
    {
      p++;
      switch (*p)
      {
      case 'f':
        strcat(op, tmp);
        break;
      case 't':
        strftime(op, 10, "%H:%M", cfg.time);
        break;
      default:
        tform[1] = *p;
        strftime(op, 10, tform, cfg.time);
      }
      p++;
      while(*op)
        op++;
    }
    else
    {
      *op = *p;
      op++;
      p++;
      *op = '\0';
    }
  }
#ifdef I_FACE
  xi_save = cfg.xi_face;
  cfg.xi_face = FALSE;
#endif
  {
    bufio *fd = bufio_fdopen(tfd);
    cfg_dump_cmd_fd(fd);
    bufio_close(fd);
    fd = NULL;
  }
#ifdef I_FACE
  cfg.xi_face = xi_save;
#endif

  if(tl_system(pcmd))
  {
    xperror1("Failed to run command", pcmd);
    if(tl_unlink(tmp))
      xperror1("Cannot unlink/delete file", mk_native(tmp));
    return -1;
  }

  if(tl_unlink(tmp))
    xperror1("Cannot unlink/delete file", mk_native(tmp));
  close(tfd);

  return 0;
}
