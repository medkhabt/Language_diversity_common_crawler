/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include "decode.h"
#include "tools.h"

#ifdef HAVE_ZLIB_H
#include <zlib.h>
#endif

#define BUFSIZE PATH_MAX

/********************************************************/
/* zapise obsah pamete na adrese buf s dlzkou len       */
/* na deskriptor fd                                     */
/* FIXME: Translate me!                                 */
/********************************************************/
static int writerm(int len, char *buf, int fd)
{
  char *p = buf;
  int fsize = BUFSIZE;

  while(p < (buf + len))
  {
    if((p + BUFSIZE) > (buf + len))
      fsize = buf + len - p;
    if(write(fd, p, fsize) < 0)
    {
      return -1;
    }
    p += BUFSIZE;
  }

  return 0;
}

/********************************************************/
/* vypis subor do deskriptora fd                        */
/* FIXME: Translate me!                                 */
/********************************************************/
static int writerf(char *fname, int fd)
{
  char buf[BUFSIZE];
  int len;
  int fnfd;

  DEBUG_BUFIO("open(%s)\n", fname);
  fnfd = tl_open(fname, O_BINARY | O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  DEBUG_BUFIO("opened(%s) --> %d/%s\n", fname, errno, strerror(errno));
  if(fnfd < 0)
  {
    xperror(mk_native(fname));
    return -1;
  }

  while((len = read(fnfd, buf, sizeof(buf))) > 0 || errno == EWOULDBLOCK)
  {
    if(write(fd, buf, len) != len)
    {
      close(fnfd);
      return -1;
    }
  }

  close(fnfd);
  return 0;
}

/* static char inflate_buffer[1024 * 1024];  -- [i_a] NOT thread safe! */
#define MIN_ZLIB_BUF_SIZE  (1024 * 1024)

int inflate_decode(char *inbuf, int insize, char **outbuf, ssize_t * outsize, char *fname)
{
#ifdef HAVE_ZLIB
  int i;
  unsigned long retlen;
  char *ret = 0;
  z_streamp zp;
  void *inflate_buffer;
  unsigned long bufsize;

  *outbuf = NULL;
  *outsize = 0;

  /* "deflate" inflation is tricky because there are two possible */
  /* encodings. We try one, if that fails, we try the other. If */
  /* that fails, we give up. */

  bufsize = insize * 5;
  if(bufsize < MIN_ZLIB_BUF_SIZE)
  {
    bufsize = MIN_ZLIB_BUF_SIZE;
  }
  inflate_buffer = _malloc(bufsize);

  i = uncompress((unsigned char *) inflate_buffer, &retlen, (unsigned char *) inbuf, insize);
  if(i == Z_OK)
  {
    ret = _malloc(retlen);
    if(ret == 0)
    {
      xperror("inflate: out of memory");
      _free(inflate_buffer);
      return -1;
    }
    else
    {
      memcpy(ret, inflate_buffer, retlen);
    }
    _free(inflate_buffer);
    *outbuf = ret;
    *outsize = retlen;
    return 0;
  }
  else if(i != Z_DATA_ERROR)
  {
    xperror1("inflate: uncompress error", zError(i));
    _free(inflate_buffer);
    return -1;
  }

  zp = _malloc(sizeof(*zp));
  ASSERT(zp);                   /* If alloca fails we're doomed anyway */
  zp->next_in = (unsigned char *) inbuf;
  zp->next_out = (unsigned char *) inflate_buffer;
  zp->avail_in = insize;
  zp->avail_out = bufsize;
  zp->zalloc = 0;
  zp->zfree = 0;
  zp->opaque = 0;
  *outbuf = 0;
  i = inflateInit(zp);
  if(i != Z_OK)
  {
    xperror1("inflate: inflateInit error", zError(i));
    if(zp->msg)
    {
      xperror1("zlib", zp->msg);
    }
    _free(zp);
    _free(inflate_buffer);
    return -1;
  }

  if((i = inflate(zp, Z_FINISH)) == Z_STREAM_END)
  {
    ret = _malloc(zp->avail_out);
    if(ret == 0)
    {
      xperror("Can't copy inflated file");
      _free(zp);
      _free(inflate_buffer);
      return -1;
    }
    else
    {
      memcpy(ret, zp->next_out, zp->avail_out);
    }
    *outbuf = ret;
    *outsize = zp->avail_out;
    inflateEnd(zp);
    _free(zp);
    _free(inflate_buffer);
    return 0;
  }
  else
  {
    if(i == Z_OK)
    {
      xperror("Whoops, zlib coding problem");
    }
    else
    {
      xperror1("inflate: inflate error", zError(i));
      if(zp->msg)
        xperror1("zlib", zp->msg);
    }
    inflateEnd(zp);
    _free(zp);
    _free(inflate_buffer);
    return -1;
  }
#else
  fprintf(stderr, "inflate mode not supported.\n");
  /* _free(zp); */
  return -1;
#endif
}

int gzip_decode(char *inbuf, int insize, char **outbuf, ssize_t * outsize, char *fname)
{
#if defined(HAVE_WAITPID) && defined(HAVE_FORK) && defined(HAVE_PIPE)
  int writepipe[2];
  pid_t pidg;
  char *out = NULL;
  int outcnt = 0, len;
  char buf[BUFSIZE];
  bool_t err = TRUE;
  int outf;

  if(cfg.cache_dir)
  {
    snprintf(buf, sizeof(buf), "%s%spavuk_decoder.tmp.XXXXXX", cfg.cache_dir, (tl_is_dirname(cfg.cache_dir) ? "" : "/"));
  }
  else
  {
    strncpy(buf, "pavuk_decoder.tmp.XXXXXX", sizeof(buf));
  }
  buf[sizeof(buf) - 1] = '\0';

  outf = tl_mkstemp(buf);
  if(outf == -1)
  {
    xperror1("tl_mkstemp", buf);
    return -1;
  }
  tl_unlink(buf);

  *outsize = 0;
  *outbuf = NULL;

  if(pipe(writepipe))
  {
    xperror("pipe");
    return -1;
  }

  pidg = fork();
  if(pidg < 0)
  {
    xperror("fork");
    close(writepipe[0]);
    close(writepipe[1]);
    return -1;
  }
  if(pidg == 0)
  {
    close(0);
    dup(writepipe[0]);
    close(writepipe[0]);
    close(writepipe[1]);

#ifdef HAVE_ZLIB
    {
      gzFile gzfd;
      int ilen, olen;

      if(!(gzfd = gzdopen(0, "rb")))
      {
        xperror("gzdopen");
        exit(1);
      }

      while((ilen = gzread(gzfd, buf, sizeof(buf))) > 0)
      {
        olen = write(outf, buf, ilen);

        if(olen != ilen)
          break;
      }
      if(ilen < 0)
      {
        int err;
        xprintf(0, "decode: %s\n", gzerror(gzfd, &err));
        exit(1);
      }
      else if(ilen)
      {
        xperror("decode");
        exit(1);
      }
      gzclose(gzfd);
      close(outf);
      exit(0);
    }
    exit(0);
#else
    close(1);
    dup(outf);
    close(outf);
    execlp(GZIP_CMD, "gunzip", "-cf", NULL);
    xperror("gunzip exec");
    exit(1);
#endif
  }

  close(writepipe[0]);

  err = fname ? writerf(fname, writepipe[1]) : writerm(insize, inbuf, writepipe[1]);

  if(err)
  {
    xperror("gzip_decode");
  }

  close(writepipe[1]);

  waitpid(pidg, NULL, 0);

  if((off_t) - 1 == lseek(outf, 0, SEEK_SET))
  {
    close(outf);
    return -1;
  }

  while((len = read(outf, buf, sizeof(buf))) > 0)
  {
    out = (char *) _realloc(out, outcnt + len + 1);
    memcpy(out + outcnt, buf, len);
    outcnt += len;
    *(out + outcnt) = '\0';
  }

  close(outf);

  *outsize = outcnt;
  *outbuf = out;
  return 0;
#else
  /* perform extract in memory... */
#ifdef HAVE_ZLIB
  static int const gz_magic[2] = { 0x1f, 0x8b };        /* gzip magic header */

/* gzip flag byte */
#define ASCII_FLAG   0x01       /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02       /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04       /* bit 2 set: extra field present */
#define ORIG_NAME    0x08       /* bit 3 set: original file name present */
#define COMMENT      0x10       /* bit 4 set: file comment present */
#define RESERVED     0xE0       /* bits 5..7: reserved */

  int i;
  char *ret = 0;
  z_streamp zp;
  unsigned long bufsize;
  unsigned char *inflate_buffer;
  int method;
  int flags;
  unsigned int len;
  unsigned char *start;
  unsigned long ulen;
  bool_t inbuf_allocated = FALSE;

  *outbuf = NULL;
  *outsize = 0;

  bufsize = insize * 5;
  if(bufsize < MIN_ZLIB_BUF_SIZE)
  {
    bufsize = MIN_ZLIB_BUF_SIZE;
  }
  inflate_buffer = _malloc(bufsize);

  zp = _malloc(sizeof(*zp));
  ASSERT(zp);                   /* If alloca fails we're doomed anyway */
  if(!fname)
  {
    ASSERT(inbuf);
    zp->next_in = (unsigned char *) inbuf;
    zp->next_out = (unsigned char *) inflate_buffer;
    zp->avail_in = insize;
    zp->avail_out = bufsize;
    zp->zalloc = 0;
    zp->zfree = 0;
    zp->opaque = 0;
  }
  else
  {
    int rlen = 0;

    bufio *fd = bufio_open(fname, O_BINARY | O_RDONLY);
    ASSERT(!inbuf);
    if(fd)
    {
      /*
         off_t flen = bufio_seek(fd, 0, SEEK_END);
         bufio_seek(fd, 0, SEEK_SET);
         ASSERT(flen == insize);
       */
      inbuf = _malloc(insize);
      inbuf_allocated = TRUE;
      rlen = bufio_read(fd, inbuf, insize);
      if(rlen != insize)
      {
        xperror1("gzip_decode", mk_native(fname));
        if(inbuf_allocated)
          _free(inbuf);
        _free(inflate_buffer);
        _free(zp);
        return -1;
      }
      bufio_close(fd);
      fd = NULL;
    }

    zp->next_in = (unsigned char *) inbuf;
    zp->next_out = (unsigned char *) inflate_buffer;
    zp->avail_in = (rlen >= 0 ? rlen : 0);
    zp->avail_out = bufsize;
    zp->zalloc = 0;
    zp->zfree = 0;
    zp->opaque = 0;
  }
  *outbuf = 0;
  i = inflateInit2(zp, -MAX_WBITS);
  /* windowBits is passed < 0 to tell that there is no zlib header.
   * Note that in this case inflate *requires* an extra "dummy" byte
   * after the compressed stream in order to complete decompression and
   * return Z_STREAM_END. Here the gzip CRC32 ensures that 4 bytes are
   * present after the compressed stream.
   */
  /* i = inflateInit(zp); */
  if(i != Z_OK)
  {
    xvperror(DBGvars(mk_native(fname)), "gzip: inflateInit2 error %s\n", zError(i));
    if(zp->msg)
    {
      xvperror(DBGvars(mk_native(fname)), "zlib: %s\n", zp->msg);
    }
    if(inbuf_allocated)
      _free(inbuf);
    _free(inflate_buffer);
    _free(zp);
    return -1;
  }
  /* check_header(zp); ** skip the .gz header */
  /* Peek ahead to check the gzip magic header */
  if(zp->next_in[0] != gz_magic[0] || zp->next_in[1] != gz_magic[1])
  {
    xvperror(DBGvars(mk_native(fname)), "gzip: error: magic header fails\n");
    inflateEnd(zp);
    if(inbuf_allocated)
      _free(inbuf);
    _free(inflate_buffer);
    _free(zp);
    return -1;
  }
  zp->avail_in -= 2;
  zp->next_in += 2;

  /* Check the rest of the gzip header */
  method = *zp->next_in++;
  flags = *zp->next_in++;
  zp->avail_in -= 2;
  if(method != Z_DEFLATED || (flags & RESERVED) != 0)
  {
    i = Z_DATA_ERROR;
    xvperror(DBGvars(mk_native(fname)), "gzip: error: unsupported compression method (%d) / flags (%d)\n", method, flags);
    inflateEnd(zp);
    if(inbuf_allocated)
      _free(inbuf);
    _free(inflate_buffer);
    _free(zp);
    return -1;
  }

  /* Discard time, xflags and OS code: */
  /* for (len = 0; len < 6; len++) (void)get_byte(s); */
  zp->avail_in -= 6;
  zp->next_in += 6;

  if((flags & EXTRA_FIELD) != 0)
  {
    /* skip the extra field */
    len = (unsigned int) *zp->next_in++;
    len += ((unsigned int) *zp->next_in++) << 8;
    zp->avail_in -= 2;
    /* len is garbage if EOF but the loop below will quit anyway */
    /* while (len-- != 0 && get_byte(s) != EOF) ; */
    zp->avail_in -= len;
    zp->next_in += len;
  }
  if((flags & ORIG_NAME) != 0)
  {
    /* skip the original file name */
    /* while ((c = get_byte(s)) != 0 && c != EOF) ; */
    int l = strnlen((const char *) zp->next_in, zp->avail_in);
    zp->avail_in -= l + 1;
    zp->next_in += l + 1;
  }
  if((flags & COMMENT) != 0)
  {
    /* skip the .gz file comment */
    /* while ((c = get_byte(s)) != 0 && c != EOF) ; */
    int l = strnlen((const char *) zp->next_in, zp->avail_in);
    zp->avail_in -= l + 1;
    zp->next_in += l + 1;
  }
  if((flags & HEAD_CRC) != 0)
  {
    /* skip the header crc */
    /* for (len = 0; len < 2; len++) (void)get_byte(s); */
    zp->avail_in -= 2;
    zp->next_in += 2;
  }

  if(zp->avail_in <= 0)
  {
    i = Z_DATA_ERROR;
    xvperror(DBGvars(mk_native(fname)), "gzip: error: faulty header, not enough input data\n");
    inflateEnd(zp);
    if(inbuf_allocated)
      _free(inbuf);
    _free(inflate_buffer);
    _free(zp);
    return -1;
  }


  /* gzread() but now in memory */
  start = zp->next_out;         /* starting point for crc computation */
  if((i = inflate(zp, Z_FINISH)) == Z_STREAM_END)
  {
    /* Check CRC and original size */
    unsigned long check_crc;
    unsigned long crc = crc32(0L, start, (zp->next_out - start));
    start = zp->next_out;

    check_crc = zp->next_in[0];
    check_crc += ((unsigned long) zp->next_in[1]) << 8;
    check_crc += ((unsigned long) zp->next_in[2]) << 16;
    check_crc += ((unsigned long) zp->next_in[3]) << 24;
    zp->avail_in -= 4;
    zp->next_in += 4;

    if(check_crc != crc)
    {
      i = Z_DATA_ERROR;
      xvperror(DBGvars(mk_native(fname)), "gzip: error: CRC32 check failed\n");
      inflateEnd(zp);
      if(inbuf_allocated)
        _free(inbuf);
      _free(inflate_buffer);
      _free(zp);
      return -1;
    }

    *outsize = (zp->next_out - inflate_buffer);

    ulen = zp->next_in[0];
    ulen += ((unsigned long) zp->next_in[1]) << 8;
    ulen += ((unsigned long) zp->next_in[2]) << 16;
    ulen += ((unsigned long) zp->next_in[3]) << 24;
    zp->avail_in -= 4;
    zp->next_in += 4;
    if(ulen != *outsize)
    {
      i = Z_DATA_ERROR;
      xvperror(DBGvars(mk_native(fname)), "gzip: error: expanded data size check failed\n");
      inflateEnd(zp);
      if(inbuf_allocated)
        _free(inbuf);
      _free(inflate_buffer);
      _free(zp);
      return -1;
    }

    ret = _malloc(ulen);
    if(ret == 0)
    {
      xvperror(DBGvars(mk_native(fname)), "gzip: Can't copy expanded data\n");
      inflateEnd(zp);
      if(inbuf_allocated)
        _free(inbuf);
      _free(inflate_buffer);
      _free(zp);
      return -1;
    }
    else
    {
      memcpy(ret, inflate_buffer, ulen);
    }
    *outbuf = ret;
    inflateEnd(zp);
    if(inbuf_allocated)
      _free(inbuf);
    _free(inflate_buffer);
    _free(zp);
    return 0;
  }
  else
  {
    if(i == Z_OK)
    {
      xvperror(DBGvars(mk_native(fname)), "gzip: Whoops, coding problem\n");
    }
    else
    {
      xvperror(DBGvars(mk_native(fname)), "gzip: decompression error %s\n", zError(i));
      if(zp->msg)
        xvperror(DBGvars(mk_native(fname)), "zlib: %s\n", zp->msg);
    }
    inflateEnd(zp);
    if(inbuf_allocated)
      _free(inbuf);
    _free(inflate_buffer);
    _free(zp);
    return -1;
  }
#else
  xperror("gzip decode mode not supported");
  return -1;
#endif
#endif
}
