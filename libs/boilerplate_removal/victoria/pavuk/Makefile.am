# Process this file with automake to produce Makefile.in (in this,
# and all subdirectories).

SUBDIRS = src man icons tests doc @MSUBDIRS@

EXTRA_DIST = pavuk.spec pavukrc.sample \
	pavuk.lsm pavuk.nsi \
	pavuk_authinfo.sample pavuk.desktop pavuk-gtkrc \
	INSTALL README \
	README.win32 README.maintainer \
	NEWS TODO CREDITS COPYING ABOUT-NLS BUGS MAILINGLIST \
	FAQ wget-pavuk.HOWTO jsbind.txt ChangeLog \
	button_icons/README \
	button_icons/examplerc \
	button_icons/frederic/break.xpm \
	button_icons/frederic/configure.xpm \
	button_icons/frederic/continue.xpm \
	button_icons/frederic/exit.xpm \
	button_icons/frederic/gobg.xpm \
	button_icons/frederic/restart.xpm \
	button_icons/frederic/stop.xpm \
	button_icons/gnome/break.xpm \
	button_icons/gnome/configure.xpm \
	button_icons/gnome/continue.xpm \
	button_icons/gnome/exit.xpm \
	button_icons/gnome/gobg.xpm \
	button_icons/gnome/restart.xpm \
	button_icons/gnome/stop.xpm \
	button_icons/old/break.xpm \
	button_icons/old/configure.xpm \
	button_icons/old/continue.xpm \
	button_icons/old/exit.xpm \
	button_icons/old/gobg.xpm \
	button_icons/old/restart.xpm \
	button_icons/old/stop.xpm \
	scripts/pavuk.sh \
	scripts/tperf.sh \
	m4/cpp_option.m4 \
	m4/sighandler_t.m4 \
	m4/signal.m4 \
	m4/socklen_t.m4 \
	pavuk.commandline-help.txt \
	build/msvc2005/pavuk.vcproj build/msvc2005/pavuk.sln \
	build/msvc2005/ac-config.win32.h \
	build/msvc2005/README.MSVC2005.txt \
	profile_me.sh \
	fix_crlf.sh \
	cvsbuild.sh \
	mime.types


DISTCLEANFILES = 
CLEANFILES = *~ *.bak */*~ */*.bak */*/*~ */*/*.bak

bin_SCRIPTS = \
	scripts/pavuk.sh \
	scripts/tperf.sh

.PHONY: xref reindent htmltidy man-html

xref:
	echo "Making xref from src/*.c"; 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) xref-src);

reindent:
	echo "Reformatting sources"; 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) reindent);

htmltidy:
	(cd man && $(MAKE) $(AM_MAKEFLAGS) htmltidy);
	(cd doc && $(MAKE) $(AM_MAKEFLAGS) htmltidy);

man-html:
	(cd man && $(MAKE) $(AM_MAKEFLAGS) man-html);
	(cd doc && $(MAKE) $(AM_MAKEFLAGS) man-html);


