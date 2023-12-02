#!/bin/sh

if [ "$1" != "--noclean" ]; then
  # If your system does not use awk, set the AWK environment variable.
  # This is no configure script and will not do complicated tests.
  if [ -z "$AWK" ]; then
    AWK="awk"
  fi

  # create the killfile
  cat /dev/null >$$.lis

  # all the files in .cvsignore
  for x in `find . -name .cvsignore`
  do
    $AWK '{printf "%s/%s\n","'`dirname $x`'",$0}' $x >>$$.lis
  done

  # remove backup files
  find . -name "*~" >>$$.lis
  find . -name ".*~" >>$$.lis
  find . -name "*.bak" >>$$.lis
  find . -name ".*.bak" >>$$.lis
  find . -name ".#*" >>$$.lis

  # remove the exceptions from the list
  # these are non-cvs files, we nevertheless want to keep
  #
  # the result of grep is redirected to rm command
  cat $$.lis | awk '{printf "ls -d -1 %s 2>/dev/null\n",$0}' |sh |
  #grep -v \
  #-e "file1$" \
  #-e "file2$" \
  # |
  $AWK '{printf "rm -rvf \"%s\"\n",$0}' |sh

  # delete the killfile
  rm $$.lis
fi

if [ "$1" != "--clean" ]; then
  aclocal -I m4
  autoheader
  autoconf
  automake -a --gnu
#  autoreconf -i

  if [ "$1" = "--suse" ]; then
    ./configure --with-locale-dir=/usr/share/locale
  elif [ "$1" = "--susethread" ]; then
    ./configure --with-locale-dir=/usr/share/locale --enable-threads
  elif [ "$1" = "--local" ]; then
    ./configure --prefix=`pwd`/test --enable-utf-8
  elif [ "$1" = "--hammer" ]; then
    ./configure --enable-hammer
  else
    ./configure --disable-gtk2
  fi
  make -s
  if [ "$1" = "--local" ]; then
    make install
  fi
fi
